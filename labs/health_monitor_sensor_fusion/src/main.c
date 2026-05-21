#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "irq.h"
#include "portable.h"
#include "portmacro.h"
#include "projdefs.h"
#include "platform.h"
#include "task.h"
#include <stdint.h>
#include "stdio.h"
#include "semphr.h"
#include "timers.h"
#include "adxl345.h"
#include "vl6180.h"
#include "i2c.h"
#include "event_groups.h"

/* 
NOTE: The peripheral driver for this is in /common/drivers. This file uses that driver I wrote. 
*/

#define PERIOD_1S_CYCLES 8084000
#define PERIOD_100MS 1000
#define CRITICAL_THRESH 100
#define CLOSE_THRESH 300
#define CLEAR_THRESH 500
#define STABLE_THRESH 100
#define MOVING_THRESH 200
#define SHAKING_THRESH 500
#define TOF_BIT (1 << 0)
#define ACCEL_BIT (1 << 1)
#define PROCESSING_BIT (1 << 2)
#define HEALTH_BIT (1 << 3)
#define REQUIRED_BITS (TOF_BIT | ACCEL_BIT | PROCESSING_BIT | HEALTH_BIT)

typedef enum message_type_t {
    ACCEL,
    TOF
} message_type_t; 

typedef enum tof_state {
    CLEAR = 0,
    CLOSE = 1,
    CRITICAL = 2,
    INVALID = 3,
} tof_state;

typedef enum accel_state {
    STABLE = 4,
    MOVING = 5,
    SHAKING = 6,
    INVALID_ = 7,
} accel_state;

typedef enum system_state {
    FULLCLEAR = 8,
    NEAR_OBJECT = 9,
    OBSTACLE_WHILE_MOVING = 10,
    VIBRATION_OR_IMPACT = 11,
    SENSOR_FAULT = 12
} system_state; 

typedef struct mixed_sensor_data_t {
    message_type_t message_type;
    union {
        vl6180_sample_t tof;
        adxl345_sample_t accel;
    } data; 
    int state;
} mixed_sensor_data_t;

QueueHandle_t sensor_queue;
TaskHandle_t tof_task_handle, accel_task_handle, processing_task_handle, health_task_handle;
EventGroupHandle_t sensor_group;


void periodic_timer_isr() {
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(accel_task_handle, &pxHigherPriorityTaskWoken); 
    portYIELD_FROM_ISR(pxHigherPriorityTaskWoken); 
}

char* get_label(int value) {
    switch (value) {
        case STABLE:
            return "MOTION: STABLE";
            break;
        case MOVING:
            return "MOTION: MOVING";
            break;
        case SHAKING:
            return "MOTION: SHAKING";
            break;
        case INVALID_:
            return "MOTION: INVALID";
            break;
        case CLOSE:
            return "PROXIMITY: CLOSE";
            break;
        case CRITICAL:
            return "PROXIMITY: CRITICAL";
            break;
        case INVALID:
            return "PROXIMITY: INVALID";
            break;
        case CLEAR:
            return "PROXIMITY: CLEAR";
            break;
        case FULLCLEAR:
            return "SYSTEM STATE: CLEAR";
            break;
        case NEAR_OBJECT:
            return "SYSTEM STATE: NEAR OBJECT"; 
            break;
        case OBSTACLE_WHILE_MOVING:
            return "SYSTEM STATE: OBSTACLE WHILE MOVING"; 
            break;
        case VIBRATION_OR_IMPACT:
            return "SYSTEM STATE: VIBRATION OR IMPACT";
            break;
        case SENSOR_FAULT:
            return "SYSTEM STATE: SENSOR FAULT";
            break;
    }
    return "INVALID"; 
}

void tof_data_retrieval(void *pvParams) {
    mixed_sensor_data_t sensor_data; 
    vl6180_sample_t tof_sample;
    while (1) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(200));
        vl6180_read_distance_mm(&tof_sample);
        sensor_data.message_type = TOF;
        sensor_data.data.tof = tof_sample;   
        if (sensor_data.data.tof.distance < CRITICAL_THRESH) {
            sensor_data.state = CRITICAL; 
        } else if (sensor_data.data.tof.distance < CLOSE_THRESH) {
            sensor_data.state = CLOSE; 
        } else if (sensor_data.data.tof.distance >= CLOSE_THRESH && sensor_data.data.tof.distance < CLEAR_THRESH) {
            sensor_data.state = CLEAR; 
        } else {
            sensor_data.state = INVALID; 
        }
        sensor_data.data.tof.timestamp = xTaskGetTickCount() * 1000 / configTICK_RATE_HZ; 
        xQueueSend(sensor_queue, &sensor_data, pdMS_TO_TICKS(200)); 
        xEventGroupSetBits(sensor_group, TOF_BIT); 
    }
}

void accel_data_retrieval(void* pvParams) {
    mixed_sensor_data_t sensor_data; 
    adxl345_sample_t accel_sample;
    while (1) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(200)); 
        bool read_success = adxl345_read_xyz(&accel_sample); 
        if (!read_success) {
            sensor_data.state = INVALID_;
        } else {
            sensor_data.message_type = ACCEL;
            sensor_data.data.accel = accel_sample;
            if (sensor_data.data.accel.delta <= STABLE_THRESH) {
                sensor_data.state = STABLE;
            } else if (sensor_data.data.accel.delta <= MOVING_THRESH) {
                sensor_data.state = MOVING; 
            } else if (sensor_data.data.accel.delta < SHAKING_THRESH) {
                sensor_data.state = SHAKING;
            } else {
                sensor_data.state = INVALID_;
            }
            sensor_data.data.accel.timestamp = xTaskGetTickCount() * 1000 / configTICK_RATE_HZ; 
            xQueueSend(sensor_queue, &sensor_data, pdMS_TO_TICKS(200)); 
            xEventGroupSetBits(sensor_group, ACCEL_BIT); 
        }
    }
}

system_state get_system_state(int tof_state, int accel_state) {
    if (tof_state == CLEAR && accel_state == STABLE) {
        return FULLCLEAR; 
    } else if (tof_state == INVALID || accel_state == INVALID_) {
        return SENSOR_FAULT; 
    } else if ((tof_state == CLOSE || tof_state == CRITICAL) && accel_state == MOVING) {
        return OBSTACLE_WHILE_MOVING;
    } else if (tof_state == CLOSE || tof_state == CRITICAL) {
        return NEAR_OBJECT;
    } else if (accel_state == SHAKING) {
        return VIBRATION_OR_IMPACT;
    } else {
        return SENSOR_FAULT;
    }
}   

void data_processing(void *pvParams) {
    mixed_sensor_data_t data;
    tof_state _tof_state = INVALID;
    accel_state _accel_state = INVALID_;
    char buffer[128];
    while (1) {
        xQueueReceive(sensor_queue, &data, portMAX_DELAY); 
        if (data.message_type == ACCEL) {
            snprintf(buffer, 128, "[%u ms] ACCEL x: %d y: %d z: %d | %s\r\n", data.data.accel.timestamp, data.data.accel.accel_x, data.data.accel.accel_y, data.data.accel.accel_z,  get_label(data.state));
            _accel_state = data.state;
        } else if (data.message_type == TOF) {
            snprintf(buffer, 128, "[%u ms] TOF distance: %d mm | %s\r\n", data.data.tof.timestamp, data.data.tof.distance, get_label(data.state));
            _tof_state = data.state;
        }
        uart_outstring(buffer); 
        snprintf(buffer, 128, "[%u ms] System State: %s\r\n", xTaskGetTickCount() * 1000 / configTICK_RATE_HZ, get_label(get_system_state(_tof_state, _accel_state))); 
        uart_outstring(buffer); 
        xEventGroupSetBits(sensor_group, PROCESSING_BIT); 
    }
}

void health_monitor(void* pvParams) {
    char buffer[128];
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
        uint16_t tof_watermark = uxTaskGetStackHighWaterMark(tof_task_handle);
        uint16_t accel_watermark = uxTaskGetStackHighWaterMark(accel_task_handle);
        uint16_t processing_watermark = uxTaskGetStackHighWaterMark(processing_task_handle);
        xEventGroupSetBits(sensor_group, HEALTH_BIT); 
        snprintf(buffer, 128, "\r\n[HEALTH] Stack space free:\r\nTOF: %u\r\nAccel: %u\r\nProcessing: %u\r\n", tof_watermark, accel_watermark, processing_watermark); 
        uart_outstring(buffer);
        snprintf(buffer, 128, "[HEALTH] Heap usage: %d\r\n", xPortGetFreeHeapSize()); 
        uart_outstring(buffer); 
        snprintf(buffer, 128, "[HEALTH] Uptime: %d\r\n", xTaskGetTickCount() * 1000 / configTICK_RATE_HZ); 
        uart_outstring(buffer);  
        snprintf(buffer, 128, "[HEALTH] Queue usage: %d free spaces", (int)uxQueueSpacesAvailable(sensor_queue)); 
        uart_outstring(buffer);  
        taskYIELD();
    }   
}

void watchdog_task(void *pvParams) {
    while (1) {
        EventBits_t bits = xEventGroupWaitBits(sensor_group, REQUIRED_BITS, pdTRUE, pdTRUE, pdMS_TO_TICKS(1000));
        if ((bits & REQUIRED_BITS) == REQUIRED_BITS) {
            iwdg_reload();
        } else {
            xEventGroupClearBits(sensor_group, REQUIRED_BITS); 
            uart_outstring("WATCHDOG RESET!\r\n"); 
        }
    }
}

void health_timer_callback(TimerHandle_t xTimer) {
    xTaskNotifyGive(health_task_handle); 
}

int main(void) {
    PLL_Init();
    led2_Init();
    uart_Init();
    i2c_init();
    iwdg_init();
    periodic_timer_init(PERIOD_100MS); 
    // queue with 8 capacity accomodates delays in processing task without losing the sample
    sensor_queue = xQueueCreate(8, sizeof(mixed_sensor_data_t)); 
    sensor_group = xEventGroupCreate();
    xTaskCreate(tof_data_retrieval, "tof task", 512, NULL, 7, &tof_task_handle);
    xTaskCreate(accel_data_retrieval, "accel task", 512, NULL, 7, &accel_task_handle);
    xTaskCreate(data_processing, "processing task", 512,NULL, 5, &processing_task_handle);
    xTaskCreate(health_monitor, "health task", 128,NULL, 3, &health_task_handle);
    xTaskCreate(watchdog_task, "watchdog task", 256,NULL, 8, NULL);
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
    TimerHandle_t health_timer = xTimerCreate(
        "health monitor timer", 
        pdMS_TO_TICKS(500), 
        pdTRUE, 
        NULL, 
        health_timer_callback
    ); 
    xTimerStart(health_timer, 0); 
    iwdg_start();
    vTaskStartScheduler();
    while (1) {

    } 
}