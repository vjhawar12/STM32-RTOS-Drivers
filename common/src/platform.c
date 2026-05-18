#include "platform.h"
#include "stm32f401xe.h"
#include <stddef.h>
#include "irq.h"
#include "stdint.h"

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

void uart_Init(void) {
    /* Enable clocks */
    RCC->AHB1ENR |= (1U << 0);    /* GPIOAEN */
    RCC->APB1ENR |= (1U << 17);   /* USART2EN */
    /* Configure PA2 and PA3 as alternate function mode */
    GPIOA->MODER &= ~(3U << (2U * 2U));
    GPIOA->MODER |=  (2U << (2U * 2U));
    GPIOA->MODER &= ~(3U << (3U * 2U));
    GPIOA->MODER |=  (2U << (3U * 2U));
    /* Select AF7 for PA2 and PA3 (USART2) */
    GPIOA->AFR[0] &= ~(0xFU << (4U * 2U));
    GPIOA->AFR[0] |=  (7U   << (4U * 2U));
    GPIOA->AFR[0] &= ~(0xFU << (4U * 3U));
    GPIOA->AFR[0] |=  (7U   << (4U * 3U));
    /* Push-pull, no pull-up/down */
    GPIOA->OTYPER &= ~(1U << 2U);
    GPIOA->OTYPER &= ~(1U << 3U);
    GPIOA->PUPDR &= ~(3U << (2U * 2U));
    GPIOA->PUPDR &= ~(3U << (3U * 2U));

    /* Disable USART before config */
    USART2->CR1 &= ~(1U << 13);

    /* Oversampling by 16 */
    USART2->CR1 &= ~(1U << 15);

    /* Baud ~115200 for your current clock assumption */
    USART2->BRR = 0;
    USART2->BRR |= (43U << 4);
    USART2->BRR |= (9U  << 0);

    /* No parity, 8 data bits */
    USART2->CR1 &= ~(1U << 10);
    USART2->CR1 &= ~(1U << 12);

    /* 1 stop bit */
    USART2->CR2 &= ~(3U << 12);

    /* TX enabled */
    USART2->CR1 |= (1U << 3);
    /* RX enabled */
    USART2->CR1 |= (1U << 2); 

    /* Enable USART */
    USART2->CR1 |= (1U << 13);
    NVIC_EnableIRQ(USART2_IRQn); 
}

void uart_enable_rx_interrupt() {
    USART2->CR1 |= (1U << 5); 
    // USART2->CR1 |= (1U << 7); 
}

void USART2_IRQHandler() {
    uart2_isr();
}

void uart_outchar(char c) {
    while ((USART2->SR & (1 << 7)) != (1 << 7)); 
    USART2->DR = (uint8_t)c;
}

void uart_outstring(const char *str) {
    if (str == NULL) return;
    int i = 0;
    while (str[i] != 0) {
        uart_outchar(str[i]);
        i++;
    }
}

char uart_inchar() {
    while ((USART2->SR & (1 << 5)) != (1 << 5)); 
    return USART2->DR; 
}

char uart_inchar_nonblocking() { 
    return USART2->DR; 
}

void uart_instring(char* str, const int length) {
    int i = 0;
    for (i = 0; i < length; i++) {
        str[i] = uart_inchar();
    }
}

void burn_cycles(uint32_t cycles) {
    while (cycles) {
        __asm volatile("nop");
        cycles--;
    }
}

void enable_gpio_interrupts() {
    
}