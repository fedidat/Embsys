#include "embsys_7segments.h"
#include "embsys_uart.h"
#include "embsys_uart_queue.h"
#include "embsys_flash.h"
#include "embsys_timer.h"

/* DEFINES */
#define MAX_MESSAGE_SIZE 517
#define MAX_QUEUED_MESSAGES 5
#define CYCLES_PER_UNIT 15000
#define FLASH_BLOCK_SIZE 4000

/* UTILITIES */
unsigned char decToHex(int num);
int hexToDec(unsigned char c);
int isCharHex(unsigned char receiveBuffer);

/* PROTOCOL */
void embsys_init();
void receive_request_byte(char c);
int process_request(unsigned char* request);
void process_queue_head();
void send_response_byte();

/* VARIABLES */
unsigned char receiveBuffer[MAX_MESSAGE_SIZE];
int bytesReceived;
unsigned char receiveArray[MAX_QUEUED_MESSAGES * MAX_MESSAGE_SIZE];
unsigned char sendArray[MAX_QUEUED_MESSAGES * MAX_MESSAGE_SIZE];
Queue receiveQueue;
Queue sendQueue;

/* INTERRUPTS */
void uart_interrupt()
{
	unsigned char receivedChar;
	embsys_uart_receive(&receivedChar);
	receive_request_byte(receivedChar);
}
void timer_interrupt()
{
	EnqueueString("TE.", 3, &sendQueue);
}
void flash_interrupt()
{
	//???
}

int main()
{
	embsys_init();
	bytesReceived = 0;
	while(1)
	{
		if(!IsEmpty(&receiveQueue))
			process_queue_head();
		send_response_byte();
	}
	return 0;
}

void embsys_init()
{
	InitQueue(&receiveQueue, receiveArray, MAX_QUEUED_MESSAGES * MAX_MESSAGE_SIZE);
	InitQueue(&sendQueue, sendArray, MAX_QUEUED_MESSAGES * MAX_MESSAGE_SIZE);
	embsys_uart_init(&uart_interrupt);
	embsys_timer_init(&timer_interrupt);
	embsys_flash_init(&flash_interrupt);
}

void receive_request_byte(char receivedChar)
{
	receiveBuffer[bytesReceived++] = receivedChar;
	if(bytesReceived == 517)
	{
		/* queue up error message because too long */
		EnqueueString("ZZ.", 3, &sendQueue);
		bytesReceived = 0;
	}
	else if(receiveBuffer[bytesReceived] == '.')
	{
		process_request(receiveBuffer);
		bytesReceived = 0;
	}
}

int process_request(unsigned char* request)
{
	if(request[0] == 'R' && request[1] == 'F')
	{
		/* read flash */
		unsigned int startAddr = hexToDec(request[2]) << 4 | hexToDec(request[3]);
		unsigned int size = hexToDec(request[4]) << 4 | hexToDec(request[5]);
		unsigned char flashBuffer[512];
		embsys_flash_read(startAddr, size, flashBuffer);
		EnqueueString("RF", 2, &sendQueue);
		EnqueueString((char*)flashBuffer, size, &sendQueue);
		EnqueueString(".", 1, &sendQueue);
		return 1;
	}
	if(request[0] == 'W' && request[1] == 'F')
	{
		/* write flash */
		unsigned int startAddr = hexToDec(request[2]) << 4 | hexToDec(request[3]);
		int count;
		for(count = 0; request[4+count] != '.'; count++);
		embsys_flash_write(startAddr, count, &request[4]);
		return 1;
	}
	if(request[0] == 'E' && request[1] == 'F')
	{
		/* erase flash */
		embsys_flash_delete_all();
		return 1;
	}
	if(request[0] == 'E' && request[1] == 'B')
	{
		/* erase block */
		unsigned int position = hexToDec(request[2]) << 4 | hexToDec(request[3]);
		embsys_flash_delete(position * FLASH_BLOCK_SIZE);
		return 1;
	}
	if(request[0] == 'S' && request[1] == 'S')
	{
		/* seven segments */
		char value = (char)(hexToDec(request[2]) << 4 | hexToDec(request[3]));
		embsys_7segments_write(value);
		return 1;
	}
	if(request[0] == 'S' && request[1] == 'T')
	{
		/* set timer */
		char cycles = (char)(hexToDec(request[2]) << 4 | hexToDec(request[3]));
		unsigned int isPeriodic = (unsigned int)request[5];
		if(isPeriodic == 0 || isPeriodic == 1)
		{
			embsys_timer_set(cycles, isPeriodic);
			return 1;
		}
	}
	EnqueueString("ZZ.", 3, &sendQueue);
	return 1;
}

void process_queue_head()
{		
	/* try to process request at Front */
	unsigned char request[MAX_MESSAGE_SIZE];
	int i;
	for(i=0; receiveQueue.Array[i] != '.'; i++)
		request[i] = receiveQueue.Array[i];
	if(!process_request(request))
	{
		while(Front(&receiveQueue) != '.')
			Dequeue(&receiveQueue);
		Dequeue(&receiveQueue);
	}
}

void send_response_byte()
{
	/* if queue not empty, try to send next byte and dequeue byte if successful */
	if(!IsEmpty(&sendQueue))
		if(embsys_uart_send(Front(&sendQueue)))
			Dequeue(&sendQueue);
}
			
unsigned char decToHex(int num) 
{
	return ((num < 10)? ('0' + num) : ('A' + num - 10));
}

int hexToDec(unsigned char c)
{
	if(c > 'a' && c <= 'f')
		c = c + 'A' - 'a';
	return ((c >= 'A')? (c - 'A' + 10) : (c - '0'));
}

int isCharHex(unsigned char c)
{
	return ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'));
}

