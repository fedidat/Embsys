#include "embsys_uart_queue.h"

/* initialize the attributes of the queue */
void embsys_uart_queue_init(uart_queue* const pQueue)
{
	pQueue->index = 0;
	pQueue->size = 0;
}

/* enqueue one character into the queue */
void enqueue(uart_queue* const pQueue, const unsigned char data)
{   
	if(pQueue == NULL || pQueue->size == BUFFER_SIZE)
		return;

	pQueue->data[pQueue->index] = data;
	pQueue->size++;
	pQueue->index = (pQueue->index+1)%BUFFER_SIZE; 
}

/* enqueue a string of characters into the queue */
void enqueueString(uart_queue* const pQueue, char* data, unsigned int count)
{   
	for(int i=0; i<count; i++)
		enqueue(pQueue, (unsigned char)data[i]);
}

/* dequeue one character from the queue and returns it */
unsigned char dequeue(uart_queue* const pQueue)
{
	if(pQueue == NULL || pQueue->size == 0)
		return NULL;
		
    /* adding BUFFER_SIZE makes sure the result is positive in all cases */
	unsigned char result = pQueue->data[(pQueue->index - pQueue->size + BUFFER_SIZE) % BUFFER_SIZE];
	pQueue->size--;
	return result;
}

