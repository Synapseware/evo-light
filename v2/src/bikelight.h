/*
 * bikelight.h
 *
 * Created: 12/29/2011 3:41:14 PM
 *  Author: Matthew
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <avr/power.h>
#include <math.h>

#include "pins.h"


#ifndef __BIKELIGHT__H_
#define __BIKELIGHT__H_


//------------------------------------------------------------------------------------------
// Main clock-div value
#define CLK_DIV					255


#define CYCLE_MIN				(CLK_DIV * 0.03)
#define CYCLE_MAX				(CLK_DIV * 0.90)

#define SMPS_OFF				0
#define SMPS_LOW				(CLK_DIV * 0.45)
#define SMPS_MAX				(CLK_DIV * 0.75)


//------------------------------------------------------------------------------------------
// blink modes
#define MODE_NONE				0
#define MODE_DIM				1
#define MODE_MAX				2
#define MODE_BK					3
#define MODE_SP					4
#define MODE_FD					5
#define MODE_TOTAL				5


// button debounce delay
#define BUTTON_TIMEOUT			250


// button states
#define STATE_PROCESSED			0
#define STATE_DEBOUNCE			1
#define STATE_TIMEOUT			2
#define STATE_RELEASED			3


// PWM timer values (not to exceed 1KHz)
// clk/64 = 16MHz/64 = 250KHz
// 250KHz / 256 ~= 977Hz
#define TIMER0_PRESCALE			((1<<CS02) | (0<<CS01) | (0<<CS00))
#define TIMER0_PWM_OUT			((1<<COM0A1) | (0<<COM0A0))


//------------------------------------------------------------------------------------------
// Methods
void init(void);
void processButton(void);
void processMode(uint8_t mode);
void processTemp(void);
void shutDown(void);
void setDriverLevel(uint8_t level);
//------------------------------------------------------------------------------------------



//------------------------------------------------------------------------------------------
// each "delay" value is in units of 10ms
// So, if a delay is set for 12, then the total time is 12 * 10ms, or 120ms.

#define BLINK_TABLE_DELAY 40	// 3 fast blinks and 1 slow blink
const static uint8_t BLINK_TABLE[] PROGMEM = {
	// brightness	delay
	0.10 * CLK_DIV,
	0,
	0.10 * CLK_DIV,
	0,
	0.10 * CLK_DIV,
	0,
	1.00 * CLK_DIV,
	0,
};
const static uint8_t BLINK_TABLE_LEN = sizeof(BLINK_TABLE)/sizeof(uint8_t);

#define SLEEP_TABLE_DELAY 20 // sleepy eye effect
const static uint8_t SLEEP_TABLE[] PROGMEM = {
	0.01 * CLK_DIV,
	0.02 * CLK_DIV,
	0.04 * CLK_DIV,
	0.07 * CLK_DIV,
	0.10 * CLK_DIV,
	0.18 * CLK_DIV,
	0.25 * CLK_DIV,
	0.33 * CLK_DIV,
	0.50 * CLK_DIV,
	0.70 * CLK_DIV,
	1.00 * CLK_DIV,
	0.70 * CLK_DIV,
	0.50 * CLK_DIV,
	0.33 * CLK_DIV,
	0.25 * CLK_DIV,
	0.18 * CLK_DIV,
	0.10 * CLK_DIV,
	0.07 * CLK_DIV,
	0.04 * CLK_DIV,
	0.02 * CLK_DIV,
	0.01 * CLK_DIV,
	0.01 * CLK_DIV
};
const static uint8_t SLEEP_TABLE_LEN = sizeof(SLEEP_TABLE)/sizeof(uint8_t);

#define FADE_TABLE_DELAY 20
const static uint8_t FADE_TABLE[] PROGMEM = {
	1.00 * CLK_DIV,
	0.30 * CLK_DIV,
	0.15 * CLK_DIV,
	0.10 * CLK_DIV,
	0.07 * CLK_DIV,
	0.02 * CLK_DIV,
	0.01 * CLK_DIV,
	1,
	1,
	1
};
const static uint8_t FADE_TABLE_LEN = sizeof(FADE_TABLE)/sizeof(uint8_t);


#endif /* __BIKELIGHT__H_ */