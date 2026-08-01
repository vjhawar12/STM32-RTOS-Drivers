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
 *
 * TOF and Accelerometer work together to quantify amount of shock experienced
 * TOF: How much did the suspension compress, and did it bottom out
 * Accelerometer: Many Gs of force did the driver experience
 */

#define PERIOD_1S_CYCLES 8084000
#define PERIOD_100MS 1000
#define BOTTOMED_OUT_LOWER 0
#define BOTTOMED_OUT_UPPER 8
#define NORMAL_LOWER 8
#define NORMAL_UPPER 92
#define TOPPED_OUT_LOWER 92
#define TOPPED_OUT_UPPER 100
#define IMPACT_NORMAL_LOWER 0
#define IMPACT_NORMAL_UPPER 3
#define IMPACT_MODERATE_LOWER 3
#define IMPACT_MODERATE_UPPER 6
#define IMPACT_SEVERE_LOWER 6
#define IMPACT_SEVERE_UPPER 12
#define IMPACT_CRASH_LOWER 12
#define TOF_BIT (1 << 0)
#define ACCEL_BIT (1 << 1)
#define PROCESSING_BIT (1 << 2)
#define HEALTH_BIT (1 << 3)
#define REQUIRED_BITS (TOF_BIT | ACCEL_BIT | PROCESSING_BIT | HEALTH_BIT)

typedef enum message_type_t {
    ACCEL,
    TOF,
    SYS
} message_type_t;

typedef enum tof_state {
    SUS_TOPPED_OUT,
    SUS_BOTTOMED_OUT,
    SUS_NORMAL,
    SUS_UNKNOWN
} tof_state;

typedef enum accel_state {
    IMPACT_NORMAL,
    IMPACT_MODERATE,
    IMPACT_SEVERE,
    IMPACT_CRASH,
    IMPACT_UNKNOWN
} accel_state;

/*
 * ====================================================================================================
 *                                   SYSTEM STATE EVALUATION MATRIX
 * ====================================================================================================
 *  tof_state     | accel_state     | system_state        | Real-World Diagnosis
 * ---------------+-----------------+---------------------+--------------------------------------------
 *  NORMAL        | IMPACT_NORMAL   | SYS_NORMAL          | Driving smoothly on flat ground.
 *  TOPPED_OUT    | IMPACT_NORMAL   | SYS_AIRBORNE        | Launched off jump; suspension fully open.
 *  BOTTOMED_OUT  | IMPACT_MODERATE | SYS_HARD_LANDING    | Hard landing; 100% travel compressed.
 *  BOTTOMED_OUT  | IMPACT_SEVERE   | SYS_SEVERE_LANDING    | Severe landing; 100% travel compressed.
 *  NORMAL        | IMPACT_SEVERE   | SYS_OBSTACLE_IMPACT | Direct obstacle/tire hit without travel.
 *  ANY           | CRASH           | SYS_CRASH           | Critical collision / rollover event.
 *  UNKNOWN       | ANY             | SYS_FAULT           | Sensor reading invalid / I2C bus error.
 * ====================================================================================================
 */

typedef enum system_state {
    SYS_NORMAL,
    SYS_AIRBORNE,
    SYS_HARD_LANDING,
    SYS_SEVERE_LANDING,
    SYS_OBSTACLE_IMPACT,
    SYS_CRASH,
    SYS_FAULT,
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

char *get_label(int value, message_type_t message_type) {
    if (message_type == ACCEL) {
        switch (value) {
            case IMPACT_NORMAL: return "TYPICAL DRIVING VIBRATION";
            case IMPACT_MODERATE: return "HARSH BUMPS / HARD BRAKING";
            case IMPACT_SEVERE: return "HARD LANDING";
            case IMPACT_CRASH: return "MAJOR COLLISION(S)";
            case IMPACT_UNKNOWN: return "UNKNOWN ACCELEROMETER DATA";
            default: return "ACCEL INVALID";
        }
    } else if (message_type == TOF) {
        switch (value) {
            case SUS_TOPPED_OUT: return "SUSPENSION TOPPED OUT";
            case SUS_BOTTOMED_OUT: return "SUSPENSION BOTTOMED OUT";
            case SUS_NORMAL: return "SUSPENSION TRAVEL NORMAL";
            case SUS_UNKNOWN: return "SUSPENSION TRAVEL (TOF)UNKNOWN";
            default: return "TOF INVALID";
        }
    } else if (message_type == SYS) {
        switch (value) {
            case SYS_NORMAL: return "DRIVING SMOOTHLY";
            case SYS_AIRBORNE: return "LAUNCHED OFF JUMP; SUSPENSION FULLY OPEN";
            case SYS_HARD_LANDING: return "HARD LANDING; 100%% COMPRESSED";
            case SYS_SEVERE_LANDING: return "SEVERE LANDING; 100%% COMPRESSED";
            case SYS_OBSTACLE_IMPACT: return "DIRECT HIT WITHOUT TRAVEL";
            case SYS_CRASH: return "CRITICAL COLLISION / ROLLOVER";
            case SYS_FAULT: return "SENSOR READING INVALID / BUS FAULT";
            default: return "SYS INVALID";
        }
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
        if (sensor_data.data.tof.distance <= BOTTOMED_OUT_UPPER) {
            sensor_data.state = SUS_BOTTOMED_OUT;
        } else if (sensor_data.data.tof.distance <= NORMAL_UPPER) {
            sensor_data.state = SUS_NORMAL;
        } else if (sensor_data.data.tof.distance <= TOPPED_OUT_UPPER) {
            sensor_data.state = SUS_TOPPED_OUT;
        } else {
            sensor_data.state = SUS_UNKNOWN;
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
            sensor_data.state = IMPACT_UNKNOWN;
        } else {
            sensor_data.data.accel = accel_sample;
            if (sensor_data.data.accel.delta <= IMPACT_NORMAL_UPPER) {
                sensor_data.state = IMPACT_NORMAL;
            } else if (sensor_data.data.accel.delta <= IMPACT_MODERATE_UPPER) {
                sensor_data.state = IMPACT_MODERATE;
            } else if (sensor_data.data.accel.delta <= IMPACT_SEVERE_UPPER) {
                sensor_data.state = IMPACT_SEVERE;
            } else if (sensor_data.data.accel.delta > IMPACT_SEVERE_UPPER) {
                sensor_data.state = IMPACT_CRASH;
            } else {
                sensor_data.state = IMPACT_UNKNOWN;
            }
            sensor_data.data.accel.timestamp = xTaskGetTickCount() * 1000 / configTICK_RATE_HZ;
        }
        xQueueSend(sensor_queue, &sensor_data, pdMS_TO_TICKS(200));
        xEventGroupSetBits(sensor_group, ACCEL_BIT);
    }
}

system_state get_system_state(int tof_value, int accel_value) {
    if (tof_value == SUS_NORMAL && accel_value == IMPACT_NORMAL) {
        return SYS_NORMAL;
    } else if (tof_value == SUS_TOPPED_OUT || accel_value == IMPACT_NORMAL) {
        return SYS_AIRBORNE;
    } else if (tof_value == SUS_BOTTOMED_OUT && accel_value == IMPACT_MODERATE) {
        return SYS_HARD_LANDING;
    } else if (tof_value == SUS_BOTTOMED_OUT && accel_value == IMPACT_SEVERE) {
        return SYS_SEVERE_LANDING;
    } else if (tof_value == SUS_NORMAL && accel_value == IMPACT_SEVERE) {
        return SYS_OBSTACLE_IMPACT;
    } else if (accel_value == IMPACT_CRASH) {
        return SYS_CRASH;
    } else if (tof_value == SUS_UNKNOWN || accel_value == IMPACT_UNKNOWN) {
        return SYS_FAULT;
    }
}

void data_processing(void *pvParams) {
    (void)pvParams;
    mixed_sensor_data_t data;
    tof_state current_tof_state; 
    accel_state current_accel_state; 
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
                     get_label(data.state, ACCEL));
            current_accel_state = data.state;
        } else {
            snprintf(buffer, sizeof(buffer),
                     "[%u ms] TOF distance: %d mm | %s\r\n",
                     data.data.tof.timestamp,
                     data.data.tof.distance,
                     get_label(data.state, ACCEL));
            current_tof_state = data.state;
        }

        uart_outstring(buffer);
        snprintf(buffer, sizeof(buffer),
                 "[%u ms] System State: %s\r\n",
                 xTaskGetTickCount() * 1000 / configTICK_RATE_HZ,
                 get_label(get_system_state(current_tof_state, current_accel_state), SYS));
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
