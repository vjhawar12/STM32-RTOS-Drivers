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
#include "spi.h"
#include "event_groups.h"
#include "uart.h"

/*
 * Vehicle telemetry application.
 *
 * This is the product-level entry point for the repository. The original
 * learning exercises remain under labs/ and can still be selected through
 * the root CMake configuration.
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

void periodic_timer_isr(void) {
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(accel_task_handle, &pxHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
}

char *get_label(int value) {
    switch (value) {
        case STABLE: return "MOTION: STABLE";
        case MOVING: return "MOTION: MOVING";
        case SHAKING: return "MOTION: SHAKING";
        case INVALID_: return "MOTION: INVALID";
        case CLOSE: return "PROXIMITY: CLOSE";
        case CRITICAL: return "PROXIMITY: CRITICAL";
        case INVALID: return "PROXIMITY: INVALID";
        case CLEAR: return "PROXIMITY: CLEAR";
        case FULLCLEAR: return "SYSTEM STATE: CLEAR";
        case NEAR_OBJECT: return "SYSTEM STATE: NEAR OBJECT";
        case OBSTACLE_WHILE_MOVING: return "SYSTEM STATE: OBSTACLE WHILE MOVING";
        case VIBRATION_OR_IMPACT: return "SYSTEM STATE: VIBRATION OR IMPACT";
        case SENSOR_FAULT: return "SYSTEM STATE: SENSOR FAULT";
        default: return "INVALID";
    }
}

void tof_data_retrieval(void *pvParams) {
    (void)pvParams;
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
        } else if (sensor_data.data.tof.distance >= CLOSE_THRESH &&
                   sensor_data.data.tof.distance < CLEAR_THRESH) {
            sensor_data.state = CLEAR;
        } else {
            sensor_data.state = INVALID;
        }

        sensor_data.data.tof.timestamp = xTaskGetTickCount() * 1000 / configTICK_RATE_HZ;
        xQueueSend(sensor_queue, &sensor_data, pdMS_TO_TICKS(200));
        xEventGroupSetBits(sensor_group, TOF_BIT);
    }
}

void accel_data_retrieval(void *pvParams) {
    (void)pvParams;
    mixed_sensor_data_t sensor_data;
    adxl345_sample_t accel_sample = {0};

    while (1) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(200));
        bool read_success = adxl345_read_xyz(&accel_sample);

        sensor_data.message_type = ACCEL;
        if (!read_success) {
            sensor_data.state = INVALID_;
        } else {
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
        }

        xQueueSend(sensor_queue, &sensor_data, pdMS_TO_TICKS(200));
        xEventGroupSetBits(sensor_group, ACCEL_BIT);
    }
}

system_state get_system_state(int tof_value, int accel_value) {
    if (tof_value == CLEAR && accel_value == STABLE) {
        return FULLCLEAR;
    } else if (tof_value == INVALID || accel_value == INVALID_) {
        return SENSOR_FAULT;
    } else if ((tof_value == CLOSE || tof_value == CRITICAL) && accel_value == MOVING) {
        return OBSTACLE_WHILE_MOVING;
    } else if (tof_value == CLOSE || tof_value == CRITICAL) {
        return NEAR_OBJECT;
    } else if (accel_value == SHAKING) {
        return VIBRATION_OR_IMPACT;
    }

    return SENSOR_FAULT;
}

void data_processing(void *pvParams) {
    (void)pvParams;
    mixed_sensor_data_t data;
    tof_state current_tof_state = INVALID;
    accel_state current_accel_state = INVALID_;
    char buffer[128];

    while (1) {
        xQueueReceive(sensor_queue, &data, portMAX_DELAY);

        if (data.message_type == ACCEL) {
            snprintf(buffer, sizeof(buffer),
                     "[%u ms] ACCEL x: %d y: %d z: %d | %s\r\n",
                     data.data.accel.timestamp,
                     data.data.accel.accel_x,
                     data.data.accel.accel_y,
                     data.data.accel.accel_z,
                     get_label(data.state));
            current_accel_state = data.state;
        } else {
            snprintf(buffer, sizeof(buffer),
                     "[%u ms] TOF distance: %d mm | %s\r\n",
                     data.data.tof.timestamp,
                     data.data.tof.distance,
                     get_label(data.state));
            current_tof_state = data.state;
        }

        uart_outstring(buffer);
        snprintf(buffer, sizeof(buffer),
                 "[%u ms] System State: %s\r\n",
                 xTaskGetTickCount() * 1000 / configTICK_RATE_HZ,
                 get_label(get_system_state(current_tof_state, current_accel_state)));
        uart_outstring(buffer);
        xEventGroupSetBits(sensor_group, PROCESSING_BIT);
    }
}

void health_monitor(void *pvParams) {
    (void)pvParams;
    char buffer[128];

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint16_t tof_watermark = uxTaskGetStackHighWaterMark(tof_task_handle);
        uint16_t accel_watermark = uxTaskGetStackHighWaterMark(accel_task_handle);
        uint16_t processing_watermark = uxTaskGetStackHighWaterMark(processing_task_handle);
        xEventGroupSetBits(sensor_group, HEALTH_BIT);

        snprintf(buffer, sizeof(buffer),
                 "\r\n[HEALTH] Stack space free:\r\nTOF: %u\r\nAccel: %u\r\nProcessing: %u\r\n",
                 tof_watermark, accel_watermark, processing_watermark);
        uart_outstring(buffer);

        snprintf(buffer, sizeof(buffer), "[HEALTH] Heap free: %u\r\n",
                 (unsigned)xPortGetFreeHeapSize());
        uart_outstring(buffer);

        snprintf(buffer, sizeof(buffer), "[HEALTH] Uptime: %u ms\r\n",
                 (unsigned)(xTaskGetTickCount() * 1000 / configTICK_RATE_HZ));
        uart_outstring(buffer);

        snprintf(buffer, sizeof(buffer), "[HEALTH] Queue free spaces: %u\r\n",
                 (unsigned)uxQueueSpacesAvailable(sensor_queue));
        uart_outstring(buffer);
    }
}

void watchdog_task(void *pvParams) {
    (void)pvParams;

    while (1) {
        EventBits_t bits = xEventGroupWaitBits(
            sensor_group,
            REQUIRED_BITS,
            pdTRUE,
            pdTRUE,
            pdMS_TO_TICKS(1000));

        if ((bits & REQUIRED_BITS) == REQUIRED_BITS) {
            iwdg_reload();
        } else {
            xEventGroupClearBits(sensor_group, REQUIRED_BITS);
            uart_outstring("WATCHDOG CHECK-IN FAILED\r\n");
        }
    }
}

void health_timer_callback(TimerHandle_t xTimer) {
    (void)xTimer;
    xTaskNotifyGive(health_task_handle);
}

int main(void) {
    PLL_Init();
    led2_Init();
    uart_Init();
    i2c_init();
    spi_init();
    iwdg_init();
    periodic_timer_init(PERIOD_100MS);

    sensor_queue = xQueueCreate(8, sizeof(mixed_sensor_data_t));
    sensor_group = xEventGroupCreate();

    xTaskCreate(tof_data_retrieval, "tof", 512, NULL, 7, &tof_task_handle);
    xTaskCreate(accel_data_retrieval, "accel", 512, NULL, 7, &accel_task_handle);
    xTaskCreate(data_processing, "processing", 512, NULL, 5, &processing_task_handle);
    xTaskCreate(health_monitor, "health", 128, NULL, 3, &health_task_handle);
    xTaskCreate(watchdog_task, "watchdog", 256, NULL, 8, NULL);

    uart_enable_rx_interrupt();

    if (!adxl345_init()) {
        uart_outstring("Could not reach ADXL345 sensor!\r\n");
    }

    bool tof_alive = vl6180_alive();
    while (!tof_alive) {
        uart_outstring("Could not reach TOF sensor!\r\n");
        toggle_led2(PERIOD_1S_CYCLES);
        tof_alive = vl6180_alive();
    }

    vl6180_init();
    vl6180_set_continuous(tof_task_handle);
    periodic_timer_start();

    TimerHandle_t health_timer = xTimerCreate(
        "health timer",
        pdMS_TO_TICKS(500),
        pdTRUE,
        NULL,
        health_timer_callback);

    xTimerStart(health_timer, 0);
    iwdg_start();
    vTaskStartScheduler();

    while (1) {}
}
