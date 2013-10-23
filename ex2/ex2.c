#include "embsys_utilities.h"
#include "embsys_7segments.h"
#include "embsys_uart.h"
#include "embsys_uart_queue.h"
#include "embsys_flash.h"
#include "embsys_timer.h"

#define MAX_MESSAGE_SIZE 1024+5
#define MAX_QUEUED_MESSAGES 5
#define CYCLES_PER_UNIT 100000

/* protoypes */
void embsys_init();
void receive_request_byte(char c);
int process_request(unsigned char* request);
void process_queue_head();
void send_response_byte();
void uart_interrupt();
void timer_interrupt();
void flash_interrupt();
void flash_read_interrupt(unsigned char* buffer, int count);

/* global variables */
unsigned char receiveBuffer[MAX_MESSAGE_SIZE];
int bytesReceived;
unsigned char receiveArray[MAX_QUEUED_MESSAGES * MAX_MESSAGE_SIZE];
unsigned char sendArray[MAX_QUEUED_MESSAGES * MAX_MESSAGE_SIZE];
Queue receiveQueue;
Queue sendQueue;

int main()
{
	embsys_init();
	bytesReceived = 0;
	while(1)
		send_response_byte();
	return 0;
}

/* Initialize queues and extensions and enable interrupts */
void embsys_init()
{
	init_queue(&receiveQueue, receiveArray, MAX_QUEUED_MESSAGES * MAX_MESSAGE_SIZE);
	init_queue(&sendQueue, sendArray, MAX_QUEUED_MESSAGES * MAX_MESSAGE_SIZE);
	embsys_uart_init(&uart_interrupt);
	embsys_timer_init(&timer_interrupt);
	embsys_flash_init(&flash_interrupt, &flash_read_interrupt);
	_enable();
}

/* interrupt functions */
void uart_interrupt()
{
	unsigned char receivedChar;
	embsys_uart_receive(&receivedChar);
	receive_request_byte(receivedChar);
}
void timer_interrupt()
{
	enqueue_string("TE.", 3, &sendQueue);
}
void flash_interrupt()
{
	if(!is_empty(&receiveQueue))
		process_queue_head();
}
void flash_read_interrupt(unsigned char* buffer, int count)
{
	enqueue_string("RF", 2, &sendQueue);
	for(int i=0; i<count; i++)
	{
		unsigned char upperHex = decToHex((unsigned int)buffer[i] >> 4);
		enqueue(upperHex, &sendQueue);
		unsigned char lowerHex = decToHex((unsigned int)buffer[i] & 0x0F);
		enqueue(lowerHex, &sendQueue);
	}
	enqueue_string(".", 1, &sendQueue);
}

/*Receive and handle one request character */
void receive_request_byte(char receivedChar)
{
	receiveBuffer[bytesReceived++] = receivedChar;
	if(bytesReceived == MAX_MESSAGE_SIZE + 5)
	{
		/* queue up error message because too long */
		enqueue_string("ZZ.", 3, &sendQueue);
		bytesReceived = 0;
	}
	else if(receiveBuffer[bytesReceived - 1] == '.')
	{
		if(!process_request(receiveBuffer))
		{
			int i, requestsInQueue;
			for(i=0, requestsInQueue=0; i<receiveQueue.Size; i++)
				if(receiveQueue.Array[i] == '.')
					requestsInQueue++;
			if(requestsInQueue >= 5) /* Too many requests waiting in queue */
				enqueue_string("ZZ.", 3, &sendQueue);
			else /* enqueue request in waiting queue */
				enqueue_string((char*)receiveBuffer, bytesReceived, &receiveQueue);
		}
		bytesReceived = 0;
	}
}

/* Process head of waiting queue */
void process_queue_head()
{		
	/* try to process request at front */
	unsigned char request[MAX_MESSAGE_SIZE];
	int i;
	for(i=0; receiveQueue.Array[i] != '.'; i++)
		request[i] = receiveQueue.Array[i];
	if(!process_request(request))
	{
		while(front(&receiveQueue) != '.')
			dequeue(&receiveQueue);
		dequeue(&receiveQueue);
	}
}

/* Process a full request according to protocol */
int process_request(unsigned char* request)
{
	if(request[0] == 'R' && request[1] == 'F' && isCharHex(request[2]) && isCharHex(request[3])
		&& isCharHex(request[4]) && isCharHex(request[5]) && isCharHex(request[6]) && isCharHex(request[7])
		&& isCharHex(request[8]) && isCharHex(request[9]) && request[10] == '.')
	{
		if(embsys_flash_busy())
			return 0;
		/* get address and size and read flash */
		unsigned int startAddr = hexToDec(request[2]) << 12 | hexToDec(request[3]) << 8
			| hexToDec(request[4]) << 4 | hexToDec(request[5]);
		unsigned int size = hexToDec(request[6]) << 12 | hexToDec(request[7]) << 8
			| hexToDec(request[8]) << 4 | hexToDec(request[9]);
		embsys_flash_read(startAddr, size);
		return 1;
	}
	if(request[0] == 'W' && request[1] == 'F' && isCharHex(request[2]) && isCharHex(request[3]))
	{
		if(embsys_flash_busy())
			return 0;
		/* get address, check buffer integrity, then write flash */
		unsigned int startAddr = hexToDec(request[2]) << 12 | hexToDec(request[3]) << 8
			| hexToDec(request[4]) << 4 | hexToDec(request[5]);
		int count;
		unsigned char buffer[512];
		for(count = 0; request[6+count*2] != '.'; count++)
		{
			if(!isCharHex(request[6+count*2]) || !isCharHex(request[6+count*2+1]))
			{
				enqueue_string("ZZ.", 3, &sendQueue);
				return 1;
			}
			int a = hexToDec(request[6+count*2]) << 4, b= hexToDec(request[6+count*2+1]);
			buffer[count] = a | b;
		}
		if(request[6+count*2] == '.')
		{
			embsys_flash_write(startAddr, count, buffer);
			return 1;
		}
	}
	if(request[0] == 'E' && request[1] == 'F' && request[2] == '.')
	{
		if(embsys_flash_busy())
			return 0;
		/* erase flash */
		embsys_flash_delete_all();
		return 1;
	}
	if(request[0] == 'E' && request[1] == 'B' && isCharHex(request[2]) && isCharHex(request[3])
		&& isCharHex(request[4]) && isCharHex(request[5]) && request[6] == '.')
	{
		if(embsys_flash_busy())
			return 0;
		/* get start address then erase block */
		unsigned int startAddr = hexToDec(request[2]) << 12 | hexToDec(request[3]) << 8
			| hexToDec(request[4]) << 4 | hexToDec(request[5]);
		embsys_flash_delete(startAddr);
		return 1;
	}
	if(request[0] == 'S' && request[1] == 'S' && isCharHex(request[2]) && isCharHex(request[3])
		&& isCharHex(request[4]) && isCharHex(request[5]) && request[6] == '.')
	{
		/* seven segments */
		char MSB = hexToDec(request[2]) << 4 | hexToDec(request[3]);
		char LSB = hexToDec(request[4]) << 4 | hexToDec(request[5]);
		embsys_7segments_write(MSB, LSB);
		return 1;
	}
	if(request[0] == 'S' && request[1] == 'T' && isCharHex(request[2]) && isCharHex(request[3])
		&& isCharHex(request[4]) && isCharHex(request[5]) &&request[8] == '.')
	{
		/* set timer */
		int cycles = (char)(hexToDec(request[2]) << 12 | hexToDec(request[3]) << 8
			| hexToDec(request[4]) << 4 | hexToDec(request[5]));
		unsigned int isPeriodic = (unsigned int)(hexToDec(request[6]) << 4 | hexToDec(request[7]));
		if(isPeriodic == 0 || isPeriodic == 1)
		{
			embsys_timer_set(cycles * CYCLES_PER_UNIT, isPeriodic);
			return 1;
		}
	}
	enqueue_string("ZZ.", 3, &sendQueue);
	return 1;
}

/* Send one response character out of the sending queue */
void send_response_byte()
{
	/* if queue not empty, try to send next byte and dequeue byte if successful */
	if(!is_empty(&sendQueue))
		if(embsys_uart_send(front(&sendQueue)))
			dequeue(&sendQueue);
}

