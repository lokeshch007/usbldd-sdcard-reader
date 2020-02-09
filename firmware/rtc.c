#include "rtc.h"

void rtc_init(rtc_t *rtc)
{
	// enable clock, reset and disable calib
	LPC_RTC->CCR = BV(CLKEN) | BV(CALIB);
	// set initial time
	rtc_set(rtc);
}

void rtc_set(rtc_t *rtc)
{
	// disable clock & reset
	LPC_RTC->CCR = BV(CTCRST) | BV(CALIB);
	// setup rtc value
	LPC_RTC->SEC = rtc->sec;
	LPC_RTC->MIN = rtc->min;
	LPC_RTC->HOUR = rtc->hour;
	LPC_RTC->DOM = rtc->day;
	LPC_RTC->MONTH = rtc->mon;
	LPC_RTC->YEAR = rtc->year;
	LPC_RTC->DOW = rtc->dow;
	LPC_RTC->DOY = rtc->doy;
	// enable clock
	LPC_RTC->CCR = BV(CLKEN) | BV(CALIB);
}

void rtc_get(rtc_t *rtc)
{
	// get rtc value
	rtc->sec = LPC_RTC->SEC;
	rtc->min = LPC_RTC->MIN;
	rtc->hour = LPC_RTC->HOUR;
	rtc->day = LPC_RTC->DOM;
	rtc->mon = LPC_RTC->MONTH;
	rtc->year = LPC_RTC->YEAR;
	rtc->dow = LPC_RTC->DOW;
	rtc->doy = LPC_RTC->DOY;	
}


