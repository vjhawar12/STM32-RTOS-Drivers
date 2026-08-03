#include "exception.h"
#include <stdint.h>
#include <string.h>
#include "cmsis_gcc.h" 
#include "stm32f401xe.h"

exception_t exception;
uint32_t lr, pc;
extern uint8_t dropped_messages;
extern uint8_t vl6180_error;

void vHandleException(const char* file, const int line) {
    __asm__("MOV %0, LR" : "=r" (lr));
    __asm__("MOV %0, PC" : "=r" (pc));
    exception.file = file;
    exception.line = line;
    exception.CFSR = SCB->CFSR;
    exception.HFSR = SCB->HFSR;
    exception.BFAR = SCB->BFAR;
    exception.MMFAR = SCB->MMFAR;
    exception.ICSR = SCB->ICSR;
    exception.SHCSR = SCB->SHCSR;
    exception.VTOR = SCB->VTOR;
    exception.xPSR = __get_xPSR();
    exception.PSP = __get_PSP();
    exception.MSP = __get_MSP();
    exception.LR = lr;
    exception.PC = pc;
    exception.dropped_messages = dropped_messages;
    exception.vl6180_error = vl6180_error;
    memcpy(exception.SHPR, SCB->SHP, sizeof(exception.SHPR));
    __BKPT(0);
    while(1) {}
}