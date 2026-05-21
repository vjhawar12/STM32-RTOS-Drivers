#include "irq.h"
#include "projdefs.h"
#include "platform.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>
#include "stdio.h"
#include "semphr.h"
#include "timers.h"
#include "uart.h"

#define PERIOD_1S_CYCLES 8084000
#define BUFFER_SIZE 32
#define PERIOD_500MS 5000

typedef enum Event {
    TIMER,
    UART
} Event; 

typedef struct EventType {
    Event event;
    char data[BUFFER_SIZE];
} EventType;

QueueHandle_t queue;
TaskHandle_t worker_task_handle; 

void oneshot_timer_isr() {}

void periodic_timer_isr() {
    EventType timer =  {
        .event = TIMER,
        .data = "TIMER elapsed"
    }; 
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(queue, &timer, &pxHigherPriorityTaskWoken); 
    portYIELD_FROM_ISR(pxHigherPriorityTaskWoken); 
}

void uart2_isr() {
    EventType uart = {
        .event = UART
    }; 
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    uart.data[0] = uart_inchar_nonblocking();
    xQueueSendFromISR(queue, &uart, &pxHigherPriorityTaskWoken); 
    portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
}

void worker_task(void* pvParams) {
    char worker_buffer[BUFFER_SIZE];
    char byte;
    char output_buffer[BUFFER_SIZE * 2]; 
    int idx = 0;
    EventType event;
    while (1) {
        xQueueReceive(queue, &event, portMAX_DELAY); 
        if (event.event == UART) {
            byte = event.data[0];
            if (byte == '\n' || byte == '\r') {
                if (idx < BUFFER_SIZE - 1) {
                    worker_buffer[idx++] = 0;
                    snprintf(output_buffer, sizeof(output_buffer), "[%d ms] message received: %s\r\n", xTaskGetTickCount(), worker_buffer);
                    uart_outstring(output_buffer); 
                }
                idx = 0;
            } else if (idx < BUFFER_SIZE - 1) {
                worker_buffer[idx++] = byte;
            }
        } else if (event.event == TIMER) {
            snprintf(output_buffer, sizeof(output_buffer), "[%d ms] %s\r\n", xTaskGetTickCount(), event.data); 
            uart_outstring(output_buffer); 
        }
    }
}

int main(void) {
    PLL_Init();
    led2_Init();
    uart_Init();
    queue = xQueueCreate(16, sizeof(EventType)); 
    uart_outstring("Starting...\r\n"); 
    periodic_timer_init(PERIOD_500MS);
    periodic_timer_start();
    uart_outstring("after start\r\n");
    xTaskCreate(worker_task, "worker task", 256, NULL, 5, &worker_task_handle);
    uart_enable_rx_interrupt();
    vTaskStartScheduler();
    while (1) {

    } 
}