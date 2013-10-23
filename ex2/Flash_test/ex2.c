#include "embsys_uart.h"
#include "embsys_flash.h"
#include "embsys_timer.h"

void flash_interrupt()
{
	embsys_uart_send('k');
}

int main()
{
	unsigned char c, car;
	int pos, recc = 0;
	embsys_uart_init();
	embsys_flash_init(&flash_interrupt);
	_enable();
	while(1) 
	{
		if(embsys_uart_receive(&c)) 
		{
			if(recc==0)
			{
				car = c;
				recc = 1;
			}
			else if(recc==1)
			{
				pos = (int)c;
				recc = 2;
			}
			else
			{
				pos |= (int)c << 8;
				if(car == 'R')
				{
					embsys_flash_read(pos, 1, &car);
					embsys_uart_send(car);
				}
				else if(car == 'D')
				{
					embsys_flash_delete_all();
					embsys_uart_send('D');
				}
				else if(car == 'd')
				{
					embsys_flash_delete(pos);
					embsys_uart_send('d');
				}
				else
					embsys_flash_write(pos, 1, &car);
				recc = 0;
			}
			
		}
	}
	return 0;
}

