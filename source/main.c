/*
 * main.c
 */
#define __MAIN_C_dc83edef7fb74d0f88488010fe346ac7	1

#include "main.h"
#include "button.h"
#include "libraries/API/apipage.h"
#include "libraries/avrlibs-baerwolf/include/extfunc.h"
#include "libraries/avrlibs-baerwolf/include/hwclock.h"
#include "libraries/avrlibs-baerwolf/include/cpucontext.h"

#include "libraries/hid-KeyboardMouse/gcc-code/lib/hidcore.h"

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

#ifdef USB_CFG_HID_NOKEYBOARD
#	error BuzzerButton needs HID-keyboard configured
#endif

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

void EVENT_CHANGE_LED_state (void) {
  // CAPS LOCK
  if (current_LED_state & _BV(HIDKEYBOARD_LEDBIT_CAPS_LOCK))	SET_HIGH(LED_RED);
  else								SET_LOW(LED_RED);
}



// thread-stuff for button handling ///////////////////
static 	uint8_t		buttoncontext_stack[256];
static	cpucontext_t	buttoncontext;

static void prepare_buttoncontext(void) {
  EXTFUNC_functype(CPUCONTEXT_entry_t) buttonmainfunc = EXTFUNC_NULL;

  buttonmainfunc = EXTFUNC_getPtr(button_main, CPUCONTEXT_entry_t);
  EXTFUNC_callByName(cpucontext_create, &buttoncontext, buttoncontext_stack, sizeof(buttoncontext_stack), buttonmainfunc, NULL);
}

static void switchto_buttocontext(void) {
  EXTFUNC_callByName(cpucontext_switch, &buttoncontext);
}
///////////////////////////////////////////////////////


#define HIDINTERVAL	HWCLOCK_UStoTICK(4000) /*4ms*/

#ifdef MAINENDCYCLES
static uint32_t intcounter = 0;
#endif
int main(void) {
  init_cpu();
  extfunc_initialize();
  EXTFUNC_callByName(cpucontext_initialize);
  EXTFUNC_callByName(hwclock_initialize);

  hidInit();
  usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */

  SET_HIGH(EXTRAPULLUP); CFG_OUTPUT(EXTRAPULLUP);
  CFG_PULLUP(BUTTON_PROG);
  CFG_OUTPUT(LED_RED);
  _delay_ms(300);
  
  EXTFUNC_callByName(button_initialize);
  prepare_buttoncontext();


  /* connect the usb */
  usbDeviceConnect();
  usbInit();
  sei();


  {
    uint8_t		i;
    uint32_t		tdiff;
    uint16_t		btnloops=0, msloops=0, btnlooplimit=0x7fff;
    hwclock_time_t	last, now;

    last=EXTFUNC_callByName(hwclock_now);
#ifdef MAINENDCYCLES
    while (intcounter < MAINENDCYCLES) {
#else
    while (1) {
#endif
      i=0;
      btnloops++;
      msloops++;
      now=EXTFUNC_callByName(hwclock_now);
      tdiff=EXTFUNC_callByName(hwclock_tickspassed, last, now);
      if (tdiff >= HIDINTERVAL) {
	tdiff/=HIDINTERVAL;
#ifdef MAINENDCYCLES
	intcounter+=tdiff;
#endif
	i=tdiff;
	_MemoryBarrier();
	hidPoll(&i);

	btnlooplimit=msloops>>2;
	msloops=0;
#if (1)
	last=now;
#else
	_MemoryBarrier();
	tdiff=i;
	tdiff*=HIDINTERVAL;
	last=EXTFUNC_callByName(hwclock_modify, last, tdiff);
#endif
      } else {
	/* we need to poll the USB more often then every 4ms */
	i=0;
	hidPoll(&i);
      }

      // about every ms
      if (btnloops >= btnlooplimit) {
	switchto_buttocontext(); /* cooperative multitasking to button thread - which will switch back to here on its own decision */
	btnloops=0;
      }

    }
  }

  cli();
  EXTFUNC_callByName(button_finalize);
  EXTFUNC_callByName(hwclock_finalize);
  EXTFUNC_callByName(cpucontext_finalize);
  extfunc_finalize();

  bootloader_startup();
  return 0;
}