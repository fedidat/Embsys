#include "embsys_sms_slave.h"
#include "embsys_uart.h"
#include "embsys_uart_queue.h"

/* declarations for the UART send thread */
#define UART_SEND_THREAD_STACK_SIZE (1024)
#define UART_SEND_PRIORITY (1)
TX_THREAD gUartSendThread;
CHAR gUartSendThreadStack[UART_SEND_THREAD_STACK_SIZE];
TX_EVENT_FLAGS_GROUP gUartSendEventFlags;
void uart_interrupt();
void send_next_uart();
void UartSendThreadMainFunc(ULONG v);

/* the send and receive UART buffers (cyclic queues) */
uart_queue requestBuffer, replyBuffer;

/* initialization */
unsigned int embsys_sms_slave_init()
{
	TX_STATUS status;		
	status = tx_event_flags_create(&gUartSendEventFlags,"Uart send Event Flags");
	if(status != TX_SUCCESS)
    	return status;
    status = tx_thread_create(&gUartSendThread, "Uart send Thread", UartSendThreadMainFunc,
                              0, gUartSendThreadStack, UART_SEND_THREAD_STACK_SIZE, UART_SEND_PRIORITY,
                              UART_SEND_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);
    if(status != TX_SUCCESS) 
    	return status;

	embsys_uart_init(&uart_interrupt);
	embsys_uart_queue_init(&requestBuffer);
	embsys_uart_queue_init(&replyBuffer);
	return 0;
}

/* main loop for the UART send thread */
void UartSendThreadMainFunc(ULONG v)
{
    ULONG actualFlag;

    while(true)
    {
            //wait for a flag wake the thread up
            tx_event_flags_get(&gUartSendEventFlags, 1, TX_AND, &actualFlag, TX_WAIT_FOREVER);

            //send next letter
            send_next_uart();
    }
}

/* the UART send thread is only put to rest when the send buffer has been emptied */
void send_next_uart()
{
    if(replyBuffer.size)
    	embsys_uart_send(dequeue(&replyBuffer));
    else
    	tx_event_flags_set(&gUartSendEventFlags, 0, TX_AND);
}

/* UART interrupt function, enters characters into the buffer */
void uart_interrupt()
{
	unsigned char receivedChar;
	embsys_uart_receive(&receivedChar);
	enqueue(&requestBuffer, receivedChar);
	if(receivedChar == '.')
		handleRequest();
	else if(requestBuffer.size == BUFFER_SIZE)
		requestBuffer.size = 0; /* clear queue if full, to avoid critical problems */
}

/* when a period comes, the request is processed here */
void handleRequest()
{
	char firstChar = dequeue(&requestBuffer), secondChar = dequeue(&requestBuffer);
	if(firstChar == 'M' && secondChar == 'C')
		handleMessageCount();
	else if(firstChar == 'R' && secondChar == 'M')
		handleReadMessage();
	else if(firstChar == 'D' && secondChar == 'E')
		handleDeleteMessage();
	else if(firstChar == 'S' && secondChar == 'M')
		handleSendMessage();
	else if(firstChar == 'U' && secondChar == 'M')
		handleUpdateMessage();
	else
		sendInvalidRequestReply();
	
	/* then enqueue a period, flush the remainder of the request, and notify the send uart thread */
	enqueue(&replyBuffer, '.');
	while(dequeue(&requestBuffer) != '.');
	tx_event_flags_set(&gUartSendEventFlags,1,TX_OR);
}

/* returns the count of messages */
void handleMessageCount()
{
	int size = getSmsCount();
	enqueueString(&replyBuffer, "MC", 2);
	enqueue(&replyBuffer, decToHex(size/16));
	enqueue(&replyBuffer, decToHex(size%16));
}

/* reads the data of a message into the send buffer */
void handleReadMessage()
{
	char upperNibble = dequeue(&requestBuffer), lowerNibble = dequeue(&requestBuffer);
	unsigned int index = hexToDec(upperNibble)*16 + hexToDec(lowerNibble);
	pSmsNode_t pSelectedSms = getSmsWithIndex(index);
	
	if(pSelectedSms == NULL)
	{
		sendInvalidRequestReply();
		return;
	}

	enqueueString(&replyBuffer, "MC", 2);
	enqueue(&replyBuffer, upperNibble);
	enqueue(&replyBuffer, lowerNibble);
	
	if (pSelectedSms->type == INCOMING_MESSAGE)
	{
		unsigned int srcLen, i;
		SMS_DELIVER* pMsg = (SMS_DELIVER*)pSelectedSms->pSMS;
			
		/* fetch timestamp time */
		enqueue(&replyBuffer, pMsg->timestamp[7]);
		enqueue(&replyBuffer, pMsg->timestamp[6]);
		enqueue(&replyBuffer, pMsg->timestamp[9]);
		enqueue(&replyBuffer, pMsg->timestamp[8]);
		enqueue(&replyBuffer, pMsg->timestamp[11]);
		enqueue(&replyBuffer, pMsg->timestamp[10]);

		/* fetch destination */
		enqueue(&replyBuffer, decToHex(DEVICE_ID_LENGTH/16));
		enqueue(&replyBuffer, decToHex(DEVICE_ID_LENGTH%16));
		char dest[] = DEVICE_ID;
		for(i=0 ; i<DEVICE_ID_LENGTH ; ++i)
		{
			enqueue(&replyBuffer, '0');
			enqueue(&replyBuffer, dest[i]);
		}

		/* fetch source */
		for(srcLen=0; (pMsg->sender_id[srcLen] != (char)NULL) && (srcLen < ID_MAX_LENGTH); ++srcLen);
		enqueue(&replyBuffer, decToHex(srcLen/16));
		enqueue(&replyBuffer, decToHex(srcLen%16));
		for(i=0 ; i<srcLen ; ++i)
		{
			enqueue(&replyBuffer, '0');
			enqueue(&replyBuffer, pMsg->sender_id[i]);
		}

		/* fetch SMS message data */
		unsigned int dataLength =pMsg->data_length;
		enqueue(&replyBuffer, decToHex(dataLength/16));
		enqueue(&replyBuffer, decToHex(dataLength%16));
		for(i=0 ; i<dataLength ; ++i)
		{
			enqueue(&replyBuffer, decToHex(pMsg->data[i]/16));
			enqueue(&replyBuffer, decToHex(pMsg->data[i]%16));
		}
	}
	else /* OUTGOING_MESSAGE */
	{
		unsigned int destLen, i;
		SMS_SUBMIT* pMsg = (SMS_SUBMIT*)pSelectedSms->pSMS;
			
		/* no timestamp, send zeros instead (see README) */
		enqueueString(&replyBuffer, "000000", 6);

		/* fetch destination */
		for(destLen=0; (pMsg->recipient_id[destLen] != (char)NULL) && (destLen < ID_MAX_LENGTH); ++destLen);
		enqueue(&replyBuffer, decToHex(destLen/16));
		enqueue(&replyBuffer, decToHex(destLen%16));
		for(i=0 ; i<destLen ; ++i)
		{
			enqueue(&replyBuffer, '0');
			enqueue(&replyBuffer, pMsg->recipient_id[i]);
		}
		
		/* fetch source */
		enqueue(&replyBuffer, decToHex(DEVICE_ID_LENGTH/16));
		enqueue(&replyBuffer, decToHex(DEVICE_ID_LENGTH%16));
		char src[] = DEVICE_ID;
		for(i=0 ; i<DEVICE_ID_LENGTH ; ++i)
		{
			enqueue(&replyBuffer, '0');
			enqueue(&replyBuffer, src[i]);
		}

		/* fetch SMS message data */
		unsigned int dataLength = pMsg->data_length;
		enqueue(&replyBuffer, decToHex(dataLength/16));
		enqueue(&replyBuffer, decToHex(dataLength%16));
		for(i=0 ; i<dataLength ; ++i)
		{
			enqueue(&replyBuffer, decToHex(pMsg->data[i]/16));
			enqueue(&replyBuffer, decToHex(pMsg->data[i]%16));
		}
	}
}

/* delete a message from the device's database */
void handleDeleteMessage()
{
	unsigned int index = hexToDec(dequeue(&requestBuffer))*16 + hexToDec(dequeue(&requestBuffer));
	unsigned int result = deleteSmsWithIndex(index);
	
	/*if success, refresh screen*/
	if(result == 0)
	{
		setRefreshDisplay();
		refreshDisplay();
	}
	
	/*enqueue reply*/
	enqueueString(&replyBuffer, "DE", 2);
	enqueue(&replyBuffer, '0');
	enqueue(&replyBuffer, decToHex(result));
}

/* sends an SMS */
void handleSendMessage()
{
	enqueueString(&replyBuffer, "SM", 2);
	enqueue(&replyBuffer, '0');
	
	/*check destination length then fetch destination*/
	unsigned int destLen = hexToDec(dequeue(&requestBuffer))*16 + hexToDec(dequeue(&requestBuffer));
	if(destLen < 1 || destLen > 12)
	{
		enqueue(&replyBuffer, '1');
		return;
	}
	SMS_SUBMIT smsToSend;
	for(int i=0; i<destLen; i++)
	{
		dequeue(&requestBuffer);
		smsToSend.recipient_id[i] = dequeue(&requestBuffer);
	}
	
	/*check data length then fetch message content*/
	smsToSend.data_length = hexToDec(dequeue(&requestBuffer))*16 + hexToDec(dequeue(&requestBuffer));
	if(smsToSend.data_length > DATA_MAX_LENGTH)
	{
		enqueue(&replyBuffer, '1');
		return;
	}
	for(i=0; i<smsToSend.data_length; i++)
		smsToSend.data[i] = (char)(hexToDec(dequeue(&requestBuffer))*16 + hexToDec(dequeue(&requestBuffer)));
	
	/*send SMS and enqueue success indicator*/
	sendSmsAndRefresh(&smsToSend);
	enqueue(&replyBuffer, '0');
}

/* updates a message's data field */
void handleUpdateMessage()
{	
	enqueueString(&replyBuffer, "UM", 2);
	unsigned int index = hexToDec(dequeue(&requestBuffer))*16 + hexToDec(dequeue(&requestBuffer));
	
	/* fetch data length and check if allowed */
	unsigned int dataLen = hexToDec(dequeue(&requestBuffer))*16 + hexToDec(dequeue(&requestBuffer));
	if(dataLen > DATA_MAX_LENGTH)
	{
		enqueue(&replyBuffer, '1');
		return;
	}
	
	/* fetch data */
	char data[DATA_MAX_LENGTH];
	for(int i=0; i<dataLen; i++)
		data[i] = (char)(hexToDec(dequeue(&requestBuffer))*16 + hexToDec(dequeue(&requestBuffer)));
	
	/* process the request and enqueue the result */ 
	enqueue(&replyBuffer, '0');
	enqueue(&replyBuffer, updateSmsDataWithIndex(index, data, dataLen));
}

/* sends an invalid request reply "IR." */
void sendInvalidRequestReply()
{
	enqueueString(&replyBuffer, "IR", 2);
}

