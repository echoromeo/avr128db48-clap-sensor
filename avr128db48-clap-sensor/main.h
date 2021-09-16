/*
 * main.h
 *
 * Created: 14/09/21 12:13:03
 *  Author: M43977
 */ 


#ifndef MAIN_H_
#define MAIN_H_

#include <avr/io.h>

#ifdef CNANO_LED
#define LED_init()	(PORTB.DIRSET = PIN3_bm)
#define LED_on()	(PORTB.OUTCLR = PIN3_bm)
#define LED_off()	(PORTB.OUTSET = PIN3_bm)
#else
#define LED_init()
#define LED_on()
#define LED_off()
#endif

#ifdef DEBUG
#define DEBUG_init() (PORTA.DIRSET = 0xff)
#define DEBUG_ADC_ISR_high() (PORTA.OUTSET = PIN2_bm)
#define DEBUG_ADC_ISR_low()  (PORTA.OUTCLR = PIN2_bm)
#define DEBUG_ADC_WCOMP_high() (PORTA.OUTSET = PIN3_bm)
#define DEBUG_ADC_WCOMP_low()  (PORTA.OUTCLR = PIN3_bm)
#define DEBUG_ADC_AVG_high() (PORTA.OUTSET = PIN4_bm)
#define DEBUG_ADC_AVG_low()  (PORTA.OUTCLR = PIN4_bm)
#define DEBUG_RTC_ISR_high() (PORTA.OUTSET = PIN5_bm)
#define DEBUG_RTC_ISR_low()  (PORTA.OUTCLR = PIN5_bm)
#else
#define DEBUG_init()		
#define DEBUG_ADC_ISR_high()
#define DEBUG_ADC_ISR_low()	
#define DEBUG_ADC_WCOMP_high()
#define DEBUG_ADC_WCOMP_low()
#define DEBUG_ADC_AVG_high()
#define DEBUG_ADC_AVG_low()	
#define DEBUG_RTC_ISR_high()
#define DEBUG_RTC_ISR_low()	
#endif


#endif /* MAIN_H_ */