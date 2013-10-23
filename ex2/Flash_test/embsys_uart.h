#ifndef EMBSYS_UART_H_
#define EMBSYS_UART_H_

void embsys_uart_init();

unsigned char embsys_uart_receive(unsigned char* byte);

int embsys_uart_send(unsigned char byte);

#endif /*EMBSYS_UART_H_*/

