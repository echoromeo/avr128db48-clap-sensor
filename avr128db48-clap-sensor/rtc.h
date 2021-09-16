/*
 * rtc.h
 *
 * Created: 14/09/21 01:16:41
 * Author: echoromeo
 */ 


#ifndef RTC_H_
#define RTC_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>

#define PIT_PERIOD_gc RTC_PERIOD_CYC256_gc // ~128 Hz?
#define PIT_TICKS_PER_SEC (32768ul/((PIT_PERIOD_gc>>RTC_PERIOD_gp)+1))

#define DEFAULT_CLAP_TIMEOUT (PIT_TICKS_PER_SEC) // 1 seconds
#define DEFAULT_CLAP_DEBOUNCE (PIT_TICKS_PER_SEC/10) // 0.1 seconds

void RTC_init(void);
void set_singleclap_debounce(uint16_t debounce);
void set_doubleclap_window(uint16_t debounce, uint16_t window);
void set_doubleclap_debounce(uint16_t debounce);
bool is_singleclap(void);
bool is_doubleclap(void);
bool is_timedout(void);

#endif /* RTC_H_ */