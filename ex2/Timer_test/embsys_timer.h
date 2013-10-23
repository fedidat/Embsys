#ifndef EMBSYS_TIMER_H_
#define EMBSYS_TIMER_H_

void embsys_timer_init(void (*callback)());

void embsys_timer_set(unsigned int cycles, int periodic);

_Interrupt1 void timer_isr();

#endif /*EMBSYS_TIMER_H_*/

