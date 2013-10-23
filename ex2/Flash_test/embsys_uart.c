#include "embsys_uart.h"

typedef volatile union 
{
	unsigned char lcr_val;
	struct 
	{
		unsigned char word_length : 2;
		unsigned char stop_bit : 1;
		unsigned char parity : 3;
		unsigned char set_break : 1;
		unsigned char dlab : 1;

	} bits;
} lcr_t;

typedef volatile union 
{
	unsigned char mcr_val;
	struct 
	{
		unsigned char data_terminal_ready : 1;
		unsigned char request_to_send : 1;
		unsigned char aux_out1 : 1;
		unsigned char aux_out2 : 1;
		unsigned char loopback_mode : 1;
		unsigned char autoflow_control : 1;
		unsigned char reserved : 2;
	} bits;
} mcr_t;

typedef volatile union 
{
	unsigned char lsr_val;
	struct 
	{
		unsigned char data_ready : 1;
		unsigned char overrun_err : 1;
		unsigned char parity_err : 1;
		unsigned char framing_err : 1;
		unsigned char break_interrupt : 1;
		unsigned char thr_emp : 1;
		unsigned char dhr_emp : 1;
		unsigned char err_fifo : 1;
	} bits;
} lsr_t;

typedef volatile struct
{
	unsigned char thr_rbr_dll;
	unsigned char ier_dlh;
	unsigned char iir_fcr;
	lcr_t lcr;
	mcr_t mcr;
	lsr_t lsr;
	unsigned char msr;
	unsigned char sr;
} uart_registers;

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

