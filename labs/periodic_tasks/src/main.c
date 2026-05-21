#include "projdefs.h"
#include "platform.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>
#include "stdio.h"
#include "uart.h"

#define PERIOD_1S_CYCLES 8084000

TickType_t last_wake; 

void low_priority_task(void* pvParams) {
    char buf[64];
    const TickType_t frequency_ticks = pdMS_TO_TICKS(3000);
    TickType_t start_ticks = last_wake;
    uint32_t count = 0;
    while (1) {
        snprintf(buf, sizeof(buf), "[%lu ms] LOW %u Step 1\r\n", xTaskGetTickCount(),  count);
        uart_outstring(buf);
        burn_cycles(PERIOD_1S_CYCLES / 3);
        snprintf(buf, sizeof(buf), "[%lu ms] LOW %u Step 2\r\n", xTaskGetTickCount(), count);
        uart_outstring(buf);
        burn_cycles(PERIOD_1S_CYCLES / 3);
        snprintf(buf, sizeof(buf), "[%lu ms] LOW %u Step 3\r\n", xTaskGetTickCount(), count);
        uart_outstring(buf);
        count++;
        burn_cycles(PERIOD_1S_CYCLES / 3);
        xTaskDelayUntil(&start_ticks, frequency_ticks); 
    }   
}

void med_priority_task(void* pvParams) {
    char buf[64];
    const TickType_t frequency_ticks = pdMS_TO_TICKS(2000);
    TickType_t start_ticks = last_wake;
    uint32_t count = 0;
    while (1) {
        snprintf(buf, sizeof(buf), "[%lu ms] MED %u Step 1\r\n", xTaskGetTickCount(),  count);
        uart_outstring(buf);
        burn_cycles(PERIOD_1S_CYCLES / 3);
        snprintf(buf, sizeof(buf), "[%lu ms] MED %u Step 2\r\n", xTaskGetTickCount(), count);
        uart_outstring(buf);
        burn_cycles(PERIOD_1S_CYCLES / 3);
        snprintf(buf, sizeof(buf), "[%lu ms] MED %u Step 3\r\n", xTaskGetTickCount(), count);
        uart_outstring(buf);
        count++; 
        burn_cycles(PERIOD_1S_CYCLES / 3);
        xTaskDelayUntil(&start_ticks, frequency_ticks); 
    }   
}

void high_priority_task(void* pvParams) {
    char buf[64];
    const TickType_t frequency_ticks = pdMS_TO_TICKS(1000);
    TickType_t start_ticks = last_wake;
    uint32_t count = 0;
    while (1) {
        snprintf(buf, sizeof(buf), "[%lu ms] HIGH %u \r\n", xTaskGetTickCount(), count);
        uart_outstring(buf);
        xTaskDelayUntil(&start_ticks, frequency_ticks); 
        count++;
    }
}

int main(void) {
    PLL_Init();
    led2_Init();
    uart_Init();
    xTaskCreate(low_priority_task, "low priority", 256, NULL, 2, NULL);
    xTaskCreate(med_priority_task, "med priority", 256, NULL, 3, NULL);
    xTaskCreate(high_priority_task, "high priority", 256, NULL, 4, NULL);
    last_wake = xTaskGetTickCount();
    vTaskStartScheduler();
    while (1) {

    } 
}