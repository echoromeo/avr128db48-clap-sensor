
/*
 * \file main.c
 *
 * \brief avr128db48-low-bom-mic-interface-using-opamp
 *
 *  (c) 2020 Microchip Technology Inc. and its subsidiaries.
 *
 *  Subject to your compliance with these terms,you may use this software and
 *  any derivatives exclusively with Microchip products.It is your responsibility
 *  to comply with third party license terms applicable to your use of third party
 *  software (including open source software) that may accompany Microchip software.
 *
 *  THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
 *  EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
 *  WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
 *  PARTICULAR PURPOSE.
 *
 *  IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
 *  INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
 *  WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
 *  BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
 *  FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
 *  ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 *  THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 */

#define F_CPU 24000000ul

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include <stdlib.h>

#define ADC_RUNNING_AVERAGE 32
#define ADC_THRESHOLD_UPDATE 100
#define ADC_MAX_THRESHOLD (ADC_THRESHOLD_UPDATE*25)
#define ADC_MIN_THRESHOLD (ADC_THRESHOLD_UPDATE*3)

#define PIT_TICKS_PER_SEC (32768ul/256ul)
#define CLAP_TIMEOUT (PIT_TICKS_PER_SEC) // 1 seconds
#define CLAP_TIMEIN (PIT_TICKS_PER_SEC/10) // 0.1 seconds 

uint16_t initialclap_to = 0;
uint16_t doubleclap_to = CLAP_TIMEOUT;
uint8_t we_have_a_clap = 0;

int16_t adc_array[256] = {0};
uint8_t adc_idx = 0;

#ifdef DEBUG
int16_t max_avg = 0;
int16_t max = 0;
#endif // Debug

void OPAMP_init(void);
void ADC_init(void);
void RTC_init(void);

int main(void)
{
	_PROTECTED_WRITE(CLKCTRL.OSCHFCTRLA, CLKCTRL_FRQSEL_24M_gc);
	
	// Init peripherals
	OPAMP_init();
	RTC_init();
	ADC_init();
	PORTB.DIRSET = PIN3_bm; //CNANO LED
	PORTB.OUTSET = PIN3_bm; //CNANO LED

	// Configure sleep
	set_sleep_mode(SLEEP_MODE_IDLE);
	
	// Debug
#ifdef DEBUG
	PORTA.DIR = 0xff;
	PORTA.OUT = 0;
#endif // Debug
	
	// Enable interrupts
	CPUINT.LVL1VEC = RTC_PIT_vect_num;
	sei();

	uint8_t loop_adc_idx = 0;
	int16_t adc_average = 0;
	while(1) {
		if (we_have_a_clap)
		{
			we_have_a_clap = 0;

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

					// Do something!
					PORTB.OUTCLR = PIN3_bm; //CNANO LED
				}
			}
		}

		// Calculate running average to move threshold with noise floor
		if (loop_adc_idx != adc_idx) {
		
#ifdef DEBUG
			PORTA.OUTSET = PIN5_bm;

			if ((int16_t) ADC0.RES > max)
			{
				max = ADC0.RES;
			}
#endif // Debug

			// In case adc_idx has increased more than 1 since last
			loop_adc_idx = adc_idx - 1;
	
			int32_t new_adc_average = 0;
			// This should wrap around nicely?
			for (uint8_t i = loop_adc_idx - ADC_RUNNING_AVERAGE; i != loop_adc_idx; i++)
			{
				new_adc_average += abs(adc_array[i]);
			}
			new_adc_average /= ADC_RUNNING_AVERAGE;
	
			// Update window threshold if change larger than?
			if (abs(new_adc_average - adc_average) > ADC_THRESHOLD_UPDATE)
			{
				adc_average = new_adc_average/2;
				ADC0.WINHT = adc_average + ADC_MAX_THRESHOLD;
				ADC0.WINLT = adc_average + ADC_MIN_THRESHOLD;
			}

			loop_adc_idx++;

#ifdef DEBUG
			if (adc_average > max_avg)
			{
				max_avg = adc_average;
			}

			PORTA.OUTCLR = PIN5_bm;
#endif // Debug
		}
		
		// Go to sleep
		sleep_mode();
	}
}

void OPAMP_init(void) {

	// Set up the timebase of the OPAMP peripheral
	OPAMP.TIMEBASE = (F_CPU*0.000001)-1; // Number of CLK_PER cycles that equal one us, minus one (4-1=3)
	
	//Make OP0 an inverting PGA with gain of -15
	OPAMP.OP0INMUX = OPAMP_OP0INMUX_MUXPOS_VDDDIV2_gc | OPAMP_OP0INMUX_MUXNEG_WIP_gc;
	OPAMP.OP0RESMUX = OPAMP_OP0RESMUX_MUXBOT_INN_gc | OPAMP_OP0RESMUX_MUXWIP_WIP7_gc | OPAMP_OP0RESMUX_MUXTOP_OUT_gc;
	// Configure OP0 Control A
	OPAMP.OP0CTRLA = OPAMP_OP0CTRLA_OUTMODE_NORMAL_gc | OPAMP_ALWAYSON_bm;
	
	//Make OP1 an inverting PGA with gain of -7
	OPAMP.OP1INMUX = OPAMP_OP1INMUX_MUXPOS_VDDDIV2_gc | OPAMP_OP1INMUX_MUXNEG_WIP_gc;
	OPAMP.OP1RESMUX = OPAMP_OP1RESMUX_MUXBOT_LINKOUT_gc | OPAMP_OP1RESMUX_MUXWIP_WIP6_gc | OPAMP_OP1RESMUX_MUXTOP_OUT_gc;
	// Configure OP1 Control A
	OPAMP.OP1CTRLA = OPAMP_OP1CTRLA_OUTMODE_NORMAL_gc | OPAMP_ALWAYSON_bm;
	
	// Enable the OPAMP peripheral
	OPAMP.CTRLA = OPAMP_ENABLE_bm;
	
}

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

void RTC_init(void) {
	// Init the PIT with interrupt
	while(RTC.STATUS);
	RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc;
	RTC.PITINTCTRL = RTC_PI_bm;
	RTC.PITCTRLA = RTC_PERIOD_CYC256_gc | RTC_PITEN_bm; // ~128 Hz ok?
}

ISR(ADC0_RESRDY_vect) {
#ifdef DEBUG
	PORTA.OUTSET = PIN2_bm;
#endif // Debug

	// Compare triggered?
	if (ADC0.INTFLAGS & ADC_WCMP_bm)
	{
#ifdef DEBUG
		PORTA.OUTSET = PIN3_bm;
#endif // Debug
	
		// Above threshold, we have a clap!
		if (ADC0.CTRLE == ADC_WINCM_ABOVE_gc)
		{
			we_have_a_clap = 1;

			// Swap interrupt trigger to avoid retrigger on high level
			ADC0.CTRLE = ADC_WINCM_BELOW_gc;
		}
		else { // ADC0.CTRLE == ADC_WINCM_BELOW_gc
			// Back below threshold, reset interrupt trigger
			ADC0.CTRLE = ADC_WINCM_ABOVE_gc;
		}

		// Clear interrupts
		ADC0.INTFLAGS = ADC_WCMP_bm | ADC_RESRDY_bm;

#ifdef DEBUG
		PORTA.OUTCLR = PIN3_bm;
#endif // Debug
	} else {
		// Add sample to running average calculation
		// Also clears both interrupts..	
		adc_array[adc_idx++] = ADC0.RES;		
	}

#ifdef DEBUG
	PORTA.OUTCLR = PIN2_bm;
#endif // Debug
}

ISR(RTC_PIT_vect) {
#ifdef DEBUG
	PORTA.OUTSET = PIN4_bm;
#endif // Debug

	// Decrement timeouts
	if (initialclap_to) {
		initialclap_to--;
	}
	if (doubleclap_to) {
		doubleclap_to--;
	} else {
		PORTB.OUTSET = PIN3_bm; //CNANO LED
	}
	
	
	// Clear interrupt
	RTC.PITINTFLAGS = RTC_PI_bm;
	
#ifdef DEBUG
	PORTA.OUTCLR = PIN4_bm;
#endif // Debug
}