#ifndef VL6180_H
#define VL6180_H

#include "stdbool.h"
#include "stdint.h"
#include "FreeRTOS.h"
#include "task.h"

#define IDENTIFICATION__MODEL_ID 0x00
#define SYSRANGE__START 0x018
#define SYSRANGE__THRESH_HIGH 0x019
#define SYSRANGE__THRESH_LOW 0x1A
#define SYSRANGE__INTERMEASUREMENT_PERIOD 0x1B
#define SYSTEM__MODE_GPIO1 0x011
#define RESULT__RANGE_VAL 0x062
#define RESULT__RANGE_STATUS 0x04D
#define SYSTEM__INTERRUPT_CONFIG_GPIO 0x014
#define SYSTEM__INTERRUPT_CLEAR 0x015
#define PERIOD_1S_CYCLES 8084000
#define VL6180_I2C_ADDR 0x29
#define MODEL_ID 0xB4

typedef struct vl6180_sample_t {
    uint32_t timestamp;
    uint16_t distance;
    bool valid;
} vl6180_sample_t; 

typedef enum MODE {
    CONTINUOUS,
    SINGLESHOT
} MODE;


// check sensor identity
// configure measurement mode
// configure ranging mode
// start measurement mode
bool vl6180_init();

// return true if alive
bool vl6180_alive(); 

// returns distance in mm
void vl6180_read_distance_mm(vl6180_sample_t *sample);

// set mode to continuous
void vl6180_set_continuous(TaskHandle_t _acquisition_task_handle);

// set mode to singleshot
void vl6180_set_singleshot();

// clear intterupt via i2c call
void vl6180_clear_interrupt();


#endif