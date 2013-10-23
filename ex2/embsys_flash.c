#include "embsys_flash.h"

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

/* function declarations */
void run_spi(int cmd, int count);
void flash_transfer_data(unsigned char direction, int count, unsigned char* data);
void embsys_flash_continue_read();
void embsys_flash_continue_write();

/* Global variables for interrupt/function interaction */
int bufferTotal;
int bufferCurrent;
int isRead;
int cur_count;
Address address;
unsigned char buffer[512];

/* Interrupts */
void (*pFlashCallback)(void);
void (*pFlashReadCallback)(unsigned char* buffer, int count);

/* Store the callback functions */
void embsys_flash_init(void (*callback)(), void (*readCallback)(unsigned char* buffer, int count))
{
	pFlashCallback = callback;
	pFlashReadCallback = readCallback;
}

/* Called upon interrupt, immediately after the end of each cycle */
_Interrupt1 void flash_isr()
{
	_sr(FLASH_SR_SPI_CYCLE_DONE, FLASH_SR);
	if(isRead == 1)
	{
		flash_transfer_data(LOAD_DATA, cur_count, buffer + bufferCurrent);	
		bufferCurrent += 64;
		if(bufferTotal - bufferCurrent > 0)
			embsys_flash_continue_read();
		else
		{
			(*pFlashReadCallback)(buffer, bufferTotal);
			(*pFlashCallback)();
		}
	}
	else
	{
		bufferCurrent += 64;
		if(bufferCurrent < bufferTotal)
			embsys_flash_continue_write();
		else
			(*pFlashCallback)();
	}
}

void run_spi(int cmd, int count) {
	/* spi cycle go = 1, interrupt enable = 1, set command, set byte count */
	unsigned int cr = SPI_CYCLE_GO | INTERRUPT_ENABLE | (cmd << 8) | ((count -1) << 16);
	/* execute the cycle */
	_sr(cr, FLASH_CR);
}

void flash_transfer_data(unsigned char direction, int count, unsigned char* data) 
{
	unsigned int data_pos = 0, i;
	if (direction == STORE_DATA) 
	{	
		for(i = 0 , data_pos = 0 ; i < count ; i+=4 , ++data_pos)
			_sr((*(unsigned int*)(data+i)),FLASH_FDATA+data_pos);
	} 
	else /* LOAD_DATA */
	{
		unsigned int j, regData;
		unsigned char* pRegData = (unsigned char*)&regData;
		while(i < count)
		{
			regData = _lr(FLASH_FDATA + data_pos);
			for(j = 0; j < 4 && i < count; ++i, ++j)
				data[i] = pRegData[j];
			data_pos++;
		}
	}
}

void embsys_flash_read(Address addr, int count) 
{
	/* note reading operation */
	isRead = 1;
	/* store parameters */
	bufferTotal = count;
	address = addr;
	bufferCurrent = 0;
	/* make first call */
	embsys_flash_continue_read();
}

/* Reads from a given address to a buffer */
void embsys_flash_continue_read() 
{
	if (bufferTotal <= 0)
		return;
	/* write flash address */
	_sr(address + bufferCurrent, FLASH_FADDR);
	/* calculate how much to read out of 64 bytes max */
	cur_count = _min(64, bufferTotal - bufferCurrent);
	/* start the SPI command */
	run_spi(CMD_READ, cur_count);
}

void embsys_flash_write(Address addr, int count, unsigned char *buff)
{
	/* note writing operation */
	isRead = 0;
	/* store parameters */
	int i;
	for(i=0; i<count; i++)
		buffer[i] = buff[i];
	bufferTotal = count;
	address = addr;
	bufferCurrent = 0;
	/* make first call */
	embsys_flash_continue_write();
}

/* Copy a buffer to the flash */
void embsys_flash_continue_write() 
{
	if (bufferTotal <= 0) 
		return;
	/* write flash address */
	_sr(address + bufferCurrent, FLASH_FADDR);
	/* calculate current count out of 64 bytes max per write */
	cur_count = _min(64, bufferTotal - bufferCurrent);
	/* copy the bytes to the flash data registers */
	flash_transfer_data(STORE_DATA, cur_count, buffer + bufferCurrent);
	/* start the SPI command */
	run_spi(CMD_WRITE, cur_count);
}

/* Delete a block of 4k at a given address */
void embsys_flash_delete(Address addr) 
{
	/* note non-reading operation for callback */
	isRead = 0;
	_sr(addr & 0xFFFFF000, FLASH_FADDR);
	run_spi(CMD_ERASE_BLOCK, 0);
}

/* Clears the flash */
void embsys_flash_delete_all() 
{
	/* note non-reading operation for callback */
	isRead = 0;
	run_spi(CMD_ERASE_ALL, 0);
}

/* Checks if flash is busy */
int embsys_flash_busy()
{
	if(_lr(FLASH_SR) & SPI_CYCLE_IN_PROGRESS == 1)
		return 1;
	else
		return 0;
}

