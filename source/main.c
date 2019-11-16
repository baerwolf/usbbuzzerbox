/*
 * main.c
 */
#define __MAIN_C_dc83edef7fb74d0f88488010fe346ac7	1

#include "main.h"
#include "libraries/API/apipage.h"
#include "libraries/avrlibs-baerwolf/include/extfunc.h"
#include "libraries/avrlibs-baerwolf/include/hwclock.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* http://nongnu.org/avr-libc/user-manual/modules.html */
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include <util/delay.h>


// hardware depended - only atmega8(A) at the moment //
#if (defined (__AVR_ATmega8__) || defined (__AVR_ATmega8A__))
void __hwclock_timer_init(void) {
  CFG_INPUT(PWM_CLOCK0);
  CFG_OUTPUT(PWM_CLOCK1);
  OCR1A=0xffff;
  OCR1B=0x8000;
  TCNT1=0;
  TCNT0=0;
}
void __hwclock_timer_start(void) {
//PWM TIMER1 (R&G)
  TCCR1A=0b00110011;
  TCCR1B=0b00011001;
//TIMER0
  TCCR0 =0b00000110; /* clock on falling edge of T0 */
}
#else
#	error unsupported AVR
#endif
///////////////////////////////////////////////////////

void init_cpu(void) {
  cli();
  bootupreason=MCUBOOTREASONREG;
  MCUBOOTREASONREG=0;
  wdt_disable();

}
#define SLEEPDELAY 500000
int main(void) {
  init_cpu();
  extfunc_initialize();
  EXTFUNC_callByName(hwclock_initialize);

  // YOUR CODE HERE:
  SET_HIGH(EXTRAPULLUP); CFG_OUTPUT(EXTRAPULLUP);
  CFG_PULLUP(BUTTON_PROG);
  CFG_OUTPUT(LED_RED);
  
  while (IS_PRESSED(BUTTON_PROG)) {
    _delay_ms(33);
  }
  
  {
    hwclock_time_t last, now;
    
    last=EXTFUNC_callByName(hwclock_now);
    while (1) {
      while (!(IS_PRESSED(BUTTON_PROG))) {
	uint32_t i;
	now=EXTFUNC_callByName(hwclock_now);
	i=EXTFUNC_callByName(hwclock_tickspassed, last, now);
	if (i > HWCLOCK_UStoTICK(SLEEPDELAY)) {
	  TOGGLE(LED_RED);
	  last=EXTFUNC_callByName(hwclock_modify, last, -HWCLOCK_UStoTICK(SLEEPDELAY));
	}
      }

      bootloader_startup();
    }
  }

  EXTFUNC_callByName(hwclock_finalize);
  extfunc_finalize();
  return 0;
}