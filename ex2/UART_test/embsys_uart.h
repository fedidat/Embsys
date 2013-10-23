#ifndef EMBSYS_UART_H_
#define EMBSYS_UART_H_

void embsys_uart_init(void (*callback)());

void embsys_uart_receive(unsigned char* byte);

int embsys_uart_send(unsigned char byte);

_Interrupt1 void uart_isr();

#endif /*EMBSYS_UART_H_*/

