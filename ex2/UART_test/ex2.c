#include "embsys_uart.h"
#include "embsys_uart_queue.h"

/* DEFINES */
#define MAX_MESSAGE_SIZE 517
#define MAX_QUEUED_MESSAGES 5

/* VARIABLES */
unsigned char receiveBuffer[MAX_MESSAGE_SIZE];
int bytesReceived;
unsigned char sendArray[MAX_QUEUED_MESSAGES * MAX_MESSAGE_SIZE];
Queue sendQueue;

/* INTERRUPTS */
void uart_interrupt()
{
	unsigned char receivedChar;
	embsys_uart_receive(&receivedChar);
	bytesReceived++;
	Enqueue((char)(receivedChar+1), &sendQueue);
}

int main()
{
	InitQueue(&sendQueue, sendArray, MAX_QUEUED_MESSAGES * MAX_MESSAGE_SIZE);
	embsys_uart_init(&uart_interrupt);
	_enable();
	bytesReceived = 0;
	while(1)
		if(!IsEmpty(&sendQueue) && embsys_uart_send(Front(&sendQueue)))
			Dequeue(&sendQueue);
	return 0;
}

