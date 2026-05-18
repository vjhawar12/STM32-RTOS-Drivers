#include "vl6180.h"
#include "i2c.h"
#include "portmacro.h"
#include "projdefs.h"
#include "stdbool.h"
#include "stm32f401xe.h"
#include "platform.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "irq.h"

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
TaskHandle_t acquisition_task_handle; 

bool vl6180_alive() {
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
bool vl6180_init() {
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

void vl3l0x_set_continuous(TaskHandle_t _acquisition_task_handle) {
    mode = CONTINUOUS;
    i2c_write_reg(DEVID, SYSRANGE__START, SYSRANGE__START | (0b1 << 1)); 
    i2c_write_reg(DEVID, SYSTEM__INTERRUPT_CONFIG_GPIO, SYSTEM__INTERRUPT_CONFIG_GPIO | (0b100 << 0)); 
    acquisition_task_handle = _acquisition_task_handle;
}

void vl310x_set_singleshot() {
    mode = SINGLESHOT; 
    i2c_write_reg(DEVID, SYSRANGE__START, SYSRANGE__START & ~(0b1 << 1)); 
    i2c_write_reg(DEVID, SYSTEM__INTERRUPT_CONFIG_GPIO, SYSTEM__INTERRUPT_CONFIG_GPIO & ~(0b111 << 0)); 
}

void vl6180_read_distance_mm(vl6180_sample_t *sample) {
    if (mode == SINGLESHOT) {
        i2c_write_reg(DEVID, SYSRANGE__START, SYSRANGE__START | (0b1 << 0));  
    }
    uint8_t distance;
    i2c_read_reg(DEVID, RESULT__RANGE_VAL, &distance);
    sample->timestamp = xTaskGetTickCount() / portTICK_RATE_MS;
    sample->distance = distance; 
    uint8_t error;
    i2c_read_reg(DEVID, RESULT__RANGE_STATUS, &error); 
    sample->valid = error == 0; 
}

void vl6180_isr() {
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(acquisition_task_handle, &pxHigherPriorityTaskWoken);
}

