#ifndef __SMS_PROTOCOL_H__
#define __SMS_PROTOCOL_H__

#define ID_MAX_LENGTH 8
#define TIMESTAMP_MAX_LENGTH 14
#define DATA_MAX_LENGTH 160

/*

  The supplied functions use this structs to pass / receive the messages' fields as 7-bit alphabet plain-text.

  'device_id', 'sender_id', 'recipient_id' are null-terminated if shorter then 8 digits.

  Minimal input error checking is made.

*/
typedef struct _SMS_PROBE {

  char device_id[ID_MAX_LENGTH];
  char timestamp[TIMESTAMP_MAX_LENGTH];
  char sender_id[ID_MAX_LENGTH];  

}SMS_PROBE;

typedef struct _SMS_DELIVER {

  char sender_id[ID_MAX_LENGTH];
  char timestamp[TIMESTAMP_MAX_LENGTH];
  unsigned data_length;  
  char data[DATA_MAX_LENGTH];

}SMS_DELIVER;

typedef struct _SMS_SUBMIT {

  char device_id[ID_MAX_LENGTH];
  char msg_reference;
  char recipient_id[ID_MAX_LENGTH];  
  unsigned data_length;  
  char data[DATA_MAX_LENGTH];

}SMS_SUBMIT;

typedef struct _SMS_SUBMIT_ACK {

  char msg_reference;
  char recipient_id[ID_MAX_LENGTH];  

}SMS_SUBMIT_ACK;

typedef enum _EMBSYS_STATUS { SUCCESS=0, FAIL } EMBSYS_STATUS;

/*

  Description:
    Fill the buffer with an SMS_PROBE / SMS_PROBE_ACK message fields

  Arguments:
    buf - a pointer to a buffer to fill / parse
    msg_fields - a pointer to a struct to get / put the fields of the message
    is_ack - a value other then NULL indicates this is an SMS_PROBE_ACK 
             and the appropriate fields in the struct are applicable
    len - the actual used size of the supllied buffer

  Return value:
  

*/
EMBSYS_STATUS embsys_fill_probe(char *buf, SMS_PROBE *msg_fields, char is_ack, unsigned *len);

/*

  Description:
    Fill the buffer with an SMS_SUBMIT message fields

  Arguments:
    buf - a pointer to a buffer to fill / parse
    msg_fields - a pointer to a struct to get / put the fields of the message
    len - the actual used size of the supllied buffer

  Return value:
  

*/
EMBSYS_STATUS embsys_fill_submit(char *buf, SMS_SUBMIT *msg_fields, unsigned *len);

/*

  Description:
    Parse the buffer as an SMS_SUBMIT_ACK message

  Arguments:
    buf - a pointer to a buffer to fill / parse
    msg_fields - a pointer to a struct to get / put the fields of the message

  Return value:
  

*/
EMBSYS_STATUS embsys_parse_submit_ack(char *buf, SMS_SUBMIT_ACK *msg_fields);

/*

  Description:
    Parse the buffer as an SMS_DELIVER message

  Arguments:
    buf - a pointer to a buffer to fill / parse
    msg_fields - a pointer to a struct to get / put the fields of the message

  Return value:
  

*/
EMBSYS_STATUS embsys_parse_deliver(char *buf, SMS_DELIVER *msg_fields);

#endif
