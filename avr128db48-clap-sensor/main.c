
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
#define CLAP_TIMEOUT ((3*32768/2)/256) // 1.5 seconds?
#define CLAP_TIMEIN ((32768/8)/256) // 0.125 seconds? 

uint16_t current_ts = 0;
uint16_t clap_ts = 0xffff/2;
uint16_t clap_to = CLAP_TIMEOUT;
uint8_t we_have_a_clap = 0;

int16_t adc_array[256] = {0};
uint8_t adc_idx = 0;
int16_t adc_average = 0;

int16_t max = 0;

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

	// Configure sleep
	set_sleep_mode(SLEEP_MODE_IDLE);
	
	// Enable interrupts
	CPUINT.CTRLA = CPUINT_LVL0RR_bm;
	CPUINT.LVL1VEC = RTC_PIT_vect_num;
	sei();

	while(1) {
		if (we_have_a_clap)
		{
			// Do something!
			PORTB.OUTTGL = PIN3_bm;
			
			we_have_a_clap = 0;
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
	OPAMP.OP0RESMUX = OPAMP_OP0RESMUX_MUXBOT_INN_gc | OPAMP_OP0RESMUX_MUXWIP_WIP6_gc | OPAMP_OP0RESMUX_MUXTOP_OUT_gc;
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
	ADC0.INTCTRL = ADC_WCMP_bm | ADC_RESRDY_bm;
//	ADC0.INTCTRL = ADC_WCMP_bm;
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
	// Calculate running average to get noise floor?
	adc_array[adc_idx++] = ADC0.RES;

	if ((int16_t) ADC0.RES > max)
	{
		max = ADC0.RES;
	}
	
	int32_t new_adc_average = 0;
	// This should wrap around nicely?
	for (uint8_t i = adc_idx - ADC_RUNNING_AVERAGE; i != adc_idx; i++)
	{
		new_adc_average += adc_array[i];
	}
	new_adc_average /= ADC_RUNNING_AVERAGE;
	
	// If differential: consider adjusting the DAC instead?
	
	// Update window threshold if change larger than?
	if (abs(new_adc_average - adc_average) > ADC_THRESHOLD_UPDATE)
	{
		adc_average = new_adc_average;
		ADC0.WINHT = adc_average + ADC_MAX_THRESHOLD;
		ADC0.WINLT = adc_average + ADC_MIN_THRESHOLD;
	}
	
	
	// Clear interrupt
	ADC0.INTFLAGS = ADC_RESRDY_bm;
}


ISR(ADC0_WCMP_vect) {

	if ((int16_t) ADC0.RES > max)
	{
		max = ADC0.RES;
	}
	
	if (ADC0.CTRLE == ADC_WINCM_ABOVE_gc)
	{
		// Swap interrupt trigger to avoid retrigger on high level
		ADC0.CTRLE = ADC_WINCM_BELOW_gc;

		// Clap detected, remove latest sample?
//		adc_idx--;
	
		// If timein < t2-t1 < timeout, we have double clap
		uint16_t clap_diff = current_ts - clap_ts;
		if ((clap_diff <= CLAP_TIMEOUT) && (clap_diff > CLAP_TIMEIN))
		{
			if (!clap_to)
			{
				we_have_a_clap = 1;
				clap_to = CLAP_TIMEOUT;
			}
		}

		// log when the first clap was
		clap_ts = current_ts;
	} 
	else { // ADC0.CTRLE == ADC_WINCM_BELOW_gc
		// Swap interrupt trigger to avoid retrigger on low level
		ADC0.CTRLE = ADC_WINCM_ABOVE_gc;
	}

	// Clear interrupt
	ADC0.INTFLAGS = ADC_WCMP_bm;
}

ISR(RTC_PIT_vect) {
	// Increment "timestamp"
	current_ts++;
	if (clap_to)
	{
		clap_to--;
	}
	
	// Clear interrupt
	RTC.PITINTFLAGS = RTC_PI_bm;
}