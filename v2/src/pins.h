/*
 * pins.h
 *
 * Created: 12/31/2011 3:50:01 PM
 *  Author: Matthew
 */ 


#ifndef PINS_H_
#define PINS_H_

/*
*/

// Power switch
#define PWR_SW			PB2
#define PWR_SW_DDR		DDRB
#define PWR_SW_PORT		PORTB

// Power LED
#define PWR_LED			PB4
#define PWR_LED_DDR		DDRB
#define PWR_LED_PORT	PORTB
#define pwrLedOn()		PWR_LED_PORT &= ~(1<<PWR_LED)
#define pwrLedOff()		PWR_LED_PORT |= (1<<PWR_LED)

// PWM output
#define PWM_PIN			PB0
#define PWM_DDR			DDRB
#define PWM_PORT		PORTB

// Battery voltage sensor input (from voltage divider)
#define BAT_PIN			PB1
#define BAT_DDR			DDRB
#define BAT_PORT		PORTB

// hardware disable for the battery voltage sensor (pull low)
#define BAT_EN			PB3
#define BAT_EN_PIN		PINB


#endif /* PINS_H_ */