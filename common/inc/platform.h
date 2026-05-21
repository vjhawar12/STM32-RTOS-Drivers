#ifndef PLATFORM_H
#define PLATFORM_H
#include "stdint.h"

#define periodic_timer_init timer2_init
#define periodic_timer_start timer2_start
#define oneshot_timer_init timer3_init
#define oneshot_timer_start timer3_start

void led2_Init(void);
void PLL_Init(void);
void timer2_init(uint16_t reload);
void timer2_start(void);
void timer3_init(uint16_t initial_value);
void timer3_start(void); 
void burn_cycles(uint32_t cycles);
void toggle_led2(uint32_t cycles);
void turnon_led2();
void turnoff_led2();
void iwdg_init();
void iwdg_start();
void iwdg_reload();


#endif