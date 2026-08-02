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

MODE mode;
TaskHandle_t acquisition_task_handle; 

bool vl6180_alive() {
    uint8_t id = 0; 
    i2c_read_reg16(VL6180_I2C_ADDR, IDENTIFICATION__MODEL_ID, &id); 
    return id == MODEL_ID; 
}

/* 
PB0 as GPIO output
PA4 as GPIO input
configure PA4 as VL6180 GPIO1 interrupt input
Set GPIO0 to 0 (PB0)
Set GPIO0 to 1
Set GPIO1 to 1 for interrupts (PA4) (open drain; 47 kohm)
Wait 1 ms
now it is possible to configure the device and start single-shot or continuous
ranging operation
*/
bool vl6180_init() {
    RCC->AHB1ENR |= ((1 << 0) | (1 << 1));
    RCC->APB2ENR |= (1 << 14);
    GPIOB->MODER &= ~(1 << 0);
    GPIOB->MODER &= ~(1 << 1);
    GPIOB->MODER |= (1 << 0); 
    GPIOA->MODER &= ~(1 << 8);
    GPIOA->MODER &= ~(1 << 9);
    EXTI->IMR &= ~(1 << 4);
    EXTI->FTSR |= (1 << 4);
    EXTI->RTSR &= ~(1 << 4);
    NVIC_SetPriority(EXTI4_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ(EXTI4_IRQn);
    SYSCFG->EXTICR[1] &= 0xFFFFFFF0; 
    GPIOB->ODR &= ~(1 << 0); 
    burn_cycles(PERIOD_1S_CYCLES / 1000); 
    GPIOB ->ODR |= (1 << 0); 
    burn_cycles(PERIOD_1S_CYCLES / 1000); 
    i2c_write_reg16(VL6180_I2C_ADDR, SYSRANGE__THRESH_HIGH, 0xFF); 
    i2c_write_reg16(VL6180_I2C_ADDR, SYSRANGE__THRESH_LOW, 0x05); 
    i2c_write_reg16(VL6180_I2C_ADDR, SYSRANGE__INTERMEASUREMENT_PERIOD, 0x0A); 
    i2c_write_reg16(VL6180_I2C_ADDR, SYSTEM__MODE_GPIO1, 0b010000); 
    uint8_t error = 0xF;
    i2c_read_reg16(VL6180_I2C_ADDR, RESULT__RANGE_STATUS, &error); 
    return error == 0;
}

void vl6180_set_continuous(TaskHandle_t _acquisition_task_handle) {
    mode = CONTINUOUS;
    acquisition_task_handle = _acquisition_task_handle;
    EXTI->IMR |= (1 << 4);
    i2c_write_reg16(VL6180_I2C_ADDR, SYSRANGE__START,  0b11); 
    i2c_write_reg16(VL6180_I2C_ADDR, SYSTEM__INTERRUPT_CONFIG_GPIO, (0b100 << 0));
}   

void vl6180_set_singleshot() {
    mode = SINGLESHOT; 
    i2c_write_reg16(VL6180_I2C_ADDR, SYSRANGE__START, 0b0); 
    i2c_write_reg16(VL6180_I2C_ADDR, SYSTEM__INTERRUPT_CONFIG_GPIO, 0b0); 
}

void vl6180_read_distance_mm(vl6180_sample_t *sample) {
    if (mode == SINGLESHOT) {
        i2c_write_reg16(VL6180_I2C_ADDR, SYSRANGE__START, (0b1 << 0));  
    }
    uint8_t distance;
    i2c_read_reg16(VL6180_I2C_ADDR, RESULT__RANGE_VAL, &distance);
    sample->timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    sample->distance = distance; 
    uint8_t error;
    i2c_read_reg16(VL6180_I2C_ADDR, RESULT__RANGE_STATUS, &error); 
    sample->valid = error >> 4 == 0b0; 
}

void vl6180_clear_interrupt() {
    i2c_write_reg16(VL6180_I2C_ADDR, SYSTEM__INTERRUPT_CLEAR, (1 << 0)); 
}

void vl6180_isr() {
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(acquisition_task_handle, &pxHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(pxHigherPriorityTaskWoken); 
}

