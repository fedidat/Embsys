#include "embsys_uart.h"

#define UART_BASE_ADDRESS 0x100000

/* UART Registers */
typedef volatile union 
{
	unsigned char ier_val;
	struct 
	{
		unsigned char received_data_interrupt : 1;
		unsigned char thr_empty_interrupt : 1;
		unsigned char rls_interrupt : 1;
		unsigned char modem_status_interrupt : 1;
		unsigned char sleep_mode : 1;
		unsigned char low_power_mod : 1;
		unsigned char reserved : 2;
	} bits;
} ier_t;

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
	ier_t ier_dlh;
	unsigned char iir_fcr;
	lcr_t lcr;
	unsigned char mcr;
	lsr_t lsr;
	unsigned char msr;
	unsigned char sr;
} uart_registers;

/* Interrupts */
uart_registers *uart;
void (*pUARTCallback)(void);

/* Called upon receiving a character, as set in the init function */
void uart_isr()
{
	/* invoke callback */
	(*pUARTCallback)();
}

/* Stores the callback and the UART extension parameters */
void embsys_uart_init(void (*callback)()) 
{
	/* store callback */
	pUARTCallback = callback;
	
	/*get uart memory-mapping address*/
	uart = (uart_registers *) UART_BASE_ADDRESS;

	/* set baud rate to 19200bps */
	uart->lcr.bits.dlab = 1;
	uart->thr_rbr_dll = 6; /* 115200bps / 19200 = 0x06 */

	/* set link parameters: 8 bits data, no parity, 1 stop bit, no autoflow control */
	uart->lcr.bits.dlab = 0;
	uart->iir_fcr = 0;
	uart->lcr.bits.word_length = 3;
	uart->lcr.bits.stop_bit = 0;
	uart->lcr.bits.parity = 0;
	
	/* enable data-received interrupt */
	uart->lcr.bits.dlab = 0;
	uart->ier_dlh.bits.received_data_interrupt = 1;
}

/* Receives one character over UART */
void embsys_uart_receive(unsigned char* byte) 
{
	/* interrupt received, directly read from the buffer */
	*byte = uart->thr_rbr_dll;
}

/* Sends one character over UART */
int embsys_uart_send(unsigned char byte) 
{
	/*check if line available, and if it is, write byte*/
	if(!uart->lsr.bits.thr_emp)
		return 0;
	else
	{
		uart->thr_rbr_dll = byte;
		while(!uart->lsr.bits.dhr_emp);
		return 1;
	}
}

