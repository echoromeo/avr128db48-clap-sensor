
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

#include <stdlib.h>

#define RUNNING_AVERAGE 16
#define ADC_THRESHOLD_UPDATE 50 //No idea
#define CLAP_TIMEOUT 20000 //No idea
#define CLAP_TIMEIN 1000 //No idea

uint16_t timestamp = 0;
uint16_t clap_ts = 0;
uint8_t we_have_a_clap = 0;

uint16_t adc_array[256] = {0};
uint8_t adc_idx = 0;
uint16_t adc_average = 0;

void OPAMP_init(void);
void ADC_init(void);
void RTC_init(void);

int main(void)
{
	_PROTECTED_WRITE(CLKCTRL.OSCHFCTRLA, CLKCTRL_FRQSEL_24M_gc);
	
	OPAMP_init();
	ADC_init();
	RTC_init();

	while(1) {
		if (we_have_a_clap)
		{
			// Do something!
			
			we_have_a_clap = 0;
		}
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
	// Set up the ADC in window mode with interrupt
	
	// Trigger the ADC from PIT event

}

void RTC_init(void) {
	// Init the PIT with interrupt
}

ISR(ADC0_RESRDY_vect) {
	// Calculate running average to get noise floor?
	adc_array[adc_idx++] = ADC0.RES;
	
	int32_t new_adc_average = 0;
	// This should wrap around nicely?
	for (uint8_t i = adc_idx - RUNNING_AVERAGE; i != adc_idx; i++)
	{
		new_adc_average += adc_array[i];
	}
	new_adc_average /= RUNNING_AVERAGE;
	
	// Update window threshold if change larger than?
	if (abs(new_adc_average - adc_average) > ADC_THRESHOLD_UPDATE)
	{
		adc_average = new_adc_average;
		ADC0.WINHT = adc_average + ADC_THRESHOLD_UPDATE*3; //No idea
	}
	
	// Clear interrupt
	ADC0.INTFLAGS = ADC_RESRDY_bm;
}


ISR(ADC0_WCMP_vect) {
	// Clap detected, remove latest sample?
	adc_idx--;
	
	// If timein < t2-t1 < timeout, we have double clap
	uint16_t clap_diff = timestamp - clap_ts; 
	if ((clap_diff <= CLAP_TIMEOUT) && (clap_diff > CLAP_TIMEIN))
	{
		we_have_a_clap = 1;
	}
		
	// else log t1
	clap_ts = timestamp;

	// Clear interrupt
	ADC0.INTFLAGS = ADC_WCMP_bm;
}

ISR(RTC_PIT_vect) {
	// Increment "timestamp"
	timestamp++;
	
	// Clear interrupt
	RTC.PITINTFLAGS = RTC_PI_bm;
}