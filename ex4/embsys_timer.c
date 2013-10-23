#include "embsys_timer.h"

#define TIMER0_COUNT	0x21
#define TIMER0_CONTROL	0x22
#define TIMER0_LIMIT	0x23

typedef union
{
	unsigned int data;
	struct
	{
		unsigned int interrupt_enable  	:1;
		unsigned int not_halted  		:1;
		unsigned int watchdog 			:1;
		unsigned int interrupt_pending 	:1;
		unsigned int reserve 			:28;
	}bits;
}timer_control;

/* initialize timer 0 */
void embsys_timer_init(unsigned int interval)
{
	timer_control t;
	t.data = 0;
	t.bits.interrupt_enable = 1;
	t.bits.interrupt_pending = 0;
	t.bits.watchdog = 0;

	_sr(0, TIMER0_COUNT);
	_sr(CYCLES_IN_MS * interval, TIMER0_LIMIT);
	_sr(t.data, TIMER0_CONTROL);
}

