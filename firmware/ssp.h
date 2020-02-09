#ifndef __SSP_H
#define __SSP_H

#include "LPC17xx.h"

#define DATALEN_Val 8
#define SCR_VAL 0
#define CPSR_Val 18
#define SDCARD_SSP LPC_SSP1

#define SSP_GPIO LPC_GPIO0

#define SCK_FREQ 4000000

/*SSP pinselection from multiplexed pins*/
#define SSP_PINS_SELECT_CLEAR()		LPC_PINCON->PINSEL0 &= ~(BV(12) | BV(13) | BV(14) | BV(15) | BV(16) | BV(17) | BV(18) | BV(19))
#define SSP_PINS_SELECT_SET() 		LPC_PINCON->PINSEL0 |= (BV(13) | BV(15) | BV(17) | BV(19))

/*SSP SSEL pin configuration*/
#define SSP_MAKE_SSEL_OUTPUT()		SSP_GPIO->FIODIR |= BV(SSEL)
#define SSP_DISABLE_SLAVE()				SSP_GPIO->FIOSET |= BV(SSEL)
#define SSP_ENABLE_SLAVE()					SSP_GPIO->FIOCLR  = BV(SSEL)


/*SSP configurations*/
#define SSP_SET_PRESCALAR(PRESCALAR_VAL) SDCARD_SSP->CPSR = PRESCALAR_VAL 
#define SSP_SET_8BIT()	SDCARD_SSP->CR0 |= DATALEN_Val-1
#define SSP_SET_CPOL_CPHA()	SDCARD_SSP->CR0 |= BV(CPOL) | BV(CPHA)
#define SSP_SET_SCR() SDCARD_SSP->CR0 |= SCR_VAL<<SCR

/*Enabling SSP*/
#define SSP_ENABLE() SDCARD_SSP->CR1 |= BV(SSEN)

/*Write/read value to DR register*/
#define DR_WRITE(VAL) SDCARD_SSP->DR = VAL

/*Read RNE bit*/
#define RNE_VAL (SDCARD_SSP->SR & BV(RNE))

/*DR register value*/
#define DR_VALUE SDCARD_SSP->DR

/*status register*/
#define STATUS_REGISTER SDCARD_SSP->SR

enum ssp_pins{
				MISO=8,
				MOSI=9,
				SCK=7,
				SSEL=6
};


enum ssp_cr0{
				FRF0=4,
				FRF1=5,
				CPOL=6,
				CPHA=7,
				SCR=8
};

enum ssp_cr1{
				SSEN=1
};

enum ssp_sr{
				RNE=2
};

void ssp_init(void);
uint8_t ssp_write(uint8_t);
uint8_t ssp_read(void);

#endif

