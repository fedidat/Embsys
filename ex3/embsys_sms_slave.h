#ifndef EMBSYS_SMS_SLAVE_H_
#define EMBSYS_SMS_SLAVE_H_

#include "tx_api.h"
#include "embsys_uart.h"
#include "embsys_utilities.h"
#include "embsys_sms.h"

/* initialization */
unsigned int embsys_sms_slave_init();

/* when a period comes, the request is processed here */
void handleRequest();

/* returns the count of messages */
void handleMessageCount();

/* reads the data of a message into the send buffer */
void handleReadMessage();

/* deletes a message from the device's database */
void handleDeleteMessage();

/* sends an SMS */
void handleSendMessage();

/* updates a message's data field */
void handleUpdateMessage();

/* sends an invalid request reply "IR." */
void sendInvalidRequestReply();

#endif /* EMBSYS_SMS_SLAVE_H_ */

