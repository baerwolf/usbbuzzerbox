/*
 * HWCLOCKCONFIG.H
 * This is version 20170129T1700ZSB
 *
 * This file is used by the actual firmware to cutomize
 * various settings using the hwclock on specific hardware.
 *
 *
 * Stephan Baerwolf (matrixstorm@gmx.de), Schwansee 2017
 * (please contact me at least before commercial use)
 */

#ifdef PWMLESS_HWCLOCK
#   define HWCLOCK_LSBTIMER_BITS		 8
#   define HWCLOCK_MSBTIMER_BITS		16

#   define HWCLOCK_LSBTIMER_VALUEREG_LOW	TCNT0
#   define HWCLOCK_LSBTIMER_VALUEREG_HIGH	SREG /* use this dummy register, since it is available on all AVRs */

#   define HWCLOCK_MSBTIMER_VALUEREG_LOW	TCNT1L
#   define HWCLOCK_MSBTIMER_VALUEREG_HIGH	TCNT1H


#else


#   define HWCLOCK_LSBTIMER_BITS		16
#   define HWCLOCK_MSBTIMER_BITS		 8

#   define HWCLOCK_LSBTIMER_VALUEREG_LOW	TCNT1L
#   define HWCLOCK_LSBTIMER_VALUEREG_HIGH	TCNT1H

#   define HWCLOCK_MSBTIMER_VALUEREG_LOW	TCNT0
#   define HWCLOCK_MSBTIMER_VALUEREG_HIGH	SREG /* use this dummy register, since it is available on all AVRs */
#endif

#define HWCLOCK_LSBTIMER_USEIOACCESS	 0 /* =1 iff registers can be accessed faster via IO-memory region */
#define HWCLOCK_MSBTIMER_USEIOACCESS	 0 /* =1 iff registers can be accessed faster via IO-memory region */


#define __HWCLOCK_NSPERTICK	(1000000/(F_CPU/1000))
