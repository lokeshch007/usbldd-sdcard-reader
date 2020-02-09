#ifndef __LED_H
#define __LED_H

#define LED 29
#define LED_GPIO LPC_GPIO1


void led_init();
void led_on();
void led_off();
void led_blink();

#endif
