#include "projdefs.h"
#include "platform.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>
#include "stdio.h"
#include "uart.h"

#define PERIOD_1S_CYCLES 8084000

void low_priority_task(void* pvParams) {
    char buf[64];
    uint32_t count = 0;
    while (1) {
        snprintf(buf, sizeof(buf), "[%lu ms] LOW %u Step 1\r\n", xTaskGetTickCount(),  count);
        uart_outstring(buf);
        burn_cycles(PERIOD_1S_CYCLES);
        snprintf(buf, sizeof(buf), "[%lu ms] LOW %u Step 2\r\n", xTaskGetTickCount(), count);
        uart_outstring(buf);
        burn_cycles(PERIOD_1S_CYCLES);
        snprintf(buf, sizeof(buf), "[%lu ms] LOW %u Step 3\r\n", xTaskGetTickCount(), count);
        uart_outstring(buf);
        count++;
        burn_cycles(PERIOD_1S_CYCLES);
        vTaskDelay(pdMS_TO_TICKS(3000)); 
    }   
}

void med_priority_task(void* pvParams) {
    char buf[64];
    uint32_t count = 0;
    while (1) {
        snprintf(buf, sizeof(buf), "[%lu ms] MED %u Step 1\r\n", xTaskGetTickCount(),  count);
        uart_outstring(buf);
        burn_cycles(PERIOD_1S_CYCLES);
        snprintf(buf, sizeof(buf), "[%lu ms] MED %u Step 2\r\n", xTaskGetTickCount(), count);
        uart_outstring(buf);
        burn_cycles(PERIOD_1S_CYCLES);
        snprintf(buf, sizeof(buf), "[%lu ms] MED %u Step 3\r\n", xTaskGetTickCount(), count);
        uart_outstring(buf);
        count++; 
        burn_cycles(PERIOD_1S_CYCLES);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }   
}

void high_priority_task(void* pvParams) {
    char buf[64];
    uint32_t count = 0;
    while (1) {
        snprintf(buf, sizeof(buf), "[%lu ms] HIGH %u \r\n", xTaskGetTickCount(), count);
        uart_outstring(buf);
        vTaskDelay(pdMS_TO_TICKS(1000)); 
        count++;
    }
}

int main(void) {
    PLL_Init();
    led2_Init();
    uart_Init();
    xTaskCreate(low_priority_task, "low priority", 256, NULL, 2, NULL);
    xTaskCreate(med_priority_task, "med priority", 256, NULL, 3, NULL);
    xTaskCreate(high_priority_task, "high priority", 256, NULL, 10, NULL);
    vTaskStartScheduler();
    while (1) {

    } 
}