#ifndef EMBSYS_PANEL_H_
#define EMBSYS_PANEL_H_

#include "embsys_utilities.h"

typedef enum
{
	BUTTON_OK 			= 0x1 << 0,
	BUTTON_1 			= 0x1 << 1,
	BUTTON_2 			= 0x1 << 2,
	BUTTON_3 			= 0x1 << 3,
	BUTTON_4 			= 0x1 << 4,
	BUTTON_5 			= 0x1 << 5,
	BUTTON_6 			= 0x1 << 6,
	BUTTON_7 			= 0x1 << 7,
	BUTTON_8 			= 0x1 << 8,
	BUTTON_9 			= 0x1 << 9,
	BUTTON_STAR 		= 0x1 << 10,
	BUTTON_0 			= 0x1 << 11,
	BUTTON_NUMBER_SIGN 	= 0x1 << 12,
}button;

unsigned int embsys_panel_init(void (*callback)(button key));

#endif /* EMBSYS_PANEL_H_ */
