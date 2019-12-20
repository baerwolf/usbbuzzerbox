/*
 * button.c
 * USB BuzzerBox project
 *
 * Stephan Baerwolf (matrixstorm@gmx.de), Schwansee 2019
 * (please contact me at least before commercial use)
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

 static void _button_sendLock(void) {
   /* GUIKey + L */
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

static void _button_sendCtrlAltDel(void) {
  /* Ctrl + Alt + Delete */
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
    tickcnt=0;
    pressed=EXTFUNC_callByName(hwclock_now);
    while (IS_PRESSED(BUTTON_PROG)) {
      __button_yield();
      now=EXTFUNC_callByName(hwclock_now);
      ticks=EXTFUNC_callByName(hwclock_tickspassed, pressed, now);
      if (ticks >= _button_delay_ms_ticks) {
          if (tickcnt < 0xff) tickcnt++;
          pressed=EXTFUNC_callByName(hwclock_modify, pressed, _button_delay_ms_ticks);
      }
    }

    __button_yield(); /* switch to other threads */

    if (tickcnt >= 1) {  /* 32ms debounce */
        if (tickcnt >= ((HIDMESSAGETIME*100)>>5)) {
            uint16_t letter;
            /* long press */

            /* maybe windows wrong password complain dialog - press some space */
            //__button_sendkey(' ');

            /* go to password promt */
            //_button_sendCtrlAltDel();

            //for (tickcnt=0;tickcnt<3;tickcnt++) {
            //    __button_sendbackspace();
            //}
            for (letter=0;letter<sizeof(hidmessage);letter++) {
                __button_sendkey(hidmessage[letter]);
                if (IS_PRESSED(BUTTON_PROG)) break;
            }

            while (IS_PRESSED(BUTTON_PROG)) {
                __button_yield();
            }

        } else {
            /* short press */

            _button_sendLock();
            _button_waitclearreport();
        }
    }
  }
  return 0;
}
