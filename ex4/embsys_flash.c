#include "embsys_flash.h"
#include "embsys_utilities.h"
#include "tx_api.h"

/* register locations */
#define FLASH_SR 0x150
#define FLASH_CR 0x151
#define FLASH_FADDR 0x152
/* the data is stored in 0x153(0x00)-0x162(0x0F) */
#define FLASH_FDATA 0x153

/* flags in the registers */
#define FLASH_SR_SPI_CYCLE_DONE 0x2
#define INTERRUPT_ENABLE 0x2
#define SPI_CYCLE_GO 0x1
#define SPI_CYCLE_IN_PROGRESS 0x1

/* commands */
#define CMD_READ 0x03
#define CMD_WRITE 0x05
#define CMD_ERASE_BLOCK 0xd8
#define CMD_ERASE_ALL 0xc7

/* constants for loading/storing data */
#define LOAD_DATA 0
#define STORE_DATA 1

typedef enum
{
	IDLE			= 0x00,
	READ_DATA		= 0x03,
	PAGE_PROGRAM 	= 0x05,
	BLOCK_ERASE		= 0xD8,
	BULK_ERASE		= 0xC7,
}SPI_command;
SPI_command flashCommand;

/* Interrupts */
void (*pFlashDoneCallback)(void) = NULL;
void (*pFlashReadCallback)(unsigned char* buffer, unsigned int count) = NULL;

TX_EVENT_FLAGS_GROUP flashEventFlags;

/* Store the callback functions */
void embsys_flash_init(void (*doneCallback)(), void (*readCallback)(unsigned char* buffer, unsigned int count))
{
	if (!doneCallback || !readCallback)
		return;
	pFlashDoneCallback = doneCallback;
	pFlashReadCallback = readCallback;
	flashCommand = IDLE;
	tx_event_flags_create(&flashEventFlags,"flash global event flags");
}

/* Called upon interrupt, immediately after the end of each cycle */
void flashISR()
{
	//acknowledge the interrupt
	_sr(FLASH_SR_SPI_CYCLE_DONE, FLASH_SR);

	if (flashCommand == READ_DATA || flashCommand == PAGE_PROGRAM || flashCommand == IDLE)
		tx_event_flags_set(&flashEventFlags,1,TX_OR);
	else /* BLOCK_ERASE or BULK_ERASE */
	{
		//erase done
		flashCommand = IDLE;
		pFlashDoneCallback();
	}
}

void flash_transfer_data(unsigned char direction, int count, unsigned char* data) 
{
	unsigned int data_pos = 0, i = 0;
	if (direction == STORE_DATA) 
		for(i = 0, data_pos = 0; i < count; i += 4, data_pos++)
			_sr((*(unsigned int*)(data + i)), FLASH_FDATA + data_pos);
	else /* LOAD_DATA */
	{
		unsigned int j, regData;
		unsigned char* pRegData = (unsigned char*)&regData;
		while(i < count)
		{
			regData = _lr(FLASH_FDATA + data_pos);
			for(j = 0; j < 4 && i < count; i++, j++)
				data[i] = pRegData[j];
			data_pos++;
		}
	}
}

void embsys_flash_read(unsigned int address, unsigned int size, unsigned char* buffer) 
{
	if(!buffer || size == 0 || size > MAX_BUFFER_SIZE || embsys_flash_busy())
		return;
		
    unsigned long int actualFlags;
    unsigned int transactionSize, currentIndex=0;
	flashCommand = READ_DATA;
	while(currentIndex < size)
	{
		//locate the maximal number of bytes that can be loaded from the flash in one request
		transactionSize = _min(size-currentIndex, FDATA_MAX_SIZE);

		//set address and launch operation
		_sr(address + currentIndex, FLASH_FADDR);
		_sr(SPI_CYCLE_GO | INTERRUPT_ENABLE | (flashCommand << 8) | ((transactionSize -1) << 16), FLASH_CR);

		//wait for the flash hardware to be idle
		tx_event_flags_get(&flashEventFlags, 1, TX_AND_CLEAR, &actualFlags, TX_WAIT_FOREVER);
		//load the date from the flash regs
		flash_transfer_data(LOAD_DATA, transactionSize, buffer+currentIndex);
		//update the number of bytes that was read
		currentIndex += transactionSize;
	}
	flashCommand = IDLE;
}

void embsys_flash_write(unsigned int address, unsigned int size, unsigned char *buffer)
{
	if(!buffer || size == 0 || size > MAX_BUFFER_SIZE || embsys_flash_busy())
		return;

    unsigned long int actualFlags;
    unsigned int transactionSize, currentIndex=0;
	flashCommand = PAGE_PROGRAM;
	while(currentIndex < size)
	{
		//locate the maximal number of bytes that can be loaded from the flash in one request
		transactionSize = _min(size-currentIndex, FDATA_MAX_SIZE);
		//read data from the given buffer to data registers
		flash_transfer_data(STORE_DATA, transactionSize, buffer+currentIndex);
		
		//set address and launch operation
		_sr(address + currentIndex, FLASH_FADDR);
		_sr(SPI_CYCLE_GO | INTERRUPT_ENABLE | (flashCommand << 8) | ((transactionSize -1) << 16), FLASH_CR);
		
		//update the number of bytes that was written
		currentIndex += transactionSize;
		//wait for the flash hardware to be idle
		tx_event_flags_get(&flashEventFlags, 1, TX_AND_CLEAR, &actualFlags, TX_WAIT_FOREVER);
	}
	flashCommand = IDLE;
}

/* Delete a block of 4k at a given address */
void embsys_flash_delete(unsigned int address) 
{
	/* write starting address to FADDR and launch operation */
	_sr(address & 0xFFFFF000, FLASH_FADDR);
	flashCommand = BLOCK_ERASE;
	_sr(SPI_CYCLE_GO | INTERRUPT_ENABLE | (flashCommand << 8), FLASH_CR);
}

/* Clears the flash */
void embsys_flash_delete_all() 
{
	/* launch bulk erase */
	flashCommand = BULK_ERASE;
	_sr(SPI_CYCLE_GO | INTERRUPT_ENABLE | (flashCommand << 8), FLASH_CR);
}

/* Checks if flash is busy */
int embsys_flash_busy()
{
	return (flashCommand != IDLE || (_lr(FLASH_SR) & SPI_CYCLE_IN_PROGRESS == 1));
}

