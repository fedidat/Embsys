#include "embsys_timer.h"

/* Registers */
#define TIMER_COUNT 0x21
#define TIMER_CONTROL 0x22
#define TIMER_LIMIT 0x23 

/* Values */
#define INTERRUPT_ENABLE 0x1
#define INITIAL_VALUE 0x0

void (*pTimerCallback)(void);

void embsys_timer_init(void (*callback)())
{
	pTimerCallback = callback;
}

void embsys_timer_set(unsigned int cycles)
{
	_sr(cycles, TIMER_LIMIT);
	_sr(INITIAL_VALUE, TIMER_COUNT);
	_sr(INTERRUPT_ENABLE, TIMER_CONTROL);
}

_Interrupt1 void timer_isr()
{
	_sr(INTERRUPT_ENABLE, TIMER_CONTROL);
	(*pTimerCallback)();
}

