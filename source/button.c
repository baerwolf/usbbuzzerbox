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
  return 0;
}

EXTFUNC_void(int8_t, button_finalize) {
  return 0;
}



static void __button_yield(void) {
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

static void _button_sendLock(void) {
  _button_waitclearreport();
  current_keyboard_report.modifier|=_BV(HIDKEYBOARD_MODBIT_LEFT_GUI);

  _button_waitclearreport();
  current_keyboard_report.modifier|=_BV(HIDKEYBOARD_MODBIT_LEFT_GUI);
  current_keyboard_report.keycode[0]=HIDKEYBOARD_KEYUSE_l;

  _button_waitclearreport();
  current_keyboard_report.modifier|=_BV(HIDKEYBOARD_MODBIT_LEFT_GUI);
  
  _button_waitclearreport();
}

#define BUTTON_DEBOUNCE_PRESSED_US	HWCLOCK_UStoTICK(20000)
EXTFUNC(int8_t, button_main, void* parameters)  {
  hwclock_time_t pressed, now;
  uint32_t ticks = 0;

  while (1) {
    
    /*not pressed*/
    while (!(IS_PRESSED(BUTTON_PROG))) {
      __button_yield();
    }

    /* pressed */
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
