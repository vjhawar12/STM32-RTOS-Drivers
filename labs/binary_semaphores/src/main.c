#include "irq.h"
#include "projdefs.h"
#include "platform.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>
#include "stdio.h"
#include "semphr.h"
#include "uart.h"

#define PERIOD_1S_CYCLES 8084000

SemaphoreHandle_t binary_semphr; 
// use binary semaphores when task2 waits on task1 without needing actual data being passed -- just a signal telling task2 to wake up

void tim2_isr() {
    BaseType_t higher_priority_awoken = pdFALSE;
    xSemaphoreGiveFromISR(binary_semphr, &higher_priority_awoken);
    portYIELD_FROM_ISR(higher_priority_awoken);  
}   

void main_task(void* pvParams) {
    char buffer[64];
    while (1) {
        xSemaphoreTake(binary_semphr, portMAX_DELAY);
        snprintf(buffer, sizeof(buffer), "[%u ms] Waking up ...\r\n", xTaskGetTickCount());
        uart_outstring(buffer); 
    }
}

void background_task(void* pvParams) {
    char buffer[64];
    TickType_t previous_wait_time = xTaskGetTickCount();
    TickType_t frequency_ticks = pdMS_TO_TICKS(100);
    while (1) {
        snprintf(buffer, sizeof(buffer), "[%u ms] background task running ...\r\n", xTaskGetTickCount());
        toggle_led2(PERIOD_1S_CYCLES / 20); 
        vTaskDelayUntil(&previous_wait_time, frequency_ticks); 
        uart_outstring(buffer); 
    }
}

int main(void) {
    PLL_Init();
    led2_Init();
    uart_Init();
    timer2_init(5000); 
    binary_semphr = xSemaphoreCreateBinary();
    xTaskCreate(main_task, "main task", 256, NULL, 10, NULL);
    xTaskCreate(background_task, "background_task", 256, NULL, 2, NULL); 
    timer2_start();
    vTaskStartScheduler();
    while (1) {

    } 
}