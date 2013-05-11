#include "embsys_7segments.h"
#include "embsys_uart.h"
#include "embsys_uart_queue.h"
#include "embsys_flash.h"
#include "embsys_pager.h"

typedef struct {
	unsigned char command;
	unsigned char variable;
	unsigned char value[2];
} message;

unsigned char decToHex(int num);
int hexToDec(unsigned char c);
int isCharHex(unsigned char receiveBuffer);
void process_request(message* receiveBuffer, Queue* sendQueue, int* C, int* T);
void send_response_byte(Queue* sendQueue);

int main()
{
	unsigned char receivedChar;
	int time = 0, last = 0, C = 0, T = 100, bytesReceived=0;
	unsigned char receiveBuffer[4];
	embsys_uart_init();
	Queue sendQueue;
	InitQueue(&sendQueue);
	while(1) 
	{
		time = 0;
		if(time - last >= T || (time < last && time > T)) 
		{
			C = (C + 1) % 256;
			embsys_7segments_write(C);
			last = time;
		}
		if(embsys_uart_receive(&receivedChar)) 
		{
			receiveBuffer[bytesReceived++] = receivedChar;
			if(bytesReceived == 4) 
			{
				process_request((message*)receiveBuffer, &sendQueue, &C, &T);
				Enqueue('\n', &sendQueue);
				bytesReceived = 0;
			}
		}
		send_response_byte(&sendQueue);
	}
	return 0;
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

void process_request(message* req, Queue* sendQueue, int* C, int* T)
{
	/*process the message, make changes if necessary and send response,
	only enqueue 'Z' if process does not complete*/
	if(req->command == 'r' && req->value[0] == '0' && req->value[1] == '0') 
	{
		if(req->variable == 'c') 
		{
			EnqueueString("RC", sendQueue);
			Enqueue(decToHex((*C >> 4) & 0x0F), sendQueue);
			Enqueue(decToHex(*C & 0x0F), sendQueue);
			return;
		}
		else if(req->variable == 't') 
		{
			EnqueueString("RT", sendQueue);
			Enqueue(decToHex((*T >> 4) & 0x0F), sendQueue);
			Enqueue(decToHex(*T & 0x0F), sendQueue);
			return;
		}
	}
	else if(req->command == 'w' && isCharHex(req->value[0]) && isCharHex(req->value[1])) 
	{
		int value = hexToDec(req->value[0])*16 + hexToDec(req->value[1]);
		if(req->variable == 'c') 
		{
			*C = value;
			embsys_7segments_write(*C);
			EnqueueString("WC00", sendQueue);
			return;
		}
		else if(req->variable == 't') 
		{
			*T = value;
			EnqueueString("WT00", sendQueue);
			return;
		}
	}
	Enqueue('Z', sendQueue);
}

void send_response_byte(Queue* sendQueue)
{
	/*if queue not empty, send next byte and dequeue byte*/
	if(sendQueue->Size) 
	{
		if(embsys_uart_send(Front(sendQueue)))
			Dequeue(sendQueue);
	}
}

