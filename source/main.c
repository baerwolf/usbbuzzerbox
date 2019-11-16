/*
 * main.c
 */
#define __MAIN_C_dc83edef7fb74d0f88488010fe346ac7	1

#include "main.h"
#include "libraries/API/apipage.h"

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


void init_cpu(void) {
  cli();
  bootupreason=MCUBOOTREASONREG;
  MCUBOOTREASONREG=0;
  wdt_disable();

}

int main(void) {
  init_cpu();

  // YOUR CODE HERE:
  SET_HIGH(EXTRAPULLUP); CFG_OUTPUT(EXTRAPULLUP);
  CFG_PULLUP(BUTTON_PROG);
  CFG_OUTPUT(LED_RED);
  
  while (IS_PRESSED(BUTTON_PROG)) {
    _delay_ms(33);
  }
  
  while (1) {
    while (!(IS_PRESSED(BUTTON_PROG))) {
      TOGGLE(LED_RED);
      _delay_ms(100);
    }

    bootloader_startup();
  }

  return 0;
}