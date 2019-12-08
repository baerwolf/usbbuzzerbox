USB Buzzer Box
==============

USB Buzzer Box is a free programmable one key USB HID keyboard.
By default for locking your computer screen.

There is a short pressed (locking) and a long pressed (type a message) mode
and multiple AVRs are supported with this code.


### Hardware dependencies
By default Buzzer Box is developed for and on tinyUSBboard, but other
AVR designs/platforms are possible.
(http://matrixstorm.com/avr/tinyusbboard/)

To increase the pullup-strength of the button-pin, an additional
"EXTRAPULLUP"-pin is configured as output and connected via resistor to
the button.

In order to support 24bit hardware time counting, two timers (one working
as PWM and the other clocked externally) need to be conneced by wire.
In case of tinyUSBboard it is PB2 (PWM@OC1B) with PD4 (T0).
See hwclock for more details on this.

The (V-)USB needs one external interrupt pin, and another gpio on the
same port. Plus some extra USB voltage limiting zeners, pullup and current
limiting resistors, the default tinyUSBboard port used for is "PortD".
See v-usb in 5V usecase for more details on this. (https://www.obdev.at/vusb/)

### Theory of operation
The code uses multiple git submodules for adding features like USB, HID,
I/O API, hardware time counting and multitasking.

The cpucontext@avrlibs-baerwolf module is used for implementing a basic
cooperative multitasking scheme. The "main-thread" is polling the USB and
(every 4ms) the HID, while every 16ms the "button-thread" (buttoncontext)
is beeing switched to for a short operation there.
(The only active interrupt is for USB - multitasking is done in main-loop
coordinated by hardware clock time.)

All the observable function of the buzzer box is done in the button thread:
There in absence of worries for the technical USB/HID stuff, the button-
polling and key-stronking is done.
Every few commands and in every delay/wait the "__button_yield" takes
care to task-switch back to main-thread running main-loop.
Both threads never finish (except debug-"MAINENDCYCLES"-builds) executing.

Since context/task-switches are "expensive" (lot's of additional code to
execute), switches to "buttoncontext" only take place ever 4x4ms. 
So code from "button.c" is executed only slowly and in low priority.
This is also why delay time resolution is 32ms (2**5) within there. 

The stack for "buttoncontext" is defined in main.c and currently 128 bytes
in size. Adding complexity to the button-code might need an increase there.


by Stephan Baerwolf (stephan@matrixstorm.com), Schwansee 2019
