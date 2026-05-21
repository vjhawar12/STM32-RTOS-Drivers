#include "uart.h"
#include "stm32f401xe.h"
#include <stdio.h>


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

