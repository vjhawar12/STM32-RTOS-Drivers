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

TaskHandle_t main_task_handle, background_task_handle; 
// Task Notifications are a light-weight alternative to binary semaphores for waking up a task that depends on another task, or an ISR. 

void tim2_isr() {
    BaseType_t higher_priority_awoken = pdFALSE;
    vTaskNotifyGiveFromISR(main_task_handle, &higher_priority_awoken); 
    portYIELD_FROM_ISR(higher_priority_awoken);  
}   

void main_task(void* pvParams) {
    char buffer[64];
    timer2_start();
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
        snprintf(buffer, sizeof(buffer), "[%u ms] Waking up ...\r\n", xTaskGetTickCount());
        uart_outstring(buffer); 
    }
}

void background_task(void* pvParams) {
    TickType_t previous_wait_time = xTaskGetTickCount();
    TickType_t frequency = pdMS_TO_TICKS(100); 
    char buffer[64];
    while (1) { 
        snprintf(buffer, sizeof(buffer), "[%u ms] background task running ...\r\n", xTaskGetTickCount());
        toggle_led2(PERIOD_1S_CYCLES / 20);
        xTaskDelayUntil(&previous_wait_time, frequency); 
        uart_outstring(buffer);  
    }
}

int main(void) {
    PLL_Init();
    led2_Init();
    uart_Init();
    timer2_init(5000); 
    xTaskCreate(main_task, "main task", 256, NULL, 10, &main_task_handle);
    xTaskCreate(background_task, "background task", 256, NULL, 2, &background_task_handle); 
    vTaskStartScheduler();
    while (1) {

    } 
}