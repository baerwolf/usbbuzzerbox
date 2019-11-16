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



#endif
