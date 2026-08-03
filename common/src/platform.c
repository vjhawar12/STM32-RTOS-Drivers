#include "platform.h"
#include "stm32f401xe.h"
#include <stddef.h>
#include "irq.h"
#include "stdint.h"
#include "FreeRTOSConfig.h"

void PLL_Init(void) {
    /* Disable PLL */
    RCC->CR &= ~(1U << 24);
    while (RCC->CR & (1U << 25)) {}

    /* Select HSI as system clock first */
    RCC->CFGR &= ~(3U << 0);

    /* Configure PLL from known reset state */
    RCC->PLLCFGR = 0x24003010;

    /* PLL source = HSI */
    RCC->PLLCFGR &= ~(1U << 22);

    /* PLLP = 2 */
    RCC->PLLCFGR &= ~(3U << 16);

    /* PLLN = 192 */
    RCC->PLLCFGR |= (192U << 6);


    /* PLLM = 19 */
    RCC->PLLCFGR |= 19U;


    /* Enable PLL */
    RCC->CR |= (1U << 24);
    while ((RCC->CR & (1U << 25)) == 0U) {}

    /* Flash latency = 2 wait states */
    FLASH->ACR &= ~(7U << 0);
    FLASH->ACR |=  (2U << 0);

    /* Select PLL as system clock */
    RCC->CFGR &= ~(3U << 0);
    RCC->CFGR |=  (2U << 0);

    /* Setting APB1 to 40.21 Mhz*/
    RCC->CFGR &= ~(0b111U << 10);
    RCC->CFGR |= (0b100U << 10);  

    /* Wait until PLL is actually used as system clock */
    while (((RCC->CFGR >> 2) & 3U) != 2U) {}
}

void led2_Init(void) {
    RCC->AHB1ENR |= (1U << 0);   /* GPIOA clock */

    /* PA5 as general purpose output */
    GPIOA->MODER &= ~(3U << (5U * 2U));
    GPIOA->MODER |=  (1U << (5U * 2U));

    /* Push-pull */
    GPIOA->OTYPER &= ~(1U << 5U);


    /* Medium speed */
    GPIOA->OSPEEDR &= ~(3U << (5U * 2U));
    GPIOA->OSPEEDR |=  (1U << (5U * 2U));

    /* No pull-up/pull-down */
    GPIOA->PUPDR &= ~(3U << (5U * 2U));
}

void toggle_led2(uint32_t cycles) {
    GPIOA->ODR |= (1 << 5);
    burn_cycles(cycles);
    GPIOA->ODR &= ~(1 << 5);    
    burn_cycles(cycles);
}

void turnon_led2() {
    GPIOA->ODR |= (1 << 5);
}

void turnoff_led2() {
    GPIOA->ODR &= ~(1 << 5);    
}

void timer2_init(uint16_t reload) {
    RCC->APB1ENR |= (1 << 0); // TIM2 clock enable
    TIM2->CR1 &= ~(1 << 0); // counter disable
    TIM2->CR1 &= ~(1 << 1); // update enabled
    TIM2->CR1 |= (1 << 4); // downcounter
    TIM2->CR1 |= (1 << 7); // auto reload
    TIM2->ARR = (uint16_t)reload;
    TIM2->PSC = 8083; // 80.84 MHZ / (8083 + 1) = 10000 hz => period = 0.1 ms
    TIM2->DIER |= (1 << 0); // interrupt enable
    NVIC_SetPriority(TIM2_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ(TIM2_IRQn); 
    TIM2->SR = 0;
    NVIC_ClearPendingIRQ(TIM2_IRQn);
}

void timer2_start(void) {
    TIM2->CR1 |= (1 << 0); // counter enable
}

void TIM2_IRQHandler(void) {
    TIM2->SR = 0;
    tim2_isr();
}

void timer3_init(uint16_t reload) {
    RCC->APB1ENR |= (1 << 0); // TIM2 clock enable
    TIM3->CR1 &= ~(1 << 0); // counter disable
    TIM3->CR1 &= ~(1 << 1); // update enabled
    TIM3->CR1 |= (1 << 4); // downcounter
    TIM3->CR1 &= ~(1 << 7); // disable auto reload
    TIM3->ARR = (uint16_t)reload;
    TIM3->PSC = 8083; // 80.84 MHZ / (8083 + 1) = 10000 hz => period = 0.1 ms
    TIM3->DIER |= (1 << 0); // interrupt enable
    NVIC_EnableIRQ(TIM3_IRQn); 
    TIM3->SR = 0;
    NVIC_ClearPendingIRQ(TIM3_IRQn);
}

void timer3_start(void) {
    TIM3->CR1 |= (1 << 0); // counter enable
}

void TIM3_IRQHandler(void) {
    TIM3->SR = 0;
    tim3_isr();
}

void USART2_IRQHandler() {
    uart2_isr();
}

void burn_cycles(uint32_t cycles) {
    while (cycles) {
        __asm__ volatile("nop");
        cycles--;
    }
}

void EXTI4_IRQHandler() {
    // write 1 to clear
    // read 1 ==> pending
    if ((EXTI->PR & (1 << 4)) != 0) {
        EXTI->PR = (1 << 4);
        tof_isr();
    }
}

void iwdg_init() {
    IWDG->KR = 0x5555;
    IWDG->PR = 0b011; // 1 khz freq ==> 1 ms period 
    IWDG->RLR = 2000; // 2000 * 1ms = 2 seconds
}

void iwdg_start() {
    IWDG->KR = 0xCCCC; 
}

void iwdg_reload() {
    IWDG->KR = 0xAAAA; 
}

__attribute__((weak)) void uart2_isr() {

}

__attribute__((weak)) void tim2_isr() {

}

__attribute__((weak)) void tof_isr() {

}

__attribute__((weak)) void tim3_isr() {

}