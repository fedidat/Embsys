#include "embsys_clock.h"
#include "embsys_7segments.h"
#include "embsys_uart.h"

int main()
{
	int time = 0, last = 0, C = 0, T = 100;
	char c;
	embsys_uart_init();
	while(1)
	{
		time = embsys_clock_read();
		if(time - last >= T || (time < last && time > T))
		{
			C = (C + 1) % 256;
			embsys_7segments_write(C);
			last = time;
		}
		if(embsys_uart_receive(&c))
			embsys_uart_send(c);
	}
	return 0;
}
