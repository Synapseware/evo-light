/*
 * bikelight.c
 *
 * Created: 12/10/2011 1:02:19 PM
 *  Author: Matthew
 
ATtiny85 - uC powered bicycle LED bicycle headlight.
- Uses SMPS theory to regulate current flow for high power LED.
- Feedback system is an analog comparator, who's value is used to
adjust the duty cycle used by the SMPS.
- A temperature sensor monitors the heat of the high power LED
board and attempts to compensate the lighting intensity or effects
used to keep temperature within design limits (100 C max)
- 
 */ 


#include "bikelight.h"



//------------------------------------------------------------------------------------------
// volatiles used by interrupt handlers
volatile uint8_t	_mode			= MODE_NONE;			// lighting mode
volatile uint8_t	_tick			= 0;					// 1ms interval flag
volatile uint8_t	_effectsIndex	= 0;					// tracks which lighting effect is active
volatile uint8_t	_button			= 0;					// button debounce counter/delay
volatile uint8_t	_state			= 0;					// button debounce state
volatile float		_throttleBy		= 0;					// temperature compensated cycle limit
volatile uint8_t	_celcius		= 0;					// system temperature


//------------------------------------------------------------------------------------------
// Prepare the ADC for internal temperature measurement.
void prepareADC(void)
{
	ADMUX	=	(0<<REFS2)	|			// use internal 1.1v analog-ref
				(1<<REFS1)	|
				(0<<REFS0)	|
				(0<<ADLAR)	|			// don't left adjust
				(1<<MUX3)	|			// set the MUX bits for the internal temperature sensor
				(1<<MUX2)	|
				(1<<MUX1)	|
				(1<<MUX0);

	ADCSRA	=	(1<<ADEN)	|			// enable ADC
				(1<<ADSC)	|			// waste a conversion
				(0<<ADATE)	|			// no auto-trigger source
				(1<<ADIE)	|			// enable ADC interrupts
				(1<<ADPS2)	|			// set prescaler
				(1<<ADPS1)	|
				(1<<ADPS0);

}


//------------------------------------------------------------------------------------------
// Configure the analog comparator for internal 1.1v reference and input on PB1
void prepareComparator(void)
{
	ADCSRB	&=	~(1<<ACME);			// clear the ACME bit

	ACSR	=	(0<<ACD)	|		// enable comparator
				(0<<ACIE);

	ACSR	=	(1<<ACBG)	|		// use internal reference
				(0<<ACIS1)	|
				(0<<ACIS0);

	BAT_DDR	&=	~(1<<BAT_PIN);
	DIDR0	|=	(1<<AIN1D);

	// enable pull-up on battery-test-enable pin (BAT_EN_PIN)
	BAT_DDR		&=	~(1<<BAT_EN);
	BAT_PORT 	|= (1<<BAT_EN);
}


//------------------------------------------------------------------------------------------
// Prepare timer0 for PWM output on PB0/OC0A
void preparePWM(void)
{
	// setup PWM output pin
	PWM_DDR	|=	(1<<PWM_PIN);

	GTCCR	|=	(0<<TSM)	|
				(0<<PSR0);

	// setup timer0 for fast PWM, output on COM0A
	TCCR0A	=	TIMER0_PWM_OUT	|
				(1<<WGM01)		|		// Fast PWM
				(1<<WGM00);
	TCCR0B	=	(0<<WGM02)		|		// Fast PWM
				TIMER0_PRESCALE;		// Prescale macro

	OCR0A	=	1;						// Mode button select check interval (31,250/250 = 125Hz, or 125ms interval on checks)
}


//------------------------------------------------------------------------------------------
// Prepare timer1 for tick counting (1ms intervals)
void prepareTicks(void)
{
	TCCR1	=	(0<<CTC1)	|
				(1<<PWM1A)	|
				(0<<COM1A1)	|
				(0<<COM1A0)	|
				(1<<CS13)	|
				(0<<CS12)	|
				(0<<CS11)	|
				(0<<CS10);

	GTCCR	|=	(0<<PWM1B)	|
				(0<<COM1B1)	|
				(0<<COM1B0)	|
				(0<<FOC1B)	|
				(0<<FOC1A)	|
				(0<<PSR1);

	TIMSK	=	(1<<OCIE1A);

	OCR1C	=	156;
}


//------------------------------------------------------------------------------------------
// Configure the power switch input
void preparePowerSwitch(void)
{
	// trigger on falling edge of INT0
	MCUCR	|=	(1<<ISC01)	|
				(0<<ISC00);

	// enable int0 interrupt triggering
	GIMSK	|=	(1<<INT0);

	// setup PWR_SWITCH pin as input with pull up resistor enabled
	PWR_SW_DDR	&=	~(1<<PWR_SW);			// set power switch as input pin
	PWR_SW_PORT	|=	(1<<PWR_SW);			// enable pull-up resistor on switch sensor
}


//------------------------------------------------------------------------------------------
// System Setup
void init(void)
{
	PRR		=	(0<<PRTIM1)	|	// enable power to timer1
				(0<<PRTIM0)	|	// enable power to timer0
				(1<<PRUSI)	|	// shut-off the USI
				(0<<PRADC);		// enable power to the ADC
	
	// setup power LED
	PWR_LED_DDR		|= (1<<PWR_LED);
	pwrLedOn();

	prepareADC();
	
	prepareComparator();

	preparePWM();

	prepareTicks();

	preparePowerSwitch();

	// init startup values
	_state			= STATE_DEBOUNCE;
	_button			= 0;
	_mode			= MODE_NONE;
	_throttleBy		= 0;

	sei();
}


//------------------------------------------------------------------------------------------
// processes button presses
void processButton(void)
{
	switch (_state)
	{
		case STATE_DEBOUNCE:
			if (++_mode > MODE_TOTAL)
				_mode = MODE_NONE;

			_effectsIndex = 0;

			// after change modes, ignore button presses for a timeout
			_button	= BUTTON_TIMEOUT;
			_state	= STATE_RELEASED;

			// clear INT0
			GIMSK	&= ~(1<<INT0);
			GIFR	|= (1<<INTF0);
			break;

		case STATE_RELEASED:
			// make sure button is up for at least the timeout period
			if ((PWR_SW_PORT & (1<<PWR_SW)) != 0)
			{
				if (_button-- == 0)
				{
					if (_mode == MODE_NONE)
						shutDown();
					else
					{
						_state	= STATE_PROCESSED;
						GIFR	|= (1<<INTF0);
						GIMSK	|= (1<<INT0);
					}
				}
			}
			else
				_button = BUTTON_TIMEOUT;
			break;
	}
}


//------------------------------------------------------------------------------------------
// processMode
// Blinks the power LED circuit to simulate certain lighting effects
// Called every 1ms
void processMode(uint8_t mode)
{
	static uint8_t _delay = 0;

	uint8_t effectsMax = 0;
	uint8_t brightness = 0;

	// skip until delay is 0
	if (_delay > 0)
	{
		_delay--;
		return;
	}

	// steady modes
	switch(mode)
	{
		case MODE_NONE:
			setDriverLevel(SMPS_OFF);
			return;
		case MODE_DIM:
			setDriverLevel(SMPS_LOW);
			return;
		case MODE_MAX:
			setDriverLevel(SMPS_MAX);
			return;
		case MODE_BK:
			effectsMax	= BLINK_TABLE_LEN;
			_delay		= BLINK_TABLE_DELAY;
			brightness	= pgm_read_byte(&BLINK_TABLE[_effectsIndex++]);
			break;
		case MODE_SP:
			effectsMax	= SLEEP_TABLE_LEN;
			_delay		= SLEEP_TABLE_DELAY;
			brightness	= pgm_read_byte(&SLEEP_TABLE[_effectsIndex++]);
			break;
		case MODE_FD:
			effectsMax	= FADE_TABLE_LEN;
			_delay		= FADE_TABLE_DELAY;
			brightness	= pgm_read_byte(&FADE_TABLE[_effectsIndex++]);
			break;
	}

	// cycle the effects
	if (_effectsIndex >= effectsMax)
		_effectsIndex = 0;

	setDriverLevel(brightness);
}


//------------------------------------------------------------------------------------------
// Process temperature info
#define TEMP_DELAY 250
void processTemp(void)
{
	static uint8_t delay = TEMP_DELAY;

	// give us a timeout between processing calls
	if (--delay > 0)
		return;
	delay = TEMP_DELAY;

	/*
	static uint8_t addr = 1;
		T = k * [(ADCH << 8) | ADCL] + TOS
		
		Table 17-2. Temperature vs. Sensor Output Voltage (Typical Case)
		Temperature	-40°C		+25°C		+85°C		+125°C		+150°C		+165°C
		ADC			230 LSB		300 LSB		370 LSB		417			446			464

		m	= (y2-y1)/(x2-x1)
			= (370-300)/(85-25) = 7/6

		b	= y - mx
			= 370 - (7/6)(85)
			= 271

		y	= 7/6x + 271
		
		Critical temp values are:
			ADC = 446	(150°C)
				= 464	(165°C)

		Limiting starts @ 125°C.  Reduce by 40% at 150°C.  Flash when over 150°C.

		LM3405 thermal shutdown is at 165°C.  Power resumed @ 150°C
	*/

	// enter ADC sleep (which starts a conversion)
	sleep_enable();
	set_sleep_mode(SLEEP_MODE_ADC);
	sleep_cpu();

	// impose temperature restrictions
	if (_celcius > 165)
		_throttleBy = 1 - 0.40;			// impose a 40% reduction
	else if (_celcius > 125)
		_throttleBy = 1 - 0.20;			// impose a 20% reduction
	else if (_celcius > 100)
		_throttleBy = 1 - 0.10;			// impose a 10% reduction
	else
		_throttleBy = 0;			// system is cool - no temp limits imposed
}


//------------------------------------------------------------------------------------------
// Processes the battery voltage level
void processBattery(void)
{
	// introduce a throttling mechanism
	static uint8_t delay = 250;
	if (delay-- > 0)
		return;
	delay = 250;

	// quit if the battery enable line is pulled low by hardware
	if ((BAT_EN_PIN & (1<<BAT_EN)) == 0)
		return;

	/*
	Using the comparator input with ACBG set, compare the battery
	voltage against the internal 1.1v reference.

	R1 = 68k
	R2 = 15k
	Break voltage ~= 6.1

	When AIN1 is lower than the reference voltage (1.1v), then ACO is set.
	*/

	// if ACO is clear then voltage is OK and we shouldn't shut down the system
	if ((ACSR & (1<<ACO)) == 0)
		return;

	// kill the system if the voltage is too low
	_mode = MODE_NONE;
	shutDown();
}


//------------------------------------------------------------------------------------------
// sets the PWM output level
void setDriverLevel(uint8_t level)
{
	// impose temperature limits
	if (_throttleBy > 0)
		level = (uint8_t)(_throttleBy * level);

	switch (level)
	{
		case SMPS_OFF:
			TCCR0A		&=	~(TIMER0_PWM_OUT);		// Disconnect PWM timer from output pin
			PWM_PORT	&=	~(1<<PWM_PIN);			// bring PWM pin low
			break;
		default:
			TCCR0A		|=	(TIMER0_PWM_OUT);		// Connect PWM timer to output pin
			OCR0A		= level;
			break;
	}
}


//------------------------------------------------------------------------------------------
// Shuts down the uC to conserve power
void shutDown(void)
{
	// turn off modules
	power_adc_disable();
	power_timer0_disable();
	power_timer1_disable();

	// Shut down power LED
	pwrLedOff();

	// shutoff PWM system
	TCCR0A		&=	~(TIMER0_PWM_OUT);
	TCCR0B		&=	~(TIMER0_PRESCALE);
	PWM_PORT	&=	~(1<<PWM_PIN);

	// trigger on low-level
	MCUCR		&=	~((1<<ISC01) | (1<<ISC00));

	// make sure interrupts are enabled for INT0
	GIMSK		|= (1<<INT0);

	// sleep!
	sleep_enable();
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_cpu();

	cli();

	init();
}


//------------------------------------------------------------------------------------------
// Program
int main(void)
{
	init();

	shutDown();

    while(1)
    {
		if (_tick)
		{
			processBattery();

			processTemp();

			processButton();

			processMode(_mode);

			_tick = 0;
		}
	}

	return 0;
}


//------------------------------------------------------------------------------------------
// Pin-change interrupt handler
ISR(INT0_vect)
{
	// only start button state processing if it's been processed completely
	if (_state == STATE_PROCESSED)
		_state = STATE_DEBOUNCE;
}


//------------------------------------------------------------------------------------------
// Ticks counter interrupt handler
ISR(TIM1_COMPA_vect)
{
	// signal processor tick (1ms)
	_tick = 1;
}


//------------------------------------------------------------------------------------------
// A/D Conversion complete
#define ADC_K	40
ISR(ADC_vect)
{
	// convert 16 bit ADC reading to °C
	_celcius = (uint8_t)((0.8571429 * ADC) - 232) + ADC_K;
}
