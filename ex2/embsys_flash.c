#include "embsys_flash.h"

// register locations
#define FLASH_SR    (0x150)
#define FLASH_CR    (0x151)
#define FLASH_FADDR (0x152)

// the data is stored in 0x153(0x00)-0x162(0x0F)
#define FLASH_FDATA (0x153)

// flags in the SR register
#define FLASH_SR_SPI_CYCLE_DONE (0x2)

// commands
#define CMD_READ (0x03)
#define CMD_WRITE (0x05)
#define CMD_ERASE_BLOCK (0xd8)
#define CMD_ERASE_ALL (0xc7)
#define INTERRUPT_ENABLE (0x1)

// constants for loading/storing data to the flash data array
#define LOAD_DATA (0)
#define STORE_DATA (1)


// function declarations
void wait_for_SPI();
void run_spi(int cmd, int count);


/* Interrupts */
void (*pFlashCallback)(void);
void embsys_flash_init(void (*callback)())
{
	int write = _lr(FLASH_CR) | INTERRUPT_ENABLE;
	_sr(write, FLASH_CR);
	pFlashCallback = callback;
}
_Interrupt1 void flash_isr()
{
	_sr(INTERRUPT_ENABLE, FLASH_CR);
	(*pFlashCallback)();
}

void wait_for_SPI() {
	// wait for the cycle done bit
	while ((_lr(FLASH_SR) & FLASH_SR_SPI_CYCLE_DONE) == 0) {
		// busy waiting
	}
	// clear it
	_sr(FLASH_SR_SPI_CYCLE_DONE, FLASH_SR);
}

void run_spi(int cmd, int count) {
	unsigned int cr;
	cr = 0x1; // spi cycle go = 1, interrupt enable = 0
	cr |= cmd << 8; // set command
	cr |= (count -1) << 16; // set byte count
	// execute the cycle
	_sr(cr, FLASH_CR);
	wait_for_SPI();
}

void transfer_data_from_flash(unsigned char direction, int count, unsigned char* data) {
	int addr = FLASH_FDATA;
	int data_pos = 0, tmp_pos;
	char tmp[4];
	if (direction == STORE_DATA) {
		// copy 4 bytes at a time
		for(; count > 0; count -=4, addr++) {
			tmp_pos = 0;
			switch(count) {
			default:
			case 4: tmp[tmp_pos++] = data[data_pos++];
			case 3: tmp[tmp_pos++] = data[data_pos++];
			case 2: tmp[tmp_pos++] = data[data_pos++];
			case 1: tmp[tmp_pos++] = data[data_pos++];
			}
			_sr(*((unsigned int*)tmp), addr);
		}
	} else {
		// load data
		for(; count > 0; count -=4, addr++) {
			tmp_pos = 0;
			*((unsigned int*)tmp) =  _lr(addr);
			switch(count) {
			default:
			case 4: data[data_pos++] = tmp[tmp_pos++];
			case 3: data[data_pos++] = tmp[tmp_pos++];
			case 2: data[data_pos++] = tmp[tmp_pos++];
			case 1: data[data_pos++] = tmp[tmp_pos++];
			}
		}
	}

}


/* Reads from a given address to a buffer */
void embsys_flash_read(Address addr, int count, unsigned char *buff) {
	if (count <= 0) {
		return;
	}
	for (int pos = 0; pos < count; pos += 64) {
		_sr(addr + pos, FLASH_FADDR);

		// calculate how much to read in this iteration
		int cur_count = _min(64, count - pos);

		// start the SPI command
		run_spi(CMD_READ, cur_count);

		// copy the read bytes to the target array
		transfer_data_from_flash(LOAD_DATA, cur_count, buff + pos);
	}
}

/* Copy a buffer to the flash */
void embsys_flash_write(Address addr, int count, unsigned char *buff) {
	if (count <= 0) {
			return;
		}
		for (int pos = 0; pos < count; pos += 64) {
			_sr(addr + pos, FLASH_FADDR);

			// calculate how much to write in this iteration
			int cur_count = _min(64, count - pos);

			// copy the bytes to the flash data registers
			transfer_data_from_flash(STORE_DATA, cur_count, buff + pos);

			// start the SPI command
			run_spi(CMD_WRITE, cur_count);
		}
}

/* Delete a block of 4k at a given address */
void embsys_flash_delete(Address addr) {
	_sr(addr & 0xFFFFF000, FLASH_FADDR);
	run_spi(CMD_ERASE_BLOCK, 0);
}

/* Clears the flash */
void embsys_flash_delete_all() {
	run_spi(CMD_ERASE_ALL, 0);
}

