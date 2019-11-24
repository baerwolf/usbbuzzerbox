/*
 * button.c
 */
#define __BUTTON_C_4e4452d56b804728958f23a3d7f49a38	1

#include "button.h"
#include "libraries/avrlibs-baerwolf/include/cpucontext.h"
#include "libraries/avrlibs-baerwolf/include/hwclock.h"

#include "libraries/hid-KeyboardMouse/gcc-code/lib/hidcore.h"
#include "libraries/hid-KeyboardMouse/gcc-code/lib/asciimap.h"

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
#ifdef LED_DEBUG
    uint32_t diff=0;
    blinknow=EXTFUNC_callByName(hwclock_now);
    diff = EXTFUNC_callByName(hwclock_tickspassed, blinklast, blinknow);
    if (diff > HWCLOCK_UStoTICK(100000)) {
        TOGGLE(LED_DEBUG);
        blinklast=EXTFUNC_callByName(hwclock_modify, blinklast, HWCLOCK_UStoTICK(100000)); /* last=now would accumulate jitter! */
    }
#endif
    EXTFUNC_callByName(cpucontext_switch, cpucontext_main_context);
}

static void _button_waitclearreport(void) {
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

#if 0 /* 1=Windows, 0=Linux */
 static void _button_sendLock(void) {
   /* Windows: GUIKey + L */
   _button_waitclearreport();
   current_keyboard_report.modifier|=_BV(HIDKEYBOARD_MODBIT_LEFT_GUI);

   _button_waitclearreport();
   current_keyboard_report.modifier|=_BV(HIDKEYBOARD_MODBIT_LEFT_GUI);
   current_keyboard_report.keycode[0]=HIDKEYBOARD_KEYUSE_l;

   _button_waitclearreport();
   current_keyboard_report.modifier|=_BV(HIDKEYBOARD_MODBIT_LEFT_GUI);

   _button_waitclearreport();
 }
#else
static void _button_sendLock(void) {
  /* Linux: Ctrl + Alt + Delete */
  _button_waitclearreport();
  current_keyboard_report.modifier|=_BV(HIDKEYBOARD_MODBIT_LEFT_CTRL);
  current_keyboard_report.modifier|=_BV(HIDKEYBOARD_MODBIT_LEFT_ALT);

  _button_waitclearreport();
  current_keyboard_report.modifier|=_BV(HIDKEYBOARD_MODBIT_LEFT_CTRL);
  current_keyboard_report.modifier|=_BV(HIDKEYBOARD_MODBIT_LEFT_ALT);
  current_keyboard_report.keycode[0]=0x4c; /* other Version of "delete"-key (page 55) */

  _button_waitclearreport();
  current_keyboard_report.modifier|=_BV(HIDKEYBOARD_MODBIT_LEFT_CTRL);
  current_keyboard_report.modifier|=_BV(HIDKEYBOARD_MODBIT_LEFT_ALT);
  
  _button_waitclearreport();
}
#endif

#define BUTTON_DEBOUNCE_PRESSED_US	HWCLOCK_UStoTICK(20000)
EXTFUNC(int8_t, button_main, void* parameters)  {
  hwclock_time_t pressed, now;
  uint32_t ticks;

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
      while (IS_PRESSED(BUTTON_PROG)) {
	__button_yield();
      }
    }

  }
  return 0;
}
