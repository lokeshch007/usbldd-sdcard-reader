#include<LPC17xx.h>
#include "led.h"

void led_init(){
				LED_GPIO->FIODIR |= BV(LED);
				led_off();
}

void led_on(){
				LED_GPIO->FIOSET |= BV(LED);
}

void led_off(){
				LED_GPIO->FIOCLR = BV(LED);
}

void led_blink(){
				led_on();
				SW_DELAY_MS(50);
				led_off();
				//asm("nop");
				SW_DELAY_MS(50);
}
