#ifndef EMBSYS_TIMER_H_
#define EMBSYS_TIMER_H_

#include "embsys_utilities.h"

#define CPU_FREQ 50*1000000
#define CYCLES_IN_MS (CPU_FREQ/1000)

void embsys_timer_init(unsigned int interval);

#endif /* EMBSYS_TIMER_H_ */
