#ifndef __EMBSYS_PERSISTENCE_H_
#define __EMBSYS_PERSISTENCE_H_

typedef enum {
	FS_SUCCESS =						0,
	FS_NOT_READY =						1,
	FS_IS_BUSY =						2,
	FAILURE_ACCESSING_FLASH =			3,
	COMMAND_PARAMETERS_ERROR =			4,
	FILE_NOT_FOUND = 					5,
	MAXIMUM_FILE_SIZE_EXCEEDED =		6,
	MAXIMUM_FLASH_SIZE_EXCEEDED =		7,
	MAXIMUM_FILES_CAPACITY_EXCEEDED =	8,
	FS_ERROR =							9,
} FS_STATUS;

/* initialize the persistence module */
FS_STATUS embsys_persistence_init();

/* write a file to flash */
FS_STATUS embsys_persistence_write(const char* filename, unsigned length, const char* data);

/* read a file from the flash */
FS_STATUS embsys_persistence_read(const char* filename, unsigned* length, char* data);

/* remove a file from flash */
FS_STATUS embsys_persistence_erase(const char* filename);

/* fetch the name of a file given its ID */
FS_STATUS embsys_persistence_get_filename_by_index(unsigned index, unsigned* length, char* name);

/* return the count of files in flash */
FS_STATUS embsys_persistence_count(unsigned* file_count);

#endif /* EMBSYS_PERSISTENCE_H_ */

