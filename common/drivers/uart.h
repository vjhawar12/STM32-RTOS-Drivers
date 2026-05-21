#ifndef UART_H
#define UART_H

void uart_Init(void);
void uart_enable_rx_interrupt();
void uart_outchar(char c);
void uart_outstring(const char *str); 
char uart_inchar();
char uart_inchar_nonblocking();
void uart_instring(char* str, const int length);

#endif