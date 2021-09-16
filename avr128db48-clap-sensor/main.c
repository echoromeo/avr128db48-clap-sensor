
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
#include <stdbool.h>

#include "main.h"
#include "rtc.h"
#include "adc.h"

typedef enum {
	SINGLE_CLAP,
	DOUBLE_CLAP,
} clap_t;

typedef struct control_type 
{
	clap_t clap_type;
	bool run_average;
} control_t;

control_t control = {
	.clap_type = DOUBLE_CLAP,
	.run_average = true,
};

void OPAMP_init(void);

int main(void)
{
	_PROTECTED_WRITE(CLKCTRL.OSCHFCTRLA, CLKCTRL_FRQSEL_24M_gc);
	
	// Init peripherals
	OPAMP_init();
	RTC_init();
	ADC_init();

	DEBUG_pin_init();
	LED_init();
	LED_off();

	// Configure sleep
	set_sleep_mode(SLEEP_MODE_IDLE);
	
	// Enable interrupts
	sei();

	while(1) {
		if (clap_detected)
		{
			clap_detected = false;
			if (control.clap_type == SINGLE_CLAP)
			{
				if (is_singleclap())
				{
					// Do something!
					LED_on();
				}
			}
			else if (control.clap_type == DOUBLE_CLAP)
			{
				if (is_doubleclap())
				{
					// Do something!
					LED_on();
				}
			}
		} else if (is_timedout())
		{
			// Do something!
			LED_off();
		}

		if (control.run_average)
		{
			running_average(ADC_RUNNING_AVERAGE);
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