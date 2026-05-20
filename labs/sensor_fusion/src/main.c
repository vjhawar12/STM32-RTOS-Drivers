#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "irq.h"
#include "portmacro.h"
#include "projdefs.h"
#include "platform.h"
#include "task.h"
#include <stdint.h>
#include <string.h>
#include "stdio.h"
#include "semphr.h"
#include "timers.h"
#include "adxl345.h" 
#include "math.h"
#include "vl6180.h"
#include "i2c.h"

/* 
NOTE: The peripheral driver for this is in /common/drivers. This file uses that driver I wrote. 
*/

#define PERIOD_1S_CYCLES 8084000
#define PERIOD_100MS 1000

typedef enum message_type_t {
    ACCEL,
    TOF
} message_type_t; 

typedef enum tof_state {
    CLOSE,
    CRITICAL,
    INVALID,
    CLEAR
} tof_state;

typedef enum accel_state {
    STABLE,
    MOVING,
    SHAKING
} accel_state;

typedef struct mixed_sensor_data_t {
    message_type_t message_type;
    uint32_t timestamp;
    char label[64];
    union {
        vl6180_sample_t tof;
        adxl345_sample_t accel;
    } data; 
} mixed_sensor_data_t;

QueueHandle_t sensor_queue;
TaskHandle_t tof_task_handle, accel_task_handle, processing_task_handle;

void periodic_timer_isr() {
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(accel_task_handle, &pxHigherPriorityTaskWoken); 
    portYIELD_FROM_ISR(pxHigherPriorityTaskWoken); 
}

void tof_data_retrieval(void *pvParams) {
    mixed_sensor_data_t sensor_data; 
    vl6180_sample_t tof_sample;
    while (1) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(200));
        vl6180_read_distance_mm(&tof_sample);
        sensor_data.message_type = TOF;
        sensor_data.data.tof = tof_sample;   
        if (sensor_data.data.tof.distance < 100) {
            strcpy(sensor_data.label, "PROXIMITY: CRITICAL"); 
        } else if (sensor_data.data.tof.distance < 300) {
            strcpy(sensor_data.label, "PROXIMITY: CLOSE"); 
        } else if (sensor_data.data.tof.distance >= 300 && sensor_data.data.tof.distance < 1000) {
            strcpy(sensor_data.label, "PROXIMITY: CLEAR"); 
        } else {
            strcpy(sensor_data.label, "PROXIMITY: INVALID"); 
        }
        sensor_data.timestamp = xTaskGetTickCount() * 1000 / configTICK_RATE_HZ; 
        xQueueSend(sensor_queue, &sensor_data, pdMS_TO_TICKS(200)); 
    }
}

float get_accel(adxl345_sample_t *accel) {
    return sqrtf(accel->accel_x * accel->accel_x) + (accel->accel_x * accel->accel_x) + (accel->accel_x * accel->accel_x); 
}

void accel_data_retrieval(void* pvParams) {
    mixed_sensor_data_t sensor_data; 
    adxl345_sample_t accel_sample;
    while (1) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(200)); 
        adxl345_read_xyz(&accel_sample); 
        sensor_data.message_type = ACCEL;
        sensor_data.data.accel = accel_sample;
        sensor_data.timestamp = xTaskGetTickCount() * 1000 / configTICK_RATE_HZ; 
        xQueueSend(sensor_queue, &sensor_data, pdMS_TO_TICKS(200)); 
    }
}

void data_processing(void *pvParams) {
    mixed_sensor_data_t data;
    char buffer[64];
    while (1) {
        xQueueReceive(sensor_queue, &data, portMAX_DELAY); 
        if (data.message_type == ACCEL) {
            snprintf(buffer, 64, "[%d ms] ACCEL x: %d y: %d z: %d | %s\r\n", data.timestamp, data.data.accel.accel_x, data.data.accel.accel_y, data.data.accel.accel_z, data.label); 
        } else if (data.message_type == TOF) {
            snprintf(buffer, 64, "[%d ms] TOF distance: %d mm | %s\r\n", data.timestamp, data.data.tof.distance, data.label);
        }
        uart_outstring(buffer); 
    }
}

int main(void) {
    PLL_Init();
    led2_Init();
    uart_Init();
    i2c_init();
    periodic_timer_init(PERIOD_100MS); 
    xTaskCreate(tof_data_retrieval, "tof task", 512, NULL, 7, &tof_task_handle);
    xTaskCreate(accel_data_retrieval, "accel task", 512, NULL, 7, &accel_task_handle);
    xTaskCreate(data_processing, "processing task", 512,NULL, 5, &processing_task_handle);
    // queue with 8 capacity accomodates delays in processing task without losing the sample
    sensor_queue = xQueueCreate(8, sizeof(mixed_sensor_data_t)); 
    uart_enable_rx_interrupt();
    adxl345_init();
    bool tof_alive = vl6180_alive();
    while (!tof_alive) {
        uart_outstring("Could not reach TOF sensor!\r\n"); 
        toggle_led2(PERIOD_1S_CYCLES); 
    } 
    vl6180_init();
    vl6180_set_continuous(tof_task_handle); 
    periodic_timer_start(); 
    vTaskStartScheduler();
    while (1) {

    } 
}