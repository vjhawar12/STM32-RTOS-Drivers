#ifndef VL6180_H
#define VL6180_H

#include "stdbool.h"
#include "stdint.h"

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



#endif