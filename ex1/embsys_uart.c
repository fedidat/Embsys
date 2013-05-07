#include "embsys_uart.h"

uart_registers *uart;

void embsys_uart_init() 
{
	/*get uart memory-mapping address*/
	uart = (uart_registers *) 0x100000;

	/*set baud rate to 19200bps*/
	uart->lcr.bits.dlab = 1;
	/*115200bps / 19200 = 0x06*/
	uart->thr_rbr_dll = 6; 
	uart->ier_dlh = 0;

	/*set link parameters: 8 bits data, no parity, 1 stop bit, no autoflow control*/
	uart->lcr.bits.dlab = 0;
	uart->ier_dlh = 0;
	uart->iir_fcr = 0;
	uart->mcr.mcr_val = 0;
	uart->lcr.bits.word_length = 3;
	uart->lcr.bits.stop_bit = 0;
	uart->lcr.bits.parity = 0;
	uart->mcr.bits.autoflow_control = 0;

}

unsigned char embsys_uart_receive(unsigned char* byte) 
{
	/*check if data received, and if yes, save byte*/
	if(!uart->lsr.bits.data_ready)
		return 0;
	else
	{
		*byte = uart->thr_rbr_dll;
		return 1;
	}
}

int embsys_uart_send(unsigned char byte) 
{
	/*check if line available, and if yes, write byte*/
	if(!uart->lsr.bits.thr_emp)
		return 0;
	else
	{
		uart->thr_rbr_dll = byte;
		return 1;
	}
}
