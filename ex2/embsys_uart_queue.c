#include "embsys_uart_queue.h"

/* Stores the array pointer: malloc doesn't work well */
Queue* init_queue(Queue* Q, unsigned char* queueArray, int arraySize) 
{
	/*init queue: size 0, front 1, rear 0*/
	Q->Size = 0;
	Q->Front = 0;
	Q->Rear = 0;
	Q->Capacity = arraySize;
	Q->Array = queueArray;
	return Q;
}

/* Returns the next value */
int next(int Value, Queue* Q) 
{
	/*returns the place of the next elements in queue*/
	if(++Value == Q->Capacity)
		Value = 0;
	return Value;
}

/* Enqueue one character */
void enqueue(unsigned char X, Queue* Q) 
{
	/*if there is place available, advance rear, increment size and save element*/
	if(is_full(Q))
		return;
	Q->Size++;
	Q->Array[Q->Rear] = X;
	Q->Rear = next(Q->Rear, Q);
}

/* Enqueue one or several characters */
void enqueue_string(char* X, unsigned int length, Queue* Q)
{
	/*enqueue elements one by one*/
	int i;
	for(i=0; i<length; i++)
		enqueue((unsigned char)X[i], Q);
}

/* Dequeue one character */
void dequeue(Queue* Q) 
{
	/*reverse of enqueue*/
	if(is_empty(Q))
		return;
	Q->Size--;
	Q->Front = next(Q->Front, Q);
}

/* Indicates if the queue is full */
int is_full(Queue* Q) 
{
	return Q->Size == Q->Capacity;
}

/* Indicates if the queue is empty */
int is_empty(Queue* Q) 
{
	return Q->Size == 0;
}

/* Returns the front character */
char front(Queue* Q)
{
	return Q->Array[Q->Front];
}

