/*
 * rtc.h
 *
 * Created: 14/09/21 11:16:41
 *  Author: M43977
 */ 


#ifndef RTC_H_
#define RTC_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>

#define PIT_TICKS_PER_SEC (32768ul/256ul)
#define CLAP_TIMEOUT (PIT_TICKS_PER_SEC) // 1 seconds
#define CLAP_TIMEIN (PIT_TICKS_PER_SEC/10) // 0.1 seconds

void RTC_init(void);
bool is_doubleclap(void);

#endif /* RTC_H_ */