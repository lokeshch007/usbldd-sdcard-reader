#ifndef __RTC_H
#define __RTC_H

#include "LPC17xx.h"

#define CLKEN		0
#define CTCRST		1
#define CALIB		4

typedef struct
{
	int sec, min, hour;
	int day, mon, year;
	int dow, doy;
}rtc_t;

void rtc_init(rtc_t *rtc);
void rtc_set(rtc_t *rtc);
void rtc_get(rtc_t *rtc);

#endif

