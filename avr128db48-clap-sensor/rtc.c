/*
 * rtc.c
 *
 * Created: 14/09/21 11:15:12
 *  Author: M43977
 */ 

#include "main.h"
#include "rtc.h"

uint16_t initialclap_to = 0;
uint16_t doubleclap_to = CLAP_TIMEOUT;

void RTC_init(void) {
	// Init the PIT with interrupt
	while(RTC.STATUS);
	RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc;
	RTC.PITINTCTRL = RTC_PI_bm;
	RTC.PITCTRLA = RTC_PERIOD_CYC256_gc | RTC_PITEN_bm; // ~128 Hz ok?
}

bool is_doubleclap(void) {
	// Check if timeout from last double clap
	if (!doubleclap_to)
	{
		// If initial clap timeout == 0 we have first clap!
		if (!initialclap_to)
		{
			// Start countdown to acceptable first clap
			initialclap_to = CLAP_TIMEOUT;
		}
		// We have second clap inside the window!
		else if (initialclap_to < (CLAP_TIMEOUT - CLAP_TIMEIN))
		{
			// Start countdown to not allow series clapping
			doubleclap_to = CLAP_TIMEOUT;

			// Is clearing this enough to not allow series clapping?
			initialclap_to = 0;

			return true;
		}
	}
	return false;
}


ISR(RTC_PIT_vect) {
	DEBUG_RTC_ISR_high();

	// Decrement timeouts
	if (initialclap_to) {
		initialclap_to--;
	}
	if (doubleclap_to) {
		doubleclap_to--;
	} else {
		LED_off();
	}
	
	// Clear interrupt
	RTC.PITINTFLAGS = RTC_PI_bm;
	
	DEBUG_RTC_ISR_low();
}
