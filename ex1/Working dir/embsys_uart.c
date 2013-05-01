#include "embsys_uart.h"

uart_registers *uart;

void embsys_uart_init() {
	//19200bps, 8 bits data, no parity, 1 stop bit, no autoflow control
	uart = (uart_registers *) 0x100000;

	uart->lcr.bits.dlab = 1;
	uart->thr_rbr_dll = 6; // 115200/19200 = 0x06
	uart->ier_dlh = 0;

	uart->lcr.bits.dlab = 0;
	uart->ier_dlh = 0;
	uart->iir_fcr = 0;
	uart->mcr.mcr_val = 0;
	uart->lcr.bits.word_length = 3;
	uart->lcr.bits.stop_bit = 0;
	uart->lcr.bits.parity = 0;
	uart->mcr.bits.autoflow_control = 0;

}

unsigned char embsys_uart_receive(unsigned char* byte) {
	if(!uart->lsr.bits.data_ready)
		return 0;
	else
	{
		*byte = uart->thr_rbr_dll;
		return 1;
	}
}

unsigned char embsys_uart_send(unsigned char byte) {
	if(!uart->lsr.bits.thr_emp)
		return 0;
	else
	{
		uart->thr_rbr_dll = byte;
		return 1;
	}
}
