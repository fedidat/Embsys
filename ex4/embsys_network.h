#ifndef EMBSYS_NETWORK_H_
#define EMBSYS_NETWORK_H_

#include "embsys_utilities.h"
#define NETWORK_MTU 161

#pragma pack(1)
typedef struct
{
	unsigned int pBuffer;
	unsigned char buff_size;
	uint16_t reserved;
}desc_t;
#pragma pack()

typedef enum
{
    NETWORK_OPERATING_MODE_RESERVE 				= 0 ,
    NETWORK_OPERATING_MODE_NORMAL 				= 0x1 << 0,
    NETWORK_OPERATING_MODE_SMSC 				= 0x1 << 1,
    NETWORK_OPERATING_MODE_INTERNAL_LOOPBACK 	= 0x1 << 2,
}network_operating_mode_t;

typedef enum
{
    RX_BUFFER_TOO_SMALL ,
    CIRCULAR_BUFFER_FULL
}packet_dropped_reason_t;

typedef enum
{
    BAD_DESCRIPTOR ,
    NETWORK_ERROR
}transmit_error_reason_t;

typedef void (*network_packet_transmitted_cb)(const unsigned char *buffer, unsigned int size);
typedef void (*network_packet_received_cb)(unsigned char buffer[], unsigned int size, unsigned int length);
typedef void (*network_packet_dropped_cb)(packet_dropped_reason_t);
typedef void (*network_transmit_error_cb)(transmit_error_reason_t, unsigned char *buffer, unsigned int size, unsigned int length );

typedef struct
{
	network_packet_transmitted_cb transmitted_cb;
	network_packet_received_cb recieved_cb;
	network_packet_dropped_cb dropped_cb;
	network_transmit_error_cb transmit_error_cb;
} network_call_back_t;

typedef struct
{
	desc_t *transmit_buffer;
	unsigned int tx_buffer_size;
	desc_t *receive_buffer; 
	unsigned int rx_buffer_size; 
	network_call_back_t callback_list;
} network_init_params_t;

/* initialization */
unsigned int network_init(const network_init_params_t *init_params);

/* set operating mode between normal and SMS optimized */
unsigned int network_set_operating_mode(network_operating_mode_t new_mode);

/* receives and starts sending a packet */
unsigned int network_send_packet(const unsigned char buffer[], unsigned int size, unsigned int length);

#endif /* EMBSYS_NETWORK_H_ */
