#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>
#include "exception.h"
extern void vHandleException();

/* Clocking */
#define configCPU_CLOCK_HZ                       80842105UL
#define configTICK_RATE_HZ                       1000U
#define configMAX_PRIORITIES                     10
#define configMINIMAL_STACK_SIZE                 128
#define configTOTAL_HEAP_SIZE                    16 * 1024U
#define configMAX_TASK_NAME_LEN                  16
#define configUSE_16_BIT_TICKS                   0

// preemptive scheduling = 1, cooperative scheduling = 0
#define configUSE_PREEMPTION                     1

// use time slicing = 1, no time slicing = 0
#define configUSE_TIME_SLICING                   1
#define configUSE_IDLE_HOOK                      0
#define configUSE_TICK_HOOK                      0
#define configUSE_MALLOC_FAILED_HOOK             0
#define configUSE_TRACE_FACILITY                 0
#define configUSE_STATS_FORMATTING_FUNCTIONS     0

/* Features */
#define configUSE_MUTEXES                        1
#define configUSE_RECURSIVE_MUTEXES              0
#define configUSE_COUNTING_SEMAPHORES            0
#define configUSE_QUEUE_SETS                     0
#define configUSE_TASK_NOTIFICATIONS             1
#define configUSE_TIMERS                         1
#define configTIMER_TASK_PRIORITY                ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH                 10
#define configTIMER_TASK_STACK_DEPTH             configMINIMAL_STACK_SIZE

/* API inclusion */
#define INCLUDE_vTaskPrioritySet                 1
#define INCLUDE_uxTaskPriorityGet                1
#define INCLUDE_vTaskDelete                      1
#define INCLUDE_vTaskSuspend                     1
#define INCLUDE_vTaskDelayUntil                  1
#define INCLUDE_vTaskDelay                       1
#define INCLUDE_xTaskGetSchedulerState           1
#define INCLUDE_xTaskGetCurrentTaskHandle        1
#define INCLUDE_xTaskGetIdleTaskHandle           1
#define INCLUDE_xTaskResumeFromISR               1
#define INCLUDE_xTaskAbortDelay                  0
#define INCLUDE_xTaskGetHandle                   0
#define INCLUDE_eTaskGetState                    0
#define INCLUDE_xTimerPendFunctionCall           0
#define INCLUDE_uxTaskGetStackHighWaterMark      1
#define INCLUDE_uxTaskGetStackHighWaterMark2     0

/* Cortex-M interrupt priorities */
#define configPRIO_BITS                          4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY  15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

#define configKERNEL_INTERRUPT_PRIORITY \
    ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )

#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )

/* Cortex-M port mappings */
#define vPortSVCHandler      SVC_Handler
#define xPortPendSVHandler   PendSV_Handler
#define xPortSysTickHandler  SysTick_Handler
#define configASSERT(condition) do { if (!(condition)) {vHandleException(__FILE__, __LINE__);} } while (0)

#endif