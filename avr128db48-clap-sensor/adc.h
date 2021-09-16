/*
 * adc.h
 *
 * Created: 14/09/21 01:20:02
 * Author: echoromeo
 */ 


#ifndef ADC_H_
#define ADC_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdbool.h>

#define ADC_RUNNING_AVERAGE 32
#define ADC_THRESHOLD_UPDATE 100
#define ADC_MAX_THRESHOLD (ADC_THRESHOLD_UPDATE*15)
#define ADC_MIN_THRESHOLD (ADC_THRESHOLD_UPDATE*3)

extern volatile bool clap_detected;

void ADC_init(void);
int16_t running_average(uint8_t samples);

#endif /* ADC_H_ */