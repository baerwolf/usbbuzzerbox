/*
 * main.c
 * USB BuzzerBox project
 *
 * Stephan Baerwolf (matrixstorm@gmx.de), Schwansee 2019
 * (please contact me at least before commercial use)
 */

/* when "PWMLESS_HWCLOCK" is defined - timers are initialized differently */

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

// hardware depended - only avrs for tinyusbboard at the moment //
#if (defined (__AVR_ATmega8__) || defined (__AVR_ATmega8A__) || \
     defined (__AVR_ATmega88__) || defined (__AVR_ATmega88P__) || defined (__AVR_ATmega88A__) || defined (__AVR_ATmega88PA__) || \
     defined (__AVR_ATmega168__) || defined (__AVR_ATmega168P__) || defined (__AVR_ATmega168A__) || defined (__AVR_ATmega168PA__) || \
     defined (__AVR_ATmega328__) || defined (__AVR_ATmega328P__) || defined (__AVR_ATmega328A__) || defined (__AVR_ATmega328PA__) || \
     defined (__AVR_ATmega164__) || defined (__AVR_ATmega164P__) || defined (__AVR_ATmega164A__) || defined (__AVR_ATmega164PA__) || \
     defined (__AVR_ATmega324__) || defined (__AVR_ATmega324P__) || defined (__AVR_ATmega324A__) || defined (__AVR_ATmega324PA__) || \
     defined (__AVR_ATmega644__) || defined (__AVR_ATmega644P__) || defined (__AVR_ATmega644A__) || defined (__AVR_ATmega644PA__) || \
     defined (__AVR_ATmega1284__)|| defined (__AVR_ATmega1284P__)||                                 defined (__AVR_ATmega32__)    || \
   0)
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
#   ifdef PWMLESS_HWCLOCK
  TCCR1A=0b00000000;
  TCCR1B=0b00000100;
#   else
  TCCR1A=0b00110011;
  TCCR1B=0b00011001;
#   endif

//TIMER0
#if (defined (__AVR_ATmega8__) || defined (__AVR_ATmega8A__))
#   ifdef PWMLESS_HWCLOCK
  TCCR0 =0b00000001; /* activate timer0 running */
#   else
  TCCR0 =0b00000110; /* clock on falling edge of T0 */
#   endif

#	ifdef OC2PWM_RED
  OCR2  =0xff;
  TCCR2 =0b01111001;
#	endif

#else /* not atmega8 */
#   ifdef PWMLESS_HWCLOCK
  TCCR0B=0b00000001; /* activate timer0 running */
#   else
  TCCR0B=0b00000110; /* clock on falling edge of T0 */
#   endif

#	ifdef OC2PWM_RED
#	define OCR2 OCR2A
  OCR2=0xff;
  TCCR2A=0b11000011;
  TCCR2B=0b00000001;
#	endif

#endif


#   ifdef PWMLESS_HWCLOCK
  /* calibrate timer0 to prescaler of timer1                */
  /* in general we need to use 2-cycle memory access:       */
  /* This introduces a problem, since the prescaler can     */
  /* overflow not just inbetween the two tcnthl accesses    */
  /* but also during execution of the second "LDS"...       */
  /* BUT IT IS NOT THE SAME AS: "IN r18" - "NOP" - "IN r19" */
  asm volatile (
      "timer0_calibrate_again%=:       \n\t"
      "lds  r18, %[tcnthl]             \n\t" /* 2 cycles - sampled after 1 cycle */
      "lds  r19, %[tcnthl]             \n\t" /* 2 cycles - sampled after 1 cycle */
      "inc  r18                        \n\t" /* 1 cycle  */
      "nop                             \n\t" /* 1 cycle to gcd()=1 */
      "cp   r18, r19                   \n\t" /* 1 cycle  */
      "brne timer0_calibrate_again%=   \n\t" /* 2 cycles jumping - 1 cycle continue */


      /* we configure here, as overflow would have happend during sampling of "LDS r19" */
      "ldi  r20, 7                     \n\t" /* 1 cycle  */
      "out  %[tcnt], r20               \n\t" /* 1 cycle  */
      "nop                             \n\t" /* 1 cycle - here TCNT0 must be 7 */

      /* if prescaler overflowed starting at "LDS r19" before sampling... */
      /* ...it overflows again in 254 cycles after that.                  */
      /* (That means if LDS is executed in 253 cycles and tcnthl sampled  */
      /*  in 254 cycles to be incremented one more...)                    */
      /* taking the other opcodes into account leaves us with 246 cycles  */
      "ldi  r20, 246                   \n\t" /* 1 cycle  */
      "timer0_calibrate_busyloop%=:    \n\t"
      "subi r20, 3                     \n\t" /* 1 cycle  */
      "brne timer0_calibrate_busyloop%=\n\t" /* 2 cycles jumping - 1 cycle continue */

      /* here we are 1 cycle ahead of overflow? (due to brne) - 245 cycles passed  */
      /* 253 cycles since last "LDS r19" ...                                       */
      "lds  r19, %[tcnthl]             \n\t" /* 2 cycles - sampled after 1 cycle   */
      "ldi  r20, 3                     \n\t" /* 1 cycle - in case of equal, overflow happens here */
      "cpse r18, r19                   \n\t" /* 1 cycle - equal means overflow when sampled */
      "out  %[tcnt], r20               \n\t" /* 1 cycle - overflow happend before sampling */


      :
      : [tcnt]          "i" (_SFR_IO_ADDR(TCNT0)),
        [tcnthl]		"i"	(_SFR_MEM_ADDR(HWCLOCK_MSBTIMER_VALUEREG_LOW)),
        [tcntll]		"i"	(_SFR_MEM_ADDR(HWCLOCK_LSBTIMER_VALUEREG_LOW))
      : "r20", "r19", "r18"
           );
#   endif
} /* end of hardware selected "__hwclock_timer_start" */
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
#ifdef OC2PWM_RED
  if (current_LED_state & _BV(HIDKEYBOARD_LEDBIT_CAPS_LOCK))	OCR2=0xff-OC2PWM_RED;
  else								OCR2=0xff;
#else
  if (current_LED_state & _BV(HIDKEYBOARD_LEDBIT_CAPS_LOCK))	SET_HIGH(LED_RED);
  else								SET_LOW(LED_RED);
#endif
#if LED_KANA
  if (current_LED_state & _BV(HIDKEYBOARD_LEDBIT_KANA))	SET_HIGH(LED_KANA);
  else								SET_LOW(LED_KANA);
#endif
}



// thread-stuff for button handling ///////////////////
static 	uint8_t		buttoncontext_stack[192];
static	cpucontext_t	buttoncontext;

static void prepare_buttoncontext(void) {
  EXTFUNC_functype(CPUCONTEXT_entry_t) buttonmainfunc = EXTFUNC_NULL;

#if (defined(DEBUGSTACK) && defined(MAINENDCYCLES))
  memset(buttoncontext_stack, 0, sizeof(buttoncontext_stack));
#endif

  buttonmainfunc = EXTFUNC_getPtr(button_main, CPUCONTEXT_entry_t);
  EXTFUNC_callByName(cpucontext_create, &buttoncontext, buttoncontext_stack, sizeof(buttoncontext_stack), buttonmainfunc, NULL);

  /* also enable interrupts in button-thread              */
  /* Since interrupts are still disabled here
   * in "prepare_buttoncontext", the new context
   * is inheriting this setting and will disable
   * global interrupts in every switch()                  */

  /* if context is not running (should be always so)      */
  if (!CPUCONTEXT_isDirty(&buttoncontext)) {
  /* enable interrupts by manipulating the suspended SREG */
#ifndef CPUCONTEXT_REGISTER_SREG
#   define  CPUCONTEXT_REGISTER_SREG(x) ((x)->stack->sreg)
#endif
    CPUCONTEXT_REGISTER_SREG(&buttoncontext)|=_BV(SREG_I);
  }
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
#if LED_KANA
  CFG_OUTPUT(LED_KANA);
#endif
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
    uint8_t		btnloops=0;
    hwclock_time_t	last, now;

    last=EXTFUNC_callByName(hwclock_now);
#ifdef MAINENDCYCLES
    while (intcounter < MAINENDCYCLES) {
#else
    while (1) {
#endif
      i=0;
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

	btnloops++;
	// about every 16ms
	if (btnloops >= 4) {
	  switchto_buttocontext(); /* cooperative multitasking to button thread - which will switch back to here on its own decision */
	  btnloops=0;
	}

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

    }
  }

  cli();
  usbDeviceDisconnect();
#if (defined(DEBUGSTACK) && defined(MAINENDCYCLES))
  {
    uint8_t *eeaddr = NULL;
    eeprom_update_block(buttoncontext_stack, &eeaddr[E2END-(sizeof(buttoncontext_stack)-1)], sizeof(buttoncontext_stack));
  }
#endif
  EXTFUNC_callByName(button_finalize);
  EXTFUNC_callByName(hwclock_finalize);
  EXTFUNC_callByName(cpucontext_finalize);
  extfunc_finalize();

  bootloader_startup();
  return 0;
}
