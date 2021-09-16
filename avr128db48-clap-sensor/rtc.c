/*
 * rtc.c
 *
 * Created: 14/09/21 01:15:12
 * Author: echoromeo
 */ 

#include "main.h"
#include "rtc.h"

typedef enum {
	INITIAL_CLAP,
	SECOND_CLAP,
	NUM_TIMEOUTS,
} counters;
volatile uint16_t downcount[NUM_TIMEOUTS] = {0};

typedef struct {
	uint16_t debounce;
	uint16_t initial;
	uint16_t secondary;
} clap_timing_t;
clap_timing_t timeout = {
	.debounce = DEFAULT_CLAP_DEBOUNCE,
	.initial = DEFAULT_CLAP_TIMEOUT,
	.secondary = DEFAULT_CLAP_TIMEOUT,
}; // Put in EEPROM?

void RTC_init(void) {
	// Init the PIT with interrupt
	while(RTC.STATUS);
	RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc;
	RTC.PITINTCTRL = RTC_PI_bm;
	RTC.PITCTRLA = PIT_PERIOD_gc | RTC_PITEN_bm;
}

void set_singleclap_debounce(uint16_t debounce) {
	timeout.debounce = debounce;
	
	// Initial cannot be smaller than debounce
	if (timeout.initial <= timeout.debounce)
	{
		// So doubling it makes sense?
		timeout.initial = timeout.debounce * 2;
	}
}

void set_doubleclap_window(uint16_t debounce, uint16_t window) {
	timeout.initial = window;
	set_singleclap_debounce(debounce);	
}

void set_doubleclap_debounce(uint16_t debounce) {
	timeout.secondary = debounce;
}

bool is_singleclap(void) {
	// If initial clap timeout == 0 we have a clap!
	if (!downcount[INITIAL_CLAP])
	{
		// Start countdown to not allow series clapping
		downcount[INITIAL_CLAP] = timeout.debounce;

		return true;
	}

	return false;
}


bool is_doubleclap(void) {
	// Check if timeout from last double clap
	if (!downcount[SECOND_CLAP])
	{
		// If initial clap timeout == 0 we have first clap!
		if (!downcount[INITIAL_CLAP])
		{
			// Start countdown to acceptable first clap
			downcount[INITIAL_CLAP] = timeout.initial;
		}
		// We have second clap inside the window!
		else if (downcount[INITIAL_CLAP] < (timeout.initial - timeout.debounce))
		{
			// Start countdown to not allow series clapping
			downcount[SECOND_CLAP] = timeout.secondary;

			// Reset initial clap for next clap
			downcount[INITIAL_CLAP] = 0;

			return true;
		}
	}
	return false;
}

bool is_timedout(void) {
	// If a timeout is running then not timed out
	for (uint8_t i = 0; i < NUM_TIMEOUTS; i++)
	{
		if (downcount[i]) {
			return false;
		}
	}

	return true;		
};


ISR(RTC_PIT_vect) {
	DEBUG_pin_RTC_ISR_high();

	// Decrement timeouts
	for (uint8_t i = 0; i < NUM_TIMEOUTS; i++)
	{
		if (downcount[i]) {
			downcount[i]--;
		}
	}
	
	// Clear interrupt
	RTC.PITINTFLAGS = RTC_PI_bm;
	
	DEBUG_pin_RTC_ISR_low();
}
