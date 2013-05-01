#include "embsys_clock.h"

unsigned int embsys_clock_read() {
	return _lr(EMBSYS_CLOCK_ADDR);
}
