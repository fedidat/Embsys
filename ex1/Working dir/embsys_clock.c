#include "embsys_clock.h"

unsigned int embsys_clock_read()
{
	/*return the value of the clock in milliseconds*/
	return _lr(EMBSYS_CLOCK_ADDR);
}
