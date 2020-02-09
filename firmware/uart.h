#ifndef __UART3_H
#define __UART3_H

#include "LPC17xx.h"

#define LPC_UART LPC_UART3

enum lcr_bits{
				LCR_WL0=0,
				LCR_WL1=1,
				LCR_STOP=2,
				LCR_PARITY=3,
				LCR_DLAB=7
};

enum lsr_bits{
				LSR_RDR=0,
				LSR_THRE=5
};

enum fcr_bits{
				FCR_ENABLE=0
};

#define PCLK	18000000U

void uart_init(uint32_t baud);
void uart_putch(char ch);
char uart_getch(void);
void uart_puts(char *str);
void uart_gets(char *str);


#endif
