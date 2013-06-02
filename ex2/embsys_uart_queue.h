#ifndef EMBSYS_UART_QUEUE_H_
#define EMBSYS_UART_QUEUE_H_

struct QueueRecord 
{
	int Front;
	int Rear;
	int Size;
	int Capacity;
	unsigned char* Array;
};
typedef struct QueueRecord Queue;

/* Stores the array pointer */
Queue* init_queue(Queue* Q, unsigned char* queueArray, int arraySize);

/* Returns the next value */
int next(int Value, Queue* Q);

/* Enqueue one character */
void enqueue(unsigned char X, Queue* Q);

/* Enqueue one or several characters */
void enqueue_string(char* X, unsigned int length, Queue* Q);

/* Dequeue one character */
void dequeue(Queue* Q);

/* Indicates if the queue is full */
int is_full(Queue* Q);

/* Indicates if the queue is empty */
int is_empty(Queue* Q);

/* Returns the front character */
char front(Queue* Q);

#endif /*EMBSYS_UART_QUEUE_H_*/

