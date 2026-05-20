#ifndef ADXL345_H
#define ADXL345_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum RANGE {
    G2,
    G4,
    G8,
    G16
} RANGE;

typedef struct adxl345_sample_t {
    int16_t accel_x; 
    int16_t accel_y;
    int16_t accel_z;
    int16_t mag_sq;
    int16_t mag_sq_prev;
    int16_t delta;
    uint32_t timestamp;
} adxl345_sample_t;

// pull cs low (motorola mode), send command byte, send dummy byte, return received byte
uint8_t adxl345_read_reg(uint8_t reg); 
// pull cs low, send command byte, send value byte, release cs
void adxl345_write_reg(uint8_t reg, uint8_t value);
void adxl345_read_multi(uint8_t start_reg, uint8_t *buffer, size_t len);  
bool adxl345_init(); 
bool adxl345_read_xyz(adxl345_sample_t *sample); 
void adxl345_set_range(RANGE range);
void adxl345_start_measure();
#endif

/* 
SPI: 

CS idles high (motorola)
MCU pulls CS low to initiate communication
To read: MCU sends command byte + dummy byte on MOSI and sensor sends on MISO
To write: MCU sends command byte + data byte on MOSI and sensor sends on MISO
MCU releases

*/