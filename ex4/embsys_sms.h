#ifndef EMBSYS_SMS_H_
#define EMBSYS_SMS_H_

#include "embsys_utilities.h"
#include "tx_api.h"
#include "embsys_sms_protocol.h"

#define MAX_NUM_SMS 100
#define DEVICE_ID "07665692"
#define DEVICE_ID_LENGTH 8

typedef enum
{
	INCOMING_MESSAGE = 0,
	OUTGOING_MESSAGE = 1,    
} message_type;

typedef struct smsNode_t smsNode_t;
typedef struct smsNode_t* pSmsNode_t;
struct smsNode_t
{
	smsNode_t* pNext;
	smsNode_t*  pPrev;
	message_type type;	
	char title[ID_MAX_LENGTH];
	uint32_t fileName;
};

/* initialize the entire SMS module */
TX_STATUS sms_init();

#endif /* EMBSYS_SMS_H_ */

