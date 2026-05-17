#ifndef I2C_H
#define I2C_H

#include <stdbool.h>
#include <stdint.h>

void i2c_init();
void i2c_write(uint8_t address_byte, uint8_t data_byte);
void i2c_read(uint8_t address_byte, uint8_t *buffer);
void i2c_write_reg(uint8_t address_byte, uint8_t reg_addr, uint8_t data_byte);
void i2c_read_reg(uint8_t address_byte, uint8_t reg_addr, uint8_t *buffer);
void i2c_read_regs(uint8_t address_byte, uint8_t start_reg, uint8_t *buffer, uint8_t length);


#endif