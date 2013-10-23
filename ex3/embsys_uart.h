#ifndef EMBSYS_UART_H_
#define EMBSYS_UART_H_

#include "embsys_utilities.h"

/* Stores the callback and the UART extension parameters */
void embsys_uart_init(void (*callback)());

/* Receives one character over UART */
void embsys_uart_receive(unsigned char* byte);

/* Sends one character over UART */
int embsys_uart_send(unsigned char byte);

#endif /*EMBSYS_UART_H_*/

