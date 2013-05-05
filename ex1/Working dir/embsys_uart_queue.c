#include "embsys_uart_queue.h"

Queue* InitQueue(Queue* Q) 
{
	/*init queue: size 0, front 1, rear 0*/
	Q->Size = 0;
	Q->Front = 1;
	Q->Rear = 0;
	return Q;
}

int Next(int Value, Queue* Q) 
{
	/*returns the place of the next elements in queue*/
	if (++Value == QueueSize)
		Value = 0;
	return Value;
}

void Enqueue(unsigned char X, Queue* Q) 
{
	/*if there is place available, advance rear, increment size and save element*/
	if(Q->Size == QueueSize)
		return;
	Q->Size++;
	Q->Rear = Next(Q->Rear, Q);
	Q->Array[Q->Rear] = X;
}

void EnqueueString(char* X, Queue* Q)
{
	/*enqueue elements one by one*/
	int i;
	for(i=0; X[i]; i++)
		Enqueue((unsigned char)X[i], Q);
}

void Dequeue(Queue* Q) 
{
	/*reverse of enqueue*/
	if(!Q->Size)
		return;
	Q->Size--;
	Q->Front = Next(Q->Front, Q);
}

