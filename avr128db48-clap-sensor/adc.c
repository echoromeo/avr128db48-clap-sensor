/*
 * adc.c
 *
 * Created: 14/09/21 11:19:45
 *  Author: M43977
 */ 

#include "main.h"
#include "adc.h"

bool clap = false;
int16_t adc_array[256] = {0};
uint8_t adc_idx = 0;

#ifdef DEBUG
int16_t max_avg = 0;
int16_t max = 0;
#endif // Debug


void ADC_init(void) {
	// Get VDD/2 on the DAC
	VREF.DAC0REF = VREF_REFSEL_VDD_gc;
	DAC0.CTRLA = DAC_ENABLE_bm;
	DAC0.DATA = 0x1ff << DAC_DATA_gp; // VDD/2

	// Set up the ADC in window mode with interrupt
	VREF.ADC0REF = VREF_REFSEL_2V048_gc;    // With differential we are ok with 2V reference?
	ADC0.CTRLA = ADC_CONVMODE_bm | ADC_FREERUN_bm | ADC_ENABLE_bm; // Differential?
	ADC0.CTRLB = ADC_SAMPNUM_ACC8_gc; // 12bit * 16 = 16bit
	//	ADC0.CTRLC = ADC_PRESC_DIV24_gc; // 1 MHz is ok?
	ADC0.CTRLC = ADC_PRESC_DIV48_gc; // 0.5 MHz is ok?
	ADC0.CTRLD = ADC_INITDLY_DLY128_gc | ADC_SAMPDLY_DLY10_gc; // Qualified guess
	ADC0.CTRLE = ADC_WINCM_ABOVE_gc;
	ADC0.SAMPCTRL = 0x10;
	ADC0.MUXPOS = ADC_MUXPOS_AIN5_gc; // OP1OUT
	ADC0.MUXNEG = ADC_MUXNEG_DAC0_gc; // VDD/2
	ADC0.INTCTRL = ADC_RESRDY_bm; // ISR handles both RESRDY and WCMP
	ADC0.WINHT = ADC_MAX_THRESHOLD;
	ADC0.WINLT = ADC_MIN_THRESHOLD;
	ADC0.COMMAND = ADC_STCONV_bm;
}

int16_t running_average(uint8_t samples) {
	static volatile uint8_t loop_adc_idx = 0;
	static volatile int16_t adc_average = 0;

	// Calculate running average to move threshold with noise floor
	if (loop_adc_idx != adc_idx) {
		
		#ifdef DEBUG
		DEBUG_ADC_AVG_high();

		if ((int16_t) ADC0.RES > max)
		{
			max = ADC0.RES;
		}
		#endif // Debug

		// In case adc_idx has increased more than 1 since last
		loop_adc_idx = adc_idx - 1;
		
		int32_t new_adc_average = 0;
		// This should wrap around nicely?
		for (uint8_t i = loop_adc_idx - samples; i != loop_adc_idx; i++)
		{
			new_adc_average += abs(adc_array[i]);
		}
		new_adc_average /= samples;
		
		// Update window threshold if change larger than?
		if (abs(new_adc_average - adc_average) > ADC_THRESHOLD_UPDATE)
		{
			adc_average = new_adc_average/2;
			ADC0.WINHT = adc_average + ADC_MAX_THRESHOLD;
			ADC0.WINLT = adc_average + ADC_MIN_THRESHOLD;

			// Should this be something like INT16_MAX
		}

		loop_adc_idx++;

		#ifdef DEBUG
		if (adc_average > max_avg)
		{
			max_avg = adc_average;
		}

		DEBUG_ADC_AVG_low();
		#endif // Debug
	}

	return adc_average;
}




ISR(ADC0_RESRDY_vect) {
	DEBUG_ADC_ISR_high();

	// Compare triggered?
	if (ADC0.INTFLAGS & ADC_WCMP_bm)
	{
		DEBUG_ADC_WCOMP_high();
		
		// Above threshold, we have a clap!
		if (ADC0.CTRLE == ADC_WINCM_ABOVE_gc)
		{
			clap = true;

			// Swap interrupt trigger to avoid retrigger on high level
			ADC0.CTRLE = ADC_WINCM_BELOW_gc;
		}
		else { // ADC0.CTRLE == ADC_WINCM_BELOW_gc
			// Back below threshold, reset interrupt trigger
			ADC0.CTRLE = ADC_WINCM_ABOVE_gc;
		}

		// Clear interrupts
		ADC0.INTFLAGS = ADC_WCMP_bm | ADC_RESRDY_bm;

		DEBUG_ADC_WCOMP_low();
	} else {
		// Add sample to running average calculation
		// Also clears both interrupts..
		adc_array[adc_idx++] = ADC0.RES;
	}

	DEBUG_ADC_ISR_low();
}

