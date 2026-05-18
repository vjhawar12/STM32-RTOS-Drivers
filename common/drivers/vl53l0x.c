#include "vl53l0x.h"
#include "i2c.h"
#include "stdbool.h"
#include "stm32f401xe.h"
#include "platform.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#define IDENTIFICATION__MODEL_ID 0x00
#define SYSRANGE__START 0x018
#define SYSRANGE__THRESH_HIGH 0x019
#define SYSRANGE__THRESH_LOW 0x1A
#define SYSRANGE__INTERMEASUREMENT_PERIOD 0x1B
#define RESULT__RANGE_VAL 0x062
#define RESULT__RANGE_STATUS 0x04D
#define SYSTEM__INTERRUPT_CONFIG_GPIO 0x014
#define PERIOD_1S_CYCLES 8084000
#define DEVID 0xB4

MODE mode;

bool vl53l0x_alive() {
    uint8_t id = 0; 
    i2c_read_reg(DEVID, IDENTIFICATION__MODEL_ID, &id); 
    return id == DEVID; 
}


/* 
Set GPIO0 to 0 (PA3)
Set GPIO0 to 1
Set GPIO1 to 1 for interrupts (PA4) (open drain; 47 kohm)
Wait 1 ms
now it is possible to configure the device and start single-shot or continuous
ranging operation
*/
bool vl53l0x_init() {
    GPIOA->ODR &= ~(1 << 3); 
    burn_cycles(PERIOD_1S_CYCLES / 1000); 
    GPIOA->ODR |= (1 << 3); 
    GPIOA->ODR |= (1 << 4);
    burn_cycles(PERIOD_1S_CYCLES / 1000); 
    i2c_write_reg(DEVID, SYSRANGE__THRESH_HIGH, 0xFF); 
    i2c_write_reg(DEVID, SYSRANGE__THRESH_LOW, 0x05); 
    i2c_write_reg(DEVID, SYSRANGE__INTERMEASUREMENT_PERIOD, 0x0A); 
    i2c_write_reg(DEVID, SYSRANGE__START, SYSRANGE__START & ~(0b1 << 1)); 
    i2c_write_reg(DEVID, SYSRANGE__START, SYSRANGE__START | (0b1 << 0)); 
    uint8_t error = 0xF;
    i2c_read_reg(DEVID, RESULT__RANGE_STATUS, &error); 
    return error == 0;
}

void vl3l0x_set_mode(MODE _mode) {
    mode = _mode;
    if (mode == CONTINUOUS) {
        i2c_write_reg(DEVID, SYSRANGE__START, SYSRANGE__START | (0b1 << 1)); 
        i2c_write_reg(DEVID, SYSTEM__INTERRUPT_CONFIG_GPIO, SYSTEM__INTERRUPT_CONFIG_GPIO | (0b100 << 0)); 
    } else {
        i2c_write_reg(DEVID, SYSRANGE__START, SYSRANGE__START & ~(0b1 << 1)); 
        i2c_write_reg(DEVID, SYSTEM__INTERRUPT_CONFIG_GPIO, SYSTEM__INTERRUPT_CONFIG_GPIO & ~(0b111 << 0)); 
    }
}

void vl53l0x_read_distance_mm(vl53l0x_sample_t *sample) {
    if (mode == SINGLESHOT) {
        i2c_write_reg(DEVID, SYSRANGE__START, SYSRANGE__START | (0b1 << 0));  
        uint8_t distance;
        i2c_read_reg(DEVID, RESULT__RANGE_VAL, &distance);
        sample->timestamp = xTaskGetTickCount() / portTICK_RATE_MS;
        sample->distance = distance; 
        uint8_t error;
        i2c_read_reg(DEVID, RESULT__RANGE_STATUS, &error); 
        sample->valid = error == 0; 
    } 
}

void vl53l0x_isr(QueueHandle_t queue) {
    uint8_t distance;
    i2c_read_reg(DEVID, RESULT__RANGE_VAL, &distance);
    uint8_t error;
    i2c_read_reg(DEVID, RESULT__RANGE_STATUS, &error); 
    vl53l0x_sample_t sample = {
        .distance = distance,
        .timestamp = xTaskGetTickCount() / portTICK_RATE_MS,
        .valid = error == 0.
    }; 
    BaseType_t pxHigherPriorityTaskWoken; 
    xQueueSendFromISR(queue, &sample, &pxHigherPriorityTaskWoken); 
}





