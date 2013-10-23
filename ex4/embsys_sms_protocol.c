#include "embsys_sms_protocol.h"
#include "embsys_utilities.h"
#include "string.h"

/* Prototypes */
unsigned int deviceIdSize(char *pBuff);
unsigned int convStringToNibbleSwapped(char* str,char *swappedNibble,unsigned strSize);
unsigned int convNibbleSwappedToString(char* nibble,char* str,unsigned strSize);
unsigned int encodeAs7bitStr(char* str,char* encodedStr,unsigned size);
unsigned int decodeTo8bitStr(char* decodedStr,char* str,unsigned size);
char bottomNBits(char byte,unsigned n);

EMBSYS_STATUS embsys_fill_probe(char *buf, SMS_PROBE *msg_fields, char is_ack, unsigned *len)
{
	unsigned int size;

	if (is_ack == (char)NULL)
		*buf++ = 0x02;
	else
		*buf++ = 0x12;
	*buf++ = size = deviceIdSize(msg_fields->device_id);
	*buf++ = 0xc9;
	buf += convStringToNibbleSwapped(msg_fields->device_id,buf,size);

	*len = (3 + (size + (size%2==0?0:1))/2);

	if (is_ack != (char)NULL)
	{
		buf += convStringToNibbleSwapped(msg_fields->timestamp,buf,TIMESTAMP_MAX_LENGTH);
		*buf++ = size = deviceIdSize(msg_fields->sender_id);
		*buf++ = 0xc9;
		buf += convStringToNibbleSwapped(msg_fields->sender_id,buf,size);
		*len += (7 + 2 + (size + (size%2==0?0:1))/2);
	}
	return SUCCESS;
}

EMBSYS_STATUS embsys_fill_submit(char *buf, SMS_SUBMIT *msg_fields, unsigned *len)
{
	unsigned senderSize,recipientSize;
	char* origBuf = buf;

	*buf++ = 0x11;
	*buf++ = senderSize = deviceIdSize(msg_fields->device_id);
	*buf++ = 0xc9;
	buf+= convStringToNibbleSwapped(msg_fields->device_id,buf,senderSize);
	*buf++ = msg_fields->msg_reference;

	*buf++ = recipientSize = deviceIdSize(msg_fields->recipient_id);
	*buf++ = 0xc9;
	buf+= convStringToNibbleSwapped(msg_fields->recipient_id,buf,recipientSize);

	*buf++ = 0x0;
	*buf++ = 0x0;
	*buf++ = 0x3b;
	*buf++ = msg_fields->data_length;

	buf += encodeAs7bitStr(msg_fields->data,buf,msg_fields->data_length);
	*len = (unsigned)(buf - origBuf);
	return SUCCESS;
}

EMBSYS_STATUS embsys_parse_submit_ack(char *buf, SMS_SUBMIT_ACK *msg_fields)
{
	unsigned numberLength;
	memset(msg_fields,0,sizeof(SMS_SUBMIT_ACK));
	do
	{
		if ((unsigned char)*buf++ != 0x07)
			break;

		msg_fields->msg_reference = *buf++;
		numberLength = *buf++;
		if ((unsigned char)*buf++ != 0xc9)
			break;
		 
		convNibbleSwappedToString(buf,msg_fields->recipient_id,numberLength);
		return SUCCESS;
	}while(0);
	return FAIL;
}

EMBSYS_STATUS embsys_parse_deliver(char *buf, SMS_DELIVER *msg_fields)
{
	unsigned numberLength;
	memset(msg_fields,0,sizeof(SMS_DELIVER));
	do
	{
		if ((unsigned char)*buf++ != 0x04)
			break;

		numberLength = *buf++;
		if ((unsigned char)*buf++ != 0xc9)
			break;

		buf += convNibbleSwappedToString(buf,msg_fields->sender_id,numberLength);

		if (*buf++ != 0x0)
			break;
		if (*buf++ != 0x0)
			break;

		buf += convNibbleSwappedToString(buf,msg_fields->timestamp,TIMESTAMP_MAX_LENGTH);
		msg_fields->data_length = *buf++;
		decodeTo8bitStr(buf, msg_fields->data, msg_fields->data_length);
		return SUCCESS;
	}while(0);
	return FAIL;
}

/* calculate the length of the device id string */
unsigned int deviceIdSize(char *pBuff)
{
	unsigned size = 0;
	while(size < ID_MAX_LENGTH && pBuff[size] != (char)NULL)
		++size;
	return size;
}

/* converts string number to nibble swapped bytes representation  with trailing Fs if needed */
unsigned int convStringToNibbleSwapped(char* str,char *swappedNibble,unsigned strSize)
{
	unsigned i;
	char* origSwappedNibble = swappedNibble;
	char n1,n2;
	for(i = 0 ; i < strSize ; i+=2)
	{
		/* if we have odd digits, add F to the last number */
		if (i+1 == strSize)
			*swappedNibble++ = (*str++ - '0') | 0xF0;
		else
		{
			/* get the value of the next two digits */
			n1 =  (*str++ - '0');
			n2 = (*str++ - '0');
			/* place the values such that the second number will be placed at the 4 MSB */
			*swappedNibble++ = (n2<<4) | n1;
		}
	}
	return (unsigned int)(swappedNibble - origSwappedNibble);
}

/* convert nibble swapped bytes representation to string with the number */
unsigned int convNibbleSwappedToString(char* nibble,char* str,unsigned strSize)
{
	unsigned int i;
	char* origNibble = nibble;

	for(i = 0; i < strSize; i += 2)
	{
		/* parse every byte into two digits unless the last byte has F at the 4 MSB */
		*str++ = (*nibble & 0xf) + '0';
		if (i+1 < strSize)
			*str++ = ((*nibble >> 4) & 0xf) + '0';
		nibble++;
	}
	return (unsigned int)(nibble - origNibble);
}

/* encode 8bits text stream to 7 bits string */
unsigned int encodeAs7bitStr(char* str,char* encodedStr,unsigned size)
{
	/* borrow indicates how many bits from the current byte we save for the next char to encode */
	unsigned int borrow = 1, i;
	char* pOrigText = encodedStr;

	for(i = 0; i < size; ++i)
	{
		*encodedStr++ = bottomNBits((str[i] >> (borrow-1)),8-borrow) | ((i+1)==size?0:(bottomNBits(str[i+1],borrow)<<(8-borrow)));
		borrow++;
		if (borrow == 8)
		{
			borrow = 1;
			++i;
		}
	}
	return (unsigned int)(encodedStr-pOrigText);
}

/* decode 7 bits string stream into regular 8 bits string stream */
unsigned int decodeTo8bitStr(char* decodedStr,char* str,unsigned size)
{
	unsigned int borrow = 0, i = 0, chars = 0;
   
	for(i = 0,chars = 0 ; chars < size ; ++i,++chars)
	{
		*str++ = (bottomNBits(decodedStr[i],7-borrow)<<borrow) | ((i==0)?0:bottomNBits((decodedStr[i-1]>>(8-borrow)),borrow));
		borrow++;
		if (borrow == 8)
		{
			borrow = 0;
			--i;
		}
	}
	return i;
}

/* return the n bottom bits from byte */
char bottomNBits(char byte,unsigned n)
{
	return ((1<<n)-1) & byte;
}

