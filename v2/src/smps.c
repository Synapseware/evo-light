#include "smps.h"

const uint8_t CYCLE_MIN = CLK_DIV * 0.03;
const uint8_t CYCLE_MAX = CLK_DIV * 0.95;
//uint8_t THERMAL_MAX = CYCLE_MAX;


const uint8_t SMPS_HIGH = CLK_DIV * 0.87;
const uint8_t SMPS_LOW = CLK_DIV * 0.25;
volatile uint8_t _cycleLimit;
volatile uint8_t _dutyCycle = 0;


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// SMPS uses the following pins:
//	PWM:	PINB4/OC1B/pin 3
//  FBK:	PINB1/AIN1D/pin 6
// It uses the feedback pin (analog compare 1) to change the PWM duty cycle.
// Uses timer 1 & PLL
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void SMPS_Setup(void)
{
	SMPS_OutputOff();

	// setup PLL
	PLLCSR	=	(1<<LSM) |
				(1<<PCKE) |
				(1<<PLLE);

	GTCCR	=	(0<<TSM) |			// timer/counter sync mode
				(1<<PWM1B) |		// enable PWM, channel B
				(0<<COM1B1) |		// clear on compare match, set when TCNT1 = 0
				(1<<COM1B0);

	// setup timer1, PWM1B, OCR1B, PCK/16 (32mHz/4/OCR1C = fOsc)
	OCR1B	= 0;					// default duty cycle
	OCR1C	= CLK_DIV;				// 2mHz / CLK_DIV (254) ~= 8kHz
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Output Adjusting routine - Cycle complete
// Check temperature and reduce duty cycle limit if necessary
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void SMPS_AdjustOutput(void)
{
}
