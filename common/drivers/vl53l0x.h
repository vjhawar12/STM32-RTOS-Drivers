#ifndef VL53L0X_H
#define VL53L0X_H

#include "stdbool.h"
#include "stdint.h"

typedef struct vl53l0x_sample_t {
    uint32_t timestamp;
    uint16_t distance;
    bool valid;
} vl53l0x_sample_t; 

typedef enum MODE {
    CONTINUOUS,
    SINGLESHOT
} MODE;


// check sensor identity
// configure measurement mode
// configure ranging mode
// start measurement mode
bool vl53l0x_init();

// return true if alive
bool vl53l0x_alive(); 

// returns distance in mm
void vl53l0x_read_distance_mm(vl53l0x_sample_t *sample);



#endif