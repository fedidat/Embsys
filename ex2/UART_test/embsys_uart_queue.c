#include "embsys_uart_queue.h"

Queue* InitQueue(Queue* Q, unsigned char* queueArray, int arraySize) 
{
	/*init queue: size 0, front 1, rear 0*/
	Q->Size = 0;
	Q->Front = 0;
	Q->Rear = 0;
	Q->Capacity = arraySize;
	Q->Array = queueArray;
	return Q;
}

int Next(int Value, Queue* Q) 
{
	/*returns the place of the next elements in queue*/
	if(++Value == Q->Capacity)
		Value = 0;
	return Value;
}

void Enqueue(unsigned char X, Queue* Q) 
{
	/*if there is place available, advance rear, increment size and save element*/
	if(IsFull(Q))
		return;
	Q->Size++;
	Q->Array[Q->Rear] = X;
	Q->Rear = Next(Q->Rear, Q);
}

void EnqueueString(char* X, unsigned int length, Queue* Q)
{
	/*enqueue elements one by one*/
	int i;
	for(i=0; i<length; i++)
		Enqueue((unsigned char)X[i], Q);
}

void Dequeue(Queue* Q) 
{
	/*reverse of enqueue*/
	if(IsEmpty(Q))
		return;
	Q->Size--;
	Q->Front = Next(Q->Front, Q);
}

int IsFull(Queue* Q) 
{
	return Q->Size == Q->Capacity;
}

int IsEmpty(Queue* Q) 
{
	return Q->Size == 0;
}

char Front(Queue* Q)
{
	return Q->Array[Q->Front];
}

