/*
 * defines.h
 */
#ifndef __DEFINES_H_94b054518dfc475a9d5b532207a60b61
#define __DEFINES_H_94b054518dfc475a9d5b532207a60b61 1

//YOUR GLOBAL DEFINES HERE:

#include "libraries/API/tinyusbboard.h"
#undef LED_B
#undef LED_PWM
#undef LED_LEFT
#undef LED_RIGHT

#ifndef EXTRAPULLUP
#	define EXTRAPULLUP	D,5
#endif

#ifndef LED_RED
#	define LED_RED		B,3
#endif

#if (defined (__AVR_ATmega8__) || defined (__AVR_ATmega8A__) || \
     defined (__AVR_ATmega88__) || defined (__AVR_ATmega88P__) || defined (__AVR_ATmega88A__) || defined (__AVR_ATmega88PA__) || \
     defined (__AVR_ATmega168__) || defined (__AVR_ATmega168P__) || defined (__AVR_ATmega168A__) || defined (__AVR_ATmega168PA__) || \
     defined (__AVR_ATmega328__) || defined (__AVR_ATmega328P__) || defined (__AVR_ATmega328A__) || defined (__AVR_ATmega328PA__))

#   ifndef PWM_CLOCK1
#   	define PWM_CLOCK1	B,2
#   endif
#   ifndef PWM_CLOCK0
#   	define PWM_CLOCK0	D,4
#   endif

#else

#   ifndef PWM_CLOCK1
#	  define PWM_CLOCK1	D,4
#   endif
#   ifndef PWM_CLOCK0
#	  define PWM_CLOCK0	B,0
#   endif

#endif

#endif
