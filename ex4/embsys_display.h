#ifndef EMBSYS_DISPLAY_H_
#define EMBSYS_DISPLAY_H_

#include "embsys_utilities.h"

#define SCREEN_HEIGHT 18
#define SCREEN_WIDTH 12

/* initialization */
unsigned int display_init(void (*flush_complete_cb)(void));

/* set a given row on the display */
unsigned int display_set_row(unsigned int row_number, bool selected, char const line[], unsigned int length);

/* flush the display */
void display_flush();

#endif /* EMBSYS_DISPLAY_H_ */
