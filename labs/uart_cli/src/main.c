#include "FreeRTOSConfig.h"
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
#define PERIOD_500MS 5000
#define MAX_RUNS 100
#define MAX_PERIOD 100

typedef struct LED_task {
    int runs;
    int period;
} LED_task; 

QueueHandle_t message_queue, led_queue;
int commands_processed = 0;
const int cli_task_priority = 5;
const int led_task_priority = 5;

void oneshot_timer_isr() {}
void periodic_timer_isr() {}

static inline void handle_help() {
    uart_outstring("Commands: help, uptime, led on, led off, led toggle, stats, tasks, echo, queue test\r\n"); 
}

static inline void handle_uptime() {
    char buffer[32]; 
    snprintf(buffer, sizeof(buffer), "uptime: %d ms\r\n", (xTaskGetTickCount() * 1000) / configTICK_RATE_HZ); 
    uart_outstring(buffer); 
}

static inline void handle_led_on() {
    LED_task led = {
        .runs = MAX_RUNS,
        .period = MAX_PERIOD
    }; 
    xQueueSend(led_queue, &led, 0); 
    uart_outstring("turned LED2 ON\r\n"); 
}

static inline void handle_led_off() {
    LED_task led = {
        .runs = 0,
        .period = 0,
    }; 
    xQueueSend(led_queue, &led, 0); 
    uart_outstring("turned LED2 OFF\r\n"); 
}

static inline void handle_led_toggle(LED_task led) {
    xQueueSend(led_queue, &led, 0); 
}

static inline void handle_stats() {
    char buffer[BUFFER_SIZE]; 
    snprintf(buffer, sizeof(buffer), "CLI commands processed: %d\r\n", commands_processed);
    uart_outstring(buffer);
}

static inline void handle_tasks() {
    char buffer[BUFFER_SIZE]; 
    snprintf(buffer, sizeof(buffer), "Task 1:\tcli_task\r\nPriority:\t%d\r\nTask 2:\tled_task\r\nPriority:\t%d\r\n", cli_task_priority, led_task_priority);
    uart_outstring(buffer);
}

static inline void handle_echo() {
    uart_outstring("Hello World!\r\n"); 
}

void uart2_isr() {
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    char byte = uart_inchar_nonblocking();
    xQueueSendFromISR(message_queue, &byte, &pxHigherPriorityTaskWoken); 
    portYIELD_FROM_ISR(pxHigherPriorityTaskWoken); 
}

void parse_command(char* buffer) {
    int period, runs;
    char cmd1[32], cmd2[32];
    if (!strcmp(buffer, "help")) {
        handle_help();
        commands_processed++;
    } else if (!strcmp(buffer, "uptime")) {
        handle_uptime();
        commands_processed++;
    } else if (!strcmp(buffer, "led on")) {
        handle_led_on();
        commands_processed++;
    } else if (!strcmp(buffer, "led off")) {
        handle_led_off();
        commands_processed++;
    } else if (!strcmp(buffer, "stats")) {
        handle_stats();
        commands_processed++;
    } else if (!strcmp(buffer, "tasks")) {
        handle_tasks();
        commands_processed++;
    } else if (!strcmp(buffer, "echo")) {
        handle_echo();
        commands_processed++;
    } else if (sscanf(buffer, "%15s %15s %d %d", cmd1, cmd2, &period, &runs) == 4 &&
               !strcmp(cmd1, "led") &&
               !strcmp(cmd2, "toggle")){
        LED_task toggle = {
            .period = period,
            .runs = runs
        }; 
        handle_led_toggle(toggle); 
        commands_processed++;
    } else {
        uart_outstring("Invalid. Type 'help' to see supported commands\r\n"); 
    }
}

void cli_task(void* pvParams) {
    char byte;
    char buffer[BUFFER_SIZE];
    int idx = 0;
    while (1) {
        xQueueReceive(message_queue, &byte, portMAX_DELAY); 
        if (byte == '\n' || byte == '\r') {
            buffer[idx] = 0;
            parse_command(buffer); 
            idx = 0;
        } else if (idx < BUFFER_SIZE - 1) {
            buffer[idx++] = byte; 
        }
    }
}

void led_task(void* pvParams) { 
    LED_task task;
    int i = 0;
    while (1) {
        xQueueReceive(led_queue, &task, portMAX_DELAY);
        uart_outstring("got led cmd\r\n");
        char buffer[32];
        snprintf(buffer, 32, "period %d runs %d\r\n", task.period, task.runs); 
        uart_outstring(buffer); 
        if (task.period == 0 && task.runs == 0) {
            turnoff_led2();
        }  else if (task.period == MAX_PERIOD && task.runs == MAX_RUNS) {
            turnon_led2();
        } else {
            for (i = 0; i < task.runs; i++) {
                uart_outstring("on\r\n");
                turnon_led2(); 
                vTaskDelay(pdMS_TO_TICKS(task.period)); 
                uart_outstring("off\r\n");
                turnoff_led2();
                vTaskDelay(pdMS_TO_TICKS(task.period)); 
            } 
            uart_outstring("done\r\n"); 
            turnoff_led2();
        }
    }
}

int main(void) {
    PLL_Init();
    led2_Init();
    uart_Init();
    led_queue = xQueueCreate(16, sizeof(LED_task)); 
    message_queue = xQueueCreate(16, sizeof(char)); 
    xTaskCreate(cli_task, "CLI task", 512, NULL, 5, NULL);
    xTaskCreate(led_task, "LED task", 512, NULL, 7, NULL);
    uart_enable_rx_interrupt();
    vTaskStartScheduler();
    while (1) {

    } 
}