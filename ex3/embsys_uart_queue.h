#ifndef EMBSYS_UART_QUEUE_H_
#define EMBSYS_UART_QUEUE_H_

#include "embsys_utilities.h"

#define BUFFER_SIZE 512

typedef struct
{
	int index;
	int size;		
	unsigned char data[BUFFER_SIZE];
}uart_queue;

/* initialize the attributes of the queue */
void embsys_uart_queue_init(uart_queue* const pQueue);

/* enqueue one character into the queue */
void enqueue(uart_queue* const pQueue, const unsigned char data);

/* enqueue a string of characters into the queue */
void enqueueString(uart_queue* const pQueue, char* data, unsigned int count);

/* dequeue one character from the queue and returns it */
unsigned char dequeue(uart_queue* const pQueue);

#endif /* EMBSYS_UART_QUEUE_H_ */

