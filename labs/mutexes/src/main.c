#include "irq.h"
#include "projdefs.h"
#include "platform.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>
#include "stdio.h"
#include "semphr.h"
#include "uart.h"

#define DELAY_100MS 100

SemaphoreHandle_t mutex; 
// use mutexes to protect shared data resources without race conditions using a lock and unlock mechanism 
// ensuring tasks block until the mutex is 'free'.

char shared_buffer[64]; 

void tim2_isr() {
   
}   

void task1(void* pvParams) {
    TickType_t pxPreviousWakeTime = xTaskGetTickCount(); 
    TickType_t freqency = pdMS_TO_TICKS(DELAY_100MS * 3); 
    while (1) {
        xSemaphoreTake(mutex, portMAX_DELAY); 
        snprintf(shared_buffer, sizeof(shared_buffer), "[%u] Task 1 using shared buffer\r\n", xTaskGetTickCount()); 
        uart_outstring(shared_buffer); 
        xSemaphoreGive(mutex); 
        xTaskDelayUntil(&pxPreviousWakeTime, freqency);
    }
}

void task2(void* pvParams) {
    TickType_t pxPreviousWakeTime = xTaskGetTickCount(); 
    TickType_t freqency = pdMS_TO_TICKS(DELAY_100MS * 3); 
    while (1) {
        xSemaphoreTake(mutex, portMAX_DELAY); 
        snprintf(shared_buffer, sizeof(shared_buffer), "[%u] Task 2 using shared buffer\r\n", xTaskGetTickCount()); 
        uart_outstring(shared_buffer); 
        xSemaphoreGive(mutex); 
        xTaskDelayUntil(&pxPreviousWakeTime, freqency) ;
    }
}

void task3(void* pvParams) {
    TickType_t pxPreviousWakeTime = xTaskGetTickCount(); 
    TickType_t freqency = pdMS_TO_TICKS(DELAY_100MS * 3); 
    while (1) {
        xSemaphoreTake(mutex, portMAX_DELAY); 
        snprintf(shared_buffer, sizeof(shared_buffer), "[%u] Task 3 using shared buffer\r\n", xTaskGetTickCount()); 
        uart_outstring(shared_buffer); 
        xSemaphoreGive(mutex); 
        xTaskDelayUntil(&pxPreviousWakeTime, freqency) ;
    }
}

int main(void) {
    PLL_Init();
    led2_Init();
    uart_Init();
    mutex = xSemaphoreCreateMutex();
    xTaskCreate(task1, "task1", 256, NULL, 5, NULL);
    xTaskCreate(task2, "task2", 256, NULL, 5, NULL); 
    xTaskCreate(task3, "task3", 256, NULL, 5, NULL); 
    timer2_start();
    vTaskStartScheduler();
    while (1) {

    } 
}