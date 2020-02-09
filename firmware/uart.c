#include "uart.h"

void uart_init(uint32_t baud){
				uint16_t dl;
				/* baudrate calculation
				 * DL = PCLK/(16*baud)
				 */
				dl=(PCLK >> 4)/baud; 
				
				//clearing 3 2 1 0 bits 
				LPC_PINCON->PINSEL0 &= ~(BV(0) | BV(1) | BV(2) | BV(3));
				//setting (3,2)=(1,0) (1,0)=(1,0) to make pins as TXD RXD
				LPC_PINCON->PINSEL0 |= BV(1) | BV(3);
				
				//enabling inbuilt FIFO hardware
				LPC_UART->FCR |= BV(FCR_ENABLE);
				
				//setting DLAB to change DLM DLL
				LPC_UART->LCR |= BV(LCR_DLAB);
				//setting Word length as 8 bit
				LPC_UART->LCR |= BV(LCR_WL0) | BV(LCR_WL1);
				
				//DLM = (15-8) of dl
				LPC_UART->DLM = dl>>8;
				//DLM = (7-0) of dl
				LPC_UART->DLL = dl&0xFF;
				
				//clearing DLAB to restrict access to DLM and DLL
				LPC_UART->LCR &= ~BV(LCR_DLAB);
}

void uart_putch(char ch){
				//checking if THRE=1 to send Frame
				while((LPC_UART->LSR & (BV(LSR_THRE))) == 0);

				//THRE=1 means previous frame was sent
				//sending ch to THR(Transmitter Holding Register)
				LPC_UART->THR = ch;
}

char uart_getch(void){
				//checking if RDR=1 to read frame
				while((LPC_UART->LSR & (BV(LSR_RDR))) == 0);
				
				//returning character stored in RBR register
				return (char)LPC_UART->RBR;
}

void uart_puts(char *str){
				int i;
				for(i=0;str[i] != '\0';i++)
								uart_putch(str[i]);
}

void uart_gets(char *str){
				char ch;
				int i=0;
				do{
								ch=uart_getch();
								str[i++]=ch;
				}while(ch != '\r');
				str[i++]='\n';
				str[i++]='\0';
}
