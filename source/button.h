/*
 * button.h
 * USB BuzzerBox project
 *
 * Stephan Baerwolf (matrixstorm@gmx.de), Schwansee 2019
 * (please contact me at least before commercial use)
 */


#ifndef __BUTTON_H_4e4452d56b804728958f23a3d7f49a38
#define __BUTTON_H_4e4452d56b804728958f23a3d7f49a38	1

#include "defines.h"
#include "libraries/avrlibs-baerwolf/include/extfunc.h"

#ifdef __BUTTON_C_4e4452d56b804728958f23a3d7f49a38
#	define	BUTTONPUBLIC
#else
#	define	BUTTONPUBLIC		extern
#endif


BUTTONPUBLIC EXTFUNC_voidhead(int8_t, button_initialize);
BUTTONPUBLIC EXTFUNC_voidhead(int8_t, button_finalize);

BUTTONPUBLIC EXTFUNC_head(int8_t, button_main, void* parameters);

#endif
