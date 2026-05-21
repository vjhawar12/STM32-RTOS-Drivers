#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "irq.h"
#include "portmacro.h"
#include "projdefs.h"
#include "platform.h"
#include "task.h"
#include <stdint.h>
#include "stdio.h"
#include "semphr.h"
#include "timers.h"
#include "adxl345.h" 
#include "uart.h"

/* 
NOTE: The peripheral driver for this is in /common/drivers. This file uses that driver I wrote. 
*/

#define PERIOD_1S_CYCLES 8084000
#define PERIOD_100MS 1000

TaskHandle_t acquisition_task_handle, consumer_task_handle; 
QueueHandle_t queue;

typedef struct adxl345_sample_time_t {
    adxl345_sample_t sample;
    uint32_t time; // timestamp at acquisition not processing time
} adxl345_sample_time_t; 

void oneshot_timer_isr() {}

void periodic_timer_isr() {
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE; 
    // using task notifications to wake up acquisition task
    vTaskNotifyGiveFromISR(acquisition_task_handle, &pxHigherPriorityTaskWoken);  
    portYIELD_FROM_ISR(pxHigherPriorityTaskWoken); 
}

void uart2_isr() {}

// reads sensor data every 100 ms
void acquisition_task(void *pvParams) {
    adxl345_sample_t sample; 
    adxl345_sample_time_t sample_with_time;
    while (1) {
        // sleeping until woken by isr
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
        if (adxl345_read_xyz(&sample)) {
            sample_with_time.sample = sample; 
            // setting time to timestamp at acquisition not processing
            sample_with_time.time = xTaskGetTickCount() * portTICK_PERIOD_MS; 
            // sending to processing task via queue
            xQueueSend(queue, &sample_with_time, pdMS_TO_TICKS(20)); 
        } 
    }
}

// prints sensor data out over uart
void consumer_task(void *pvParams) {
    char buffer[64]; 
    adxl345_sample_time_t accel_data;
    while (1) {
        // sleeping until woken by acquisition task
        xQueueReceive(queue, &accel_data, portMAX_DELAY); 
        snprintf(buffer, sizeof(buffer), "[%lu ms] X: %d, Y: %d, Z: %d\r\n", accel_data.time, accel_data.sample.accel_x, accel_data.sample.accel_y, accel_data.sample.accel_z); 
        uart_outstring(buffer); 
    }
}


int main(void) {
    PLL_Init();
    led2_Init();
    uart_Init();
    periodic_timer_init(PERIOD_100MS); 
    xTaskCreate(acquisition_task, "acquisition task", 512, NULL, 5, &acquisition_task_handle);
    xTaskCreate(consumer_task, "consumer task", 512, NULL, 7, &consumer_task_handle);
    // queue with 8 capacity accomodates delays in processing task without losing the sample
    queue = xQueueCreate(8, sizeof(adxl345_sample_time_t)); 
    uart_enable_rx_interrupt();
    adxl345_init(); 
    periodic_timer_start(); 
    vTaskStartScheduler();
    while (1) {

    } 
}