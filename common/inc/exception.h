#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <stdint.h>
#include <stddef.h>

typedef struct exception_t {
    uint32_t CFSR;
    uint32_t HFSR;
    uint32_t BFAR;
    uint32_t MMFAR;
    uint32_t ICSR;
    uint32_t SHCSR;
    uint32_t VTOR;
    uint32_t SHPR[2];
    uint32_t xPSR;
    uint32_t PSP;
    uint32_t MSP;
    uint32_t LR;
    uint32_t PC;
    char *file;
    int line;
} exception_t;

void vHandleException(const char* file, const int line);

#endif