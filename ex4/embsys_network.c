#include "embsys_network.h"
#define NETWORK_BASE_ADDR 0x200000

/* pragma pack prevents structure padding */
#pragma pack(1)
typedef struct
{
	desc_t* NTXBP;
	unsigned int NTXBL;
	unsigned int NTXFH;
	unsigned int NTXFT;

	desc_t* NRXBP;
	unsigned int NRXBL;
	unsigned int NRXFH;
	unsigned int NRXFT;

	struct
	{
		unsigned int NBSY							:1;
		unsigned int enableTxInterrupt				:1;
		unsigned int enableRxInterrupt				:1;
		unsigned int enablePacketDroppedInterrupt	:1;
		unsigned int enableTransmitErrorInterrupt	:1;
		unsigned int reserved						:24;
		unsigned int NOM							:3;
	}NCTRL;

	struct
	{
		unsigned int NTIP			:1;
		unsigned int reserved1		:1;
		unsigned int NRIP			:1;
		unsigned int NROR			:1;
		unsigned int reserved2		:4;
		union
		{
			unsigned int data		:8;
			struct
			{
				unsigned int RxComplete              :1;
				unsigned int TxComplete              :1;
				unsigned int RxBufferTooSmall        :1;
				unsigned int CircularBufferFull      :1;
				unsigned int TxBadDescriptor         :1;
				unsigned int TxNetworkError          :1;
				unsigned int reserved                :2;
			}bits;
		}NIRE;
		unsigned int reserved3       :16;
	}NSTAT;
}volatile nwRegisters_t;
#pragma pack()

nwRegisters_t* nwRegisters = (nwRegisters_t*)(NETWORK_BASE_ADDR);

network_call_back_t nwCallBacks = {0};

unsigned int network_init(const network_init_params_t *init_params)
{
	/* check for null pointers and buffer size */
	if(!init_params || !init_params->transmit_buffer || !init_params->receive_buffer || 
		!init_params->callback_list.dropped_cb || !init_params->callback_list.recieved_cb || 
		!init_params->callback_list.transmit_error_cb || !init_params->callback_list.transmitted_cb || 
		init_params->tx_buffer_size<2 || init_params->rx_buffer_size<2)
		return 1;

	/* send parameters */
	nwCallBacks = init_params->callback_list;
	nwRegisters->NTXBP = init_params->transmit_buffer;
	nwRegisters->NTXBL = init_params->tx_buffer_size;
	nwRegisters->NTXFH = 0;
	nwRegisters->NTXFT = 0;

	/* recieve parameters */
	nwRegisters->NRXBP = init_params->receive_buffer;
	nwRegisters->NRXBL = init_params->rx_buffer_size;
	nwRegisters->NRXFH = 0;
	nwRegisters->NRXFT = 0;

	/* enable interrupts */
	nwRegisters->NCTRL.enableTxInterrupt = 1;
	nwRegisters->NCTRL.enableRxInterrupt = 1;
	nwRegisters->NCTRL.enablePacketDroppedInterrupt = 1;
	nwRegisters->NCTRL.enableTransmitErrorInterrupt = 1;
	nwRegisters->NCTRL.NBSY = 0;

	return 0;
}

unsigned int network_set_operating_mode(network_operating_mode_t new_mode)
{
	/* sanity checks on operating mode */
	if(new_mode != NETWORK_OPERATING_MODE_RESERVE && new_mode != NETWORK_OPERATING_MODE_NORMAL  &&
    	new_mode != NETWORK_OPERATING_MODE_SMSC && new_mode != NETWORK_OPERATING_MODE_INTERNAL_LOOPBACK)
		return 1;

	nwRegisters->NCTRL.NOM = new_mode;
	return 0;
}

unsigned int network_send_packet(const unsigned char buffer[], unsigned int size, unsigned int length)
{
	/* sanity checks on input arguments */
	if(!buffer || length > NETWORK_MTU || size < length)
		return 1;
	if(length == 0)
		return 0;

	/* if the head is 1 cell behind the tail - the buffer is full */
	if((nwRegisters->NTXFH +1)%nwRegisters->NTXBL == nwRegisters->NTXFT)
		return 1;

	/* set next packet adn advance head */
	desc_t* pPacket = nwRegisters->NTXBP+nwRegisters->NTXFH;
	pPacket->pBuffer = (unsigned int)buffer;
	pPacket->buff_size = size;
	pPacket->reserved = length;
	nwRegisters->NTXFH = (nwRegisters->NTXFH+1)%(nwRegisters->NTXBL);

	return 0;
}

void networkISR()
{
	/* the order of those events important since in Loopback mode
	 * if we handle Rx before Tx, Rx will be asserted again, need to ack Tx
	 * before acking Rx */
	if(nwRegisters->NSTAT.NIRE.bits.TxComplete)
	{
		//locate the packet, advance tail, ack and callback
		desc_t* pPacket = nwRegisters->NTXBP+nwRegisters->NTXFT;
		nwRegisters->NTXFT = (nwRegisters->NTXFT+1)%nwRegisters->NTXBL;
		nwRegisters->NSTAT.NIRE.bits.TxComplete = 1;
		nwCallBacks.transmitted_cb((unsigned char*)pPacket->pBuffer,pPacket->buff_size);
	}
	else if(nwRegisters->NSTAT.NIRE.bits.RxComplete)
	{
		//locate the packet, advance head, ack and callback
		desc_t* pPacket = nwRegisters->NRXBP+nwRegisters->NRXFT;
		nwRegisters->NRXFT = (nwRegisters->NRXFT + 1)%nwRegisters->NRXBL;
		nwRegisters->NSTAT.NIRE.bits.RxComplete = 1;
    	nwCallBacks.recieved_cb((unsigned char*)pPacket->pBuffer,pPacket->buff_size,pPacket->reserved & 0xFF);
	}
	else if(nwRegisters->NSTAT.NIRE.bits.RxBufferTooSmall)
	{
		//ack and callback
		nwRegisters->NSTAT.NIRE.bits.RxBufferTooSmall = 1;
		nwCallBacks.dropped_cb(RX_BUFFER_TOO_SMALL);
	}
	else if(nwRegisters->NSTAT.NIRE.bits.CircularBufferFull)
	{
		//ack and callback
		nwRegisters->NSTAT.NIRE.bits.CircularBufferFull = 1;
		nwCallBacks.dropped_cb(CIRCULAR_BUFFER_FULL);
	}
	else if(nwRegisters->NSTAT.NIRE.bits.TxBadDescriptor)
	{
		//locate the packet, advance tail, ack, and callback
		desc_t* pPacket = nwRegisters->NTXBP+nwRegisters->NTXFT;
		nwRegisters->NTXFT = (nwRegisters->NTXFT+1)%nwRegisters->NTXBL;
		nwRegisters->NSTAT.NIRE.bits.TxBadDescriptor = 1;
		nwCallBacks.transmit_error_cb(BAD_DESCRIPTOR,(unsigned char*)pPacket->pBuffer,pPacket->buff_size,pPacket->reserved & 0xFFFF);
	}
	else if(nwRegisters->NSTAT.NIRE.bits.TxNetworkError)
	{
		//locate the packet, advance tail, ack and callback
		desc_t* pPacket = nwRegisters->NTXBP+nwRegisters->NTXFT;
		nwRegisters->NTXFT = (nwRegisters->NTXFT+1)%nwRegisters->NTXBL;
		nwRegisters->NSTAT.NIRE.bits.TxNetworkError = 1;
		nwCallBacks.transmit_error_cb(NETWORK_ERROR,(unsigned char*)pPacket->pBuffer,pPacket->buff_size,pPacket->reserved & 0xFFFF);
	}
}

