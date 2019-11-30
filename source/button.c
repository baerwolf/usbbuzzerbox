/*
 * button.c
 */
#define __BUTTON_C_4e4452d56b804728958f23a3d7f49a38	1

#include "button.h"
#include "libraries/avrlibs-baerwolf/include/cpucontext.h"
#include "libraries/avrlibs-baerwolf/include/hwclock.h"

#include "libraries/hid-KeyboardMouse/gcc-code/lib/hidcore.h"
#include "libraries/hid-KeyboardMouse/gcc-code/lib/asciimap.h"

#include <stdint.h>
#include <string.h>

/* put the message intentionally into SRAM to avoid long delay due to loading */
#ifdef HIDMESSAGE
static uint8_t hidmessage[] = HIDMESSAGE "\n";
#else
static uint8_t hidmessage[] = "";
#endif

#ifndef HIDMESSAGETIME
/* 10 seconds default timeout */
#   define HIDMESSAGETIME 100
#endif

EXTFUNC_void(int8_t, button_initialize) {
#ifdef LED_DEBUG
  CFG_OUTPUT(LED_DEBUG);
#endif
  return 0;
}

EXTFUNC_void(int8_t, button_finalize) {
  return 0;
}

#ifdef LED_DEBUG
static hwclock_time_t blinklast, blinknow;
#endif
static void __button_yield(void) {
#if (defined(DEBUGSTACK) && defined(MAINENDCYCLES))
    /* exitable code with stack debugger */
    uint8_t canary[8];
#endif
#ifdef LED_DEBUG
    uint32_t diff=0;
    blinknow=EXTFUNC_callByName(hwclock_now);
    diff = EXTFUNC_callByName(hwclock_tickspassed, blinklast, blinknow);
    if (diff > HWCLOCK_UStoTICK(100000)) {
        TOGGLE(LED_DEBUG);
        blinklast=EXTFUNC_callByName(hwclock_modify, blinklast, HWCLOCK_UStoTICK(100000)); /* last=now would accumulate jitter! */
    }
#endif
#if (defined(DEBUGSTACK) && defined(MAINENDCYCLES))
    memset(canary, DEBUGSTACK, sizeof(canary));
#endif
    EXTFUNC_callByName(cpucontext_switch, cpucontext_main_context);
#if (defined(DEBUGSTACK) && defined(MAINENDCYCLES))
    /* it seems the compiler is optimizing the context_switch with function return */
    /* so do sth. after switch, so function does not return immediatly */
    memset(canary, ~(DEBUGSTACK), sizeof(canary));
#endif
}

#define _button_delay_ms_ticks HWCLOCK_UStoTICK(32000)
static void _button_delay_32ms(uint8_t cnt) {
    hwclock_time_t last, now;
    uint32_t diff;

    last=EXTFUNC_callByName(hwclock_now);
    while (cnt > 0) {
        __button_yield();
        now=EXTFUNC_callByName(hwclock_now);
        diff = EXTFUNC_callByName(hwclock_tickspassed, last, now);
        if (diff >= _button_delay_ms_ticks) {
            cnt--;
            last=EXTFUNC_callByName(hwclock_modify, last, _button_delay_ms_ticks);
        }
    }
}

static void _button_waitclearreport(void) {
  /* wait some natural time */
  _button_delay_32ms(32>>5);
  /* clear all reported keys */
  while (keyboard_report_dirty) { __button_yield(); }
  keyboard_report_clear(&current_keyboard_report);
  /* but still mark dirty */
  keyboard_report_dirty |= 0x02;
}

static void __button_sendkey(uint8_t key) {
  _button_waitclearreport();
  asciitokeyreport(key, &current_keyboard_report);
  
  _button_waitclearreport();
}

static void __button_sendbackspace(void) {
    _button_waitclearreport();
    current_keyboard_report.modifier|=_BV(HIDKEYBOARD_MODBIT_LEFT_SHIFT);
    current_keyboard_report.keycode[0]=HIDKEYBOARD_KEYUSE_delete;
    _button_waitclearreport();
}

#if 1 /* 1=Windows, 0=Linux */
 static void _button_sendLock(void) {
   /* Windows: GUIKey + L */
   _button_waitclearreport();
   current_keyboard_report.modifier|=_BV(HIDKEYBOARD_MODBIT_LEFT_GUI);

   _button_delay_32ms(128>>5);
   _button_waitclearreport();
   current_keyboard_report.modifier|=_BV(HIDKEYBOARD_MODBIT_LEFT_GUI);
   current_keyboard_report.keycode[0]=HIDKEYBOARD_KEYUSE_l;

   _button_delay_32ms(160>>5);
   _button_waitclearreport();
   current_keyboard_report.modifier|=_BV(HIDKEYBOARD_MODBIT_LEFT_GUI);
 }
#else
static void _button_sendLock(void) {
  /* Linux: Ctrl + Alt + Delete */
  _button_waitclearreport();
  current_keyboard_report.modifier|=_BV(HIDKEYBOARD_MODBIT_LEFT_CTRL);
  current_keyboard_report.modifier|=_BV(HIDKEYBOARD_MODBIT_LEFT_ALT);

  _button_delay_32ms(128>>5);
  _button_waitclearreport();
  current_keyboard_report.modifier|=_BV(HIDKEYBOARD_MODBIT_LEFT_CTRL);
  current_keyboard_report.modifier|=_BV(HIDKEYBOARD_MODBIT_LEFT_ALT);
  current_keyboard_report.keycode[0]=0x4c; /* other Version of "delete"-key (page 55) */

  _button_delay_32ms(160>>5);
  _button_waitclearreport();
  current_keyboard_report.modifier|=_BV(HIDKEYBOARD_MODBIT_LEFT_CTRL);
  current_keyboard_report.modifier|=_BV(HIDKEYBOARD_MODBIT_LEFT_ALT);
}
#endif

#define BUTTON_DEBOUNCE_PRESSED_US	HWCLOCK_UStoTICK(20000)
EXTFUNC(int8_t, button_main, void* parameters)  {
  hwclock_time_t pressed, now;
  uint32_t ticks;
  uint8_t tickcnt;

  while (1) {
    
    /*not pressed*/
    while (!(IS_PRESSED(BUTTON_PROG))) {
      __button_yield();
    }

    /* pressed */
    ticks=0;
    pressed=EXTFUNC_callByName(hwclock_now);
    while (IS_PRESSED(BUTTON_PROG)) {
      __button_yield();
      now=EXTFUNC_callByName(hwclock_now);
      ticks=EXTFUNC_callByName(hwclock_tickspassed, pressed, now);
      if (ticks >= BUTTON_DEBOUNCE_PRESSED_US) break;
    }

    __button_yield(); /* switch to other threads */

    if (ticks >= BUTTON_DEBOUNCE_PRESSED_US) {
      //__button_sendkey('\n');
      _button_sendLock();
      /* job done - wait until unpressed */
      tickcnt=0;
      pressed=EXTFUNC_callByName(hwclock_now);
      while (IS_PRESSED(BUTTON_PROG)) {
	    __button_yield();
        now=EXTFUNC_callByName(hwclock_now);
        ticks=EXTFUNC_callByName(hwclock_tickspassed, pressed, now);
        if (ticks >= HWCLOCK_UStoTICK(100000)) {
            if (tickcnt < 0xff) tickcnt++;
            pressed=EXTFUNC_callByName(hwclock_modify, pressed, HWCLOCK_UStoTICK(100000));
        }
      }
      _button_waitclearreport();

      /* button was pressed more than 10seconds? */
      if (tickcnt > HIDMESSAGETIME) {
          if (sizeof(hidmessage) > 0) {
            for (tickcnt=0;tickcnt<3;tickcnt++) {
                __button_sendbackspace();
            }
            for (tickcnt=0;tickcnt<sizeof(hidmessage);tickcnt++) {
                __button_sendkey(hidmessage[tickcnt]);
            }
          }
      }
    }

  }
  return 0;
}
