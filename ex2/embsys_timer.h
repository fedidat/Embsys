#ifndef EMBSYS_TIMER_H_
#define EMBSYS_TIMER_H_

/* Stores the callback */
void embsys_timer_init(void (*callback)());

/* Starts the timer and saves flag for periodicity */
void embsys_timer_set(unsigned int cycles, int periodic);

#endif /*EMBSYS_TIMER_H_*/

