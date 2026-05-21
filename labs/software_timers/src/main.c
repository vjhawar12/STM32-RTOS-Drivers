#include "irq.h"
#include "projdefs.h"
#include "platform.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>
#include <string.h>
#include "stdio.h"
#include "semphr.h"
#include "timers.h"
#include "uart.h"

#define PERIOD_1S_CYCLES 8084000
#define BUFFER_SIZE 64

QueueHandle_t queue; 
TimerHandle_t oneshot_timer_handle, periodic_timer_handle;

char periodic_timer_message[BUFFER_SIZE];  
char oneshot_timer_message[BUFFER_SIZE];   

void periodic_timer_isr() {}
void oneshot_timer_isr() {}

void periodic_timer_callback(TimerHandle_t timer_handle) {
    xQueueSend(queue, periodic_timer_message, 0); 
}   

void oneshot_timer_callback(TimerHandle_t timer_handle) {
    xQueueSend(queue, oneshot_timer_message, 0); 
}

void worker_task(void* pvParams) {
    char message[BUFFER_SIZE];
    char uart_output[BUFFER_SIZE * 2];
    while (1) { 
        xQueueReceive(queue, message, portMAX_DELAY); 
        snprintf(uart_output, sizeof(uart_output), "[%lu ms] Message Received: %s\r\n", xTaskGetTickCount(), message); 
        uart_outstring(uart_output); 
    }
}

int main(void) {
    PLL_Init();
    led2_Init();
    uart_Init();
    strcpy(periodic_timer_message, "from PERIODIC");
    strcpy(oneshot_timer_message, "from ONESHOT"); 
    queue = xQueueCreate(2, BUFFER_SIZE); 
    oneshot_timer_handle = xTimerCreate("oneshot", pdMS_TO_TICKS(200), pdFALSE, 0, oneshot_timer_callback); 
    periodic_timer_handle = xTimerCreate("periodic", pdMS_TO_TICKS(500), pdTRUE, 0, periodic_timer_callback); 
    xTaskCreate(worker_task, "worker task", 256, NULL, 10, NULL);
    xTimerStart(oneshot_timer_handle, 0); 
    xTimerStart(periodic_timer_handle, 0); 
    vTaskStartScheduler();
    while (1) {

    } 
}