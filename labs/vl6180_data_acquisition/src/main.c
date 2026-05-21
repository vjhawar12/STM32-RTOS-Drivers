#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "i2c.h"
#include "portmacro.h"
#include "projdefs.h"
#include "platform.h"
#include "task.h"
#include <stdint.h>
#include "stdio.h"
#include "semphr.h"
#include "timers.h"
#include "vl6180.h" 
#include "uart.h"
#include "stdbool.h"

/* 
NOTE: The peripheral driver for this is in /common/drivers. This file uses that driver I wrote. 
*/

#define PERIOD_1S_CYCLES 8084000
#define PERIOD_100MS 1000

TaskHandle_t acquisition_task_handle; 
QueueHandle_t queue;

void acquisition_task(void* pvParams) {
    vl6180_sample_t sample;
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
        vl6180_read_distance_mm(&sample); 
        xQueueSend(queue, &sample, pdMS_TO_TICKS(200));
    }
}

void processing_task(void *pvParams) {
    char buffer[64];
    vl6180_sample_t sample;
    while (1) {
        xQueueReceive(queue, &sample, portMAX_DELAY); 
        if (sample.valid) {
            snprintf(buffer, 64, "[%d ms] Distance: %d\r\n", sample.timestamp, sample.distance); 
        } else {
            snprintf(buffer, 64, "[%d ms] Invalid reading\r\n", sample.timestamp); 
        }
        uart_outstring(buffer);
    }
}

int main() {
    PLL_Init();
    uart_Init();
    uart_enable_rx_interrupt();
    i2c_init();
    led2_Init();
    bool tof_alive = vl6180_alive();
    if (!tof_alive) {
        uart_outstring("Could not reach TOF sensor!\r\n"); 
        while(1) {
            toggle_led2(PERIOD_1S_CYCLES); 
        }
    }
    vl6180_init();
    xTaskCreate(acquisition_task, "acquisition task", 512, NULL, 5, &acquisition_task_handle); 
    xTaskCreate(processing_task, "processing task", 512, NULL, 3, NULL); 
    vl6180_set_continuous(acquisition_task_handle);
    vTaskStartScheduler();
}