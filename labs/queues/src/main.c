#include "projdefs.h"
#include "platform.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>
#include "stdio.h"
#include "queue.h"
#include "uart.h"

#define PERIOD_1S_CYCLES 8084000

typedef struct fake_sensor_data{
    uint32_t sample_id;
    TickType_t produced_tick;
    int32_t value;
} fake_sensor_data; 

QueueHandle_t queue;
// use queues when you want to pass data between producer and consumer tasks while avoiding race conditions, avoiding global state variables, and using blocking not polling

void producer_with_queue(void* pvParams) {
    int value = 1;
    int sample_id = 0;
    TickType_t last_wake = xTaskGetTickCount(); 
    const TickType_t frequency = pdMS_TO_TICKS(100);
    while (1) {
        fake_sensor_data data;  
        data.value = value *= 2;
        data.sample_id = sample_id++;
        data.produced_tick = xTaskGetTickCount();
        if (xQueueSend(queue, &data, 0) == pdPASS){}
        else {
            uart_outstring("Queue full\r\n");
        }
        xTaskDelayUntil(&last_wake, frequency); 
    } 
}

void consumer_with_queue(void* pvParams) {
    char buffer[64];
    while (1) {
        fake_sensor_data data;
        // setting xTicksToWait to portMAX_DELAY makes it block (not poll) until an item is added to the queue
        if (xQueueReceive(queue, &data, portMAX_DELAY) == pdPASS) {
            snprintf(buffer, sizeof(buffer), "ID: %u Time Sent: %u Time received: %u Value: %u\r\n", data.sample_id, data.produced_tick, xTaskGetTickCount(), data.value); 
            uart_outstring(buffer);
        } else uart_outstring("Queue empty\r\n");
        vTaskDelay(pdMS_TO_TICKS(500)); 
    } 
}

int main(void) {
    PLL_Init();
    led2_Init();
    uart_Init();
    queue = xQueueCreate(8, sizeof(fake_sensor_data));
    xTaskCreate(producer_with_queue, "producer with queue", 256, NULL, 2, NULL);
    xTaskCreate(consumer_with_queue, "consumer with queue", 256, NULL, 2, NULL);
    // xTaskCreate(producer_without_queue, "producer no queue", 256, NULL, 2, NULL);
    // xTaskCreate(consumer_without_queue, "consumer no queue", 256, NULL, 2, NULL);
    vTaskStartScheduler();
    while (1) {

    } 
}