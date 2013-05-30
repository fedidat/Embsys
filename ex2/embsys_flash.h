#ifndef EMBSYS_FLASH_H_
#define EMBSYS_FLASH_H_

typedef unsigned int Address; 

/* Interrupts */
void embsys_flash_init(void (*callback)());
_Interrupt1 void flash_isr();

/* Reads from a given address to a buffer */
void embsys_flash_read(Address addr, int count, unsigned char *buff);

/* Copy a buffer to the flash */
void embsys_flash_write(Address addr, int count, unsigned char *buff);

/* Delete a block of 4k at a given address */
void embsys_flash_delete(Address addr);

/* Clears the flash */
void embsys_flash_delete_all();

#endif /*EMBSYS_FLASH_H_*/
