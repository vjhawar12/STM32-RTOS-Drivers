#include "adxl345.h"
#include "spi.h"
#include <stdint.h>
#include <stdbool.h>
#include "math.h"

// device ID
#define DEVID 0x00
// offset accceleration registers
#define OFSX 0x1E
#define OFSY 0x1F
#define OFSZ 0x20
// SPI configuration register
#define DATA_FORMAT 0x31
#define SPI_BIT 6
#define FULL_RES_BIT 3
// acceleration data registers
// x-axis data 0
#define DATAX0 0x32
// x-axis data 1
#define DATAX1 0x33
// Y-axis data 0
#define DATAY0 0x34
// Y-axis data 1
#define DATAY1 0x35
// Z-axis data 0
#define DATAZ0 0x36
// Z-axis data 1
#define DATAZ1 0x37
// power register
#define POWER_CTL 0x2D
#define MEASURE_BIT 3

#define SPI_WRITE ~(0b1 << 7)
#define SPI_READ  (0b1 << 7)
#define SINGLE_BYTE ~(0b1 << 6)
#define MULTI_BYTE (0b1 << 6)
#define SPI_DUMMY_BYTE 0xFF


bool adxl345_init() {
    uint8_t devid = adxl345_read_reg(DEVID); 
    if (devid != 0xE5) {
        return false;
    }
    // setting 4 wire mode
    uint8_t value = adxl345_read_reg(DATA_FORMAT); 
    value &= ~(1 << SPI_BIT); 
    adxl345_write_reg(DATA_FORMAT, value);
    // set range
    adxl345_set_range(G8); 
    // set full resolution
    value = adxl345_read_reg(DATA_FORMAT); 
    value &= ~(1 << FULL_RES_BIT); 
    adxl345_write_reg(DATA_FORMAT, value);
    // starting measurement  
    adxl345_start_measure();
    return true;
}

void adxl345_start_measure() {
    uint8_t value = adxl345_read_reg(POWER_CTL); 
    value |= (1 << MEASURE_BIT); 
    adxl345_write_reg(POWER_CTL, value); 
}

void adxl345_set_range(RANGE range) {
    uint8_t reg = adxl345_read_reg(DATA_FORMAT); 
    reg &= ~(0b11 << 0); 
    if (range != G2) {
        reg |= (range << 0); 
    }
    adxl345_write_reg(DATA_FORMAT, reg); 
}

void adxl345_write_reg(uint8_t reg, uint8_t data_byte) {
    uint8_t cmd_byte = reg & SINGLE_BYTE & SPI_WRITE; 
    adxl_cs_low(); 
    spi_transfer_byte(cmd_byte); 
    spi_transfer_byte(data_byte); 
    adxl_cs_high(); 
}

uint8_t adxl345_read_reg(uint8_t reg) {
    uint8_t ret_value;
    uint8_t cmd_byte = (reg & SINGLE_BYTE) | SPI_READ; 
    adxl_cs_low();
    ret_value = spi_transfer_byte(cmd_byte); 
    ret_value = spi_transfer_byte(SPI_DUMMY_BYTE); 
    adxl_cs_high();
    return ret_value;
}

void adxl345_read_multi(uint8_t start_reg, uint8_t *buffer, size_t len) {
    int i = 0;
    adxl_cs_low();
    uint8_t cmd_byte = (start_reg | MULTI_BYTE) | SPI_READ; 
    spi_transfer_byte(cmd_byte); 
    for (i = 0; i < len; i++) {
        buffer[i] = spi_transfer_byte(SPI_DUMMY_BYTE); 
    }
    adxl_cs_high();
}

bool adxl345_read_xyz(adxl345_sample_t *sample) {
    if (sample == NULL) {
        return false;
    }
    uint8_t accel_buffer[6];
    adxl345_read_multi(DATAX0, accel_buffer, 6); 
    uint16_t accel_x = ((uint16_t)accel_buffer[1] << 8) | accel_buffer[0]; 
    uint16_t accel_y = ((uint16_t)accel_buffer[3] << 8) | accel_buffer[2]; 
    uint16_t accel_z = ((uint16_t)accel_buffer[5] << 8) | accel_buffer[4]; 
    sample->accel_x = (int16_t)accel_x; 
    sample->accel_y = (int16_t)accel_y;
    sample->accel_z = (int16_t)accel_z;
    sample->mag_sq_prev = sample->mag_sq; 
    sample->mag_sq = sample->accel_x * sample->accel_x + sample->accel_y * sample->accel_y + sample->accel_z * sample->accel_z; 
    sample->delta = abs(sample->mag_sq - sample->mag_sq_prev); 
    return true;
}