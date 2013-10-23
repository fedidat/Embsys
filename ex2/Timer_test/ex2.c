#include "embsys_uart.h"
#include "embsys_timer.h"

void embsys_timer_expired()
{
	embsys_uart_send('A');
}

int main()
{
	embsys_uart_init();
	embsys_timer_init(&embsys_timer_expired);
	embsys_timer_set(1500000);
	
	_enable();

	while(1);
	return 0;
}

