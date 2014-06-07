#ifndef SMPS_H_
#define SMPS_H_



#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include "pins.h"


const uint8_t SMPS_HIGH;
const uint8_t SMPS_LOW;


void SMPS_Setup(void);
inline void SMPS_OutputOn(uint8_t level);
inline void SMPS_OutputOff(void);
inline void SMPS_AdjustOutput(void);

#endif /* SMPS_H_ */