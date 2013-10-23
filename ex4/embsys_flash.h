#ifndef EMBSYS_FLASH_H_
#define EMBSYS_FLASH_H_

#define MAX_BUFFER_SIZE 512
#define FLASH_BLOCKS (16)
#define FLASH_CAPACITY (FLASH_BLOCKS*4*1024)
#define FLASH_BLOCK_SIZE (4*1024)
#define FDATA_MAX_SIZE 64

/* Interrupts */
void embsys_flash_init(void (*doneCallback)(), void (*readCallback)(unsigned char* buffer, unsigned int count));

/* Reads from a given address to a buffer */
void embsys_flash_read(unsigned int address, unsigned int size, unsigned char* buffer);

/* Copy a buffer to the flash */
void embsys_flash_write(unsigned int address, unsigned int size, unsigned char *buffer);

/* Delete a block of 4k at a given address */
void embsys_flash_delete(unsigned int address);

/* Clears the flash */
void embsys_flash_delete_all();

/* Checks if flash is busy */
int embsys_flash_busy();

#endif /*EMBSYS_FLASH_H_*/
