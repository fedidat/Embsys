#ifndef EMBSYS_UART_H_
#define EMBSYS_UART_H_

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

void embsys_uart_init();

unsigned char embsys_uart_receive(unsigned char* byte);

int embsys_uart_send(unsigned char byte);

#endif /*EMBSYS_UART_H_*/

