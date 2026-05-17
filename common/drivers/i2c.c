#include "i2c.h"
#include "stm32f401xe.h"
#include <stdint.h>

// can use I2C1 on APB1 on PB8 (SCL) and PB9 (SDA)

/* 
The following is the required sequence in controller mode.
•Program the peripheral input clock in I2C_CR2 register in order to generate correct
timings
•Configure the clock control registers
•Configure the rise time register
•Program the I2C_CR1 register to enable the peripheral
•Set the START bit in the I2C_CR1 register to generate a Start condition
*/

void i2c_init() {
    // turn on clock for GPIOB
    RCC->AHB1ENR |= (1 << 1);
    // turn on clock for I2C1 (APB1)
    RCC->APB1ENR |= (1 << 21); 
    // configure pins
    // AF mode for pins 8 and 9
    GPIOB->MODER &= ~(0b11 << 16);
    GPIOB->MODER |= (0b10 << 16);
    GPIOB->MODER &= ~(0b11 << 18); 
    GPIOB->MODER |= (0b10 << 18); 
    // add internal pullups for now
    GPIOB->PUPDR &= ~(0b11 << 16);
    GPIOB->PUPDR &= ~(0b11 << 18);
    GPIOB->PUPDR |= (0b01 << 16);
    GPIOB->PUPDR |= (0b01 << 18);
    // AF04 for SDA and SCL
    GPIOB->AFR[1] &= ~(0xF << 0); 
    GPIOB->AFR[1] |= (0b100 << 0); 
    GPIOB->AFR[1] &= ~(0xF << 4); 
    GPIOB->AFR[1] |= (0b100 << 4); 
    // open drain for SDA and SCL
    GPIOB->OTYPER |= (1 << 8); 
    GPIOB->OTYPER |= (1 << 9); 
    // freq = 40 mhz
    I2C1->CR2 &= ~(0b101000 << 0);
    I2C1->CR2 |= (0b101000 << 0); 
    // CCR = 40.21 Mhz / (2 * 100khz) = 201
    I2C1->CCR &= ~(0xC9 << 0); 
    I2C1->CCR |= (0xC9 << 0); 
    // Rise time: (1000ns / T) + 1
    I2C1->TRISE &= ~(0x29 << 0);
    I2C1->TRISE |= (0x29 << 0);
    // peripheral enable
    I2C1->CR1 |= (0b1 << 0);
    // acknowledgement enable
    I2C1->CR1 |= (0b1 << 10);
    // 7 bit address
    I2C1->OAR1 &= ~(0b1 << 15); 
}

/* 
1-byte write: 

Master generates START condition (SDA pulled low hwile SCL high)
Master sends 7 Address bits + 1 w bit = 1 byte
ACK from receiver to indicate address reception
Master sends Data byte
ACK from receiver to indicate data reception
Master generates STOP condition
*/

void i2c_write(uint8_t address_byte, uint8_t data_byte) {
    // while bus busy
    while (I2C1->SR2 & (1 << 1)) {}
    // set start bit
    I2C1->CR1 |= (0b1 << 8);
    // wait for start condition to be generated
    while ((I2C1->SR1 & (1 << 0)) == 0) {}
    // send address + w bit to SDA
    I2C1->DR = ((address_byte << 1) << 0);
    // read sr1 and sr2 to clear ADDR
    while ((I2C1->SR1 & (1 << 1)) == 0) {}
    volatile uint32_t sr2 = I2C1->SR1;
    sr2 = I2C1->SR2;
    // wait for transmit data register to be empty
    while ((I2C1->SR1 & (1 << 7)) == 0) {}
    // send data byte to SDA 
    I2C1->DR = (data_byte << 0);
    // wait for transfer to be finished
    while ((I2C1->SR1 & (1 << 2)) == 0) {}
    // set stop bit
    I2C1->CR1 |= (0b1 << 9); 
}   

/* 
1-byte read: 

Master generates START condition (SDA pulled low while SCL high)
Master sends 7 address bits + 1 r bit
Slave sends ACK to indicate address reception
Slave sends data on SDA
controller sends NACK to indicate data was read
STOP condition
*/

void i2c_read(uint8_t address_byte, uint8_t *buffer) {
    // while bus busy
    while (I2C1->SR2 & (1 << 1)) {}
    // disable ACK
    I2C1->CR1 &= ~(0b1 << 10);
    // set start bit
    I2C1->CR1 |= (0b1 << 8);
    // wait for start condition to be generated
    while ((I2C1->SR1 & (1 << 0)) == 0) {}
    // send address + r bit to SDA
    I2C1->DR = (((address_byte << 1) | 0b1) << 0);
    // read sr1 and sr2 to clear ADDR
    while ((I2C1->SR1 & (1 << 1)) == 0) {}
    volatile uint32_t sr2 = I2C1->SR1;
    sr2 = I2C1->SR2;
    // set stop bit
    I2C1->CR1 |= (0b1 << 9); 
    // wait for receive data register to be full
    while ((I2C1->SR1 & (1 << 6)) == 0) {}
    // sample byte
    *buffer = (uint8_t)I2C1->DR; 
    // reenable ACK
    I2C1->CR1 |= (0b1 << 10);
}

void i2c_write_reg(uint8_t address_byte, uint8_t reg_addr, uint8_t data_byte) {
    // while bus busy
    while (I2C1->SR2 & (1 << 1)) {}
    // set start bit
    I2C1->CR1 |= (0b1 << 8);
    // wait for start condition to be generated
    while ((I2C1->SR1 & (1 << 0)) == 0) {}
    // send address + w bit to SDA
    I2C1->DR = ((address_byte << 1) << 0);
    // read sr1 and sr2 to clear ADDR
    while ((I2C1->SR1 & (1 << 1)) == 0) {}
    volatile uint32_t sr2 = I2C1->SR1;
    sr2 = I2C1->SR2;
    // wait for transmit data register to be empty
    while ((I2C1->SR1 & (1 << 7)) == 0) {}
    // send data byte to SDA 
    I2C1->DR = (reg_addr << 0);
    // wait for transfer to be finished
    while ((I2C1->SR1 & (1 << 2)) == 0) {}
    // wait for transmit data register to be empty
    while ((I2C1->SR1 & (1 << 7)) == 0) {}
    // send data byte to SDA 
    I2C1->DR = (data_byte << 0);
    // wait for transfer to be finished
    while ((I2C1->SR1 & (1 << 2)) == 0) {}
    // set stop bit
    I2C1->CR1 |= (0b1 << 9); 
}

void i2c_read_reg(uint8_t address_byte, uint8_t reg_addr, uint8_t *buffer) {
    // while bus busy
    while (I2C1->SR2 & (1 << 1)) {}
    // set start bit
    I2C1->CR1 |= (0b1 << 8);
    // wait for start condition to be generated
    while ((I2C1->SR1 & (1 << 0)) == 0) {}
    // send address + w bit to SDA
    I2C1->DR = ((address_byte << 1) << 0);
    // read sr1 and sr2 to clear ADDR
    while ((I2C1->SR1 & (1 << 1)) == 0) {}
    volatile uint32_t sr2 = I2C1->SR1;
    sr2 = I2C1->SR2;
    // wait for transmit data register to be empty
    while ((I2C1->SR1 & (1 << 7)) == 0) {}
    // send data byte to SDA 
    I2C1->DR = (reg_addr << 0);
    // wait for transfer to be finished
    while ((I2C1->SR1 & (1 << 2)) == 0) {}
     // set start bit
    I2C1->CR1 |= (0b1 << 8);
    // wait for start condition to be generated
    while ((I2C1->SR1 & (1 << 0)) == 0) {}
    // disable ACK
    I2C1->CR1 &= ~(0b1 << 10);
    // send address + r bit to SDA
    I2C1->DR = (((address_byte << 1) | 0b1) << 0);
    // read sr1 and sr2 to clear ADDR
    while ((I2C1->SR1 & (1 << 1)) == 0) {}
    sr2 = I2C1->SR1;
    sr2 = I2C1->SR2;
    // set stop bit
    I2C1->CR1 |= (0b1 << 9); 
    // wait for receive data register to be full
    while ((I2C1->SR1 & (1 << 6)) == 0) {}
    // sample byte
    *buffer = (uint8_t)I2C1->DR; 
    // reenable ACK
    I2C1->CR1 |= (0b1 << 10);
}

void i2c_read_regs(uint8_t address_byte, uint8_t start_reg, uint8_t *buffer, uint8_t length) {
     // while bus busy
    while (I2C1->SR2 & (1 << 1)) {}
    // set start bit
    I2C1->CR1 |= (0b1 << 8);
    // wait for start condition to be generated
    while ((I2C1->SR1 & (1 << 0)) == 0) {}
    // send address + w bit to SDA
    I2C1->DR = ((address_byte << 1) << 0);
    // read sr1 and sr2 to clear ADDR
    while ((I2C1->SR1 & (1 << 1)) == 0) {}
    volatile uint32_t sr2 = I2C1->SR1;
    sr2 = I2C1->SR2;
    // wait for transmit data register to be empty
    while ((I2C1->SR1 & (1 << 7)) == 0) {}
    // send data byte to SDA 
    I2C1->DR = (start_reg << 0);
    // wait for transfer to be finished
    while ((I2C1->SR1 & (1 << 2)) == 0) {}
     // set start bit
    I2C1->CR1 |= (0b1 << 8);
    // wait for start condition to be generated
    while ((I2C1->SR1 & (1 << 0)) == 0) {}
    // disable ACK
    I2C1->CR1 &= ~(0b1 << 10);
    // send address + r bit to SDA
    I2C1->DR = (((address_byte << 1) | 0b1) << 0);
    // read sr1 and sr2 to clear ADDR
    while ((I2C1->SR1 & (1 << 1)) == 0) {}
    sr2 = I2C1->SR1;
    sr2 = I2C1->SR2;
    // reenable ACK
    I2C1->CR1 |= (0b1 << 10); 
    int i = 0;
    for (i = 0; i < length; i++) {
        if (i == length - 1) {
            // set stop bit
            I2C1->CR1 |= (0b1 << 9);
            // disable ACK
            I2C1->CR1 &= ~(0b1 << 10);
        }
        // wait for receive data register to be full
        while ((I2C1->SR1 & (1 << 6)) == 0) {}
        // sample byte
        buffer[i] = (uint8_t)I2C1->DR;
    }
    // reenable ACK
    I2C1->CR1 |= (0b1 << 10);
}