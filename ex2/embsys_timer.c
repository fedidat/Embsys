#include "embsys_timer.h"

/* Registers */
#define TIMER_COUNT 0x21
#define TIMER_CONTROL 0x22
#define TIMER_LIMIT 0x23 

/* Values */
#define INTERRUPT_DISABLE 0x0
#define INTERRUPT_ENABLE 0x1
#define INITIAL_VALUE 0x0

/* Interrupts */
int isPeriodic;
void (*pTimerCallback)(void);

/* Stores the callback */
void embsys_timer_init(void (*callback)())
{
	pTimerCallback = callback;
}

/* Starts the timer and saves flag for periodicity */
void embsys_timer_set(unsigned int cycles, int periodic)
{
	isPeriodic = periodic;
	_sr(cycles, TIMER_LIMIT);
	_sr(INITIAL_VALUE, TIMER_COUNT);
	_sr(INTERRUPT_ENABLE, TIMER_CONTROL);
}

/* Called after the timer expires, in interrupts are enabled */
_Interrupt1 void timer_isr()
{
	if(isPeriodic == 0)
		_sr(INTERRUPT_DISABLE, TIMER_CONTROL);
	else if(isPeriodic == 1)
		_sr(INTERRUPT_ENABLE, TIMER_CONTROL);
	(*pTimerCallback)();
}

