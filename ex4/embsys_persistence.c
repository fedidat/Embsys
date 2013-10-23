#include "embsys_persistence.h"
#include "embsys_flash.h"
#include "embsys_utilities.h"
#include "tx_api.h"

/* Persistence defines */
#define FILE_NAME_MAX_LEN (8)
#define ERASE_UNITS_NUMBER (FLASH_CAPACITY/FLASH_BLOCK_SIZE)
#define MAX_FILE_SIZE 512
#define MAX_FILES_CAPACITY 1000
#define FLASH_UNINIT_4_BYTES 0xFFFFFFFF
#define MAX_UINT_32 0xFFFFFFFF
#define CANARY_VALUE 0xDEADBEEF

/* block types */
typedef enum
{
	DATA	= 0, 
	LOG		= 1,
}EraseUnitType;

/* Help functions */
#define READ_LOG_ENTRY(logIndex,entry) (embsys_flash_read((logIndex+1)*FLASH_BLOCK_SIZE - sizeof(LogEntry),sizeof(LogEntry),(unsigned char*)&entry))
#define WRITE_LOG_ENTRY(logIndex,entry) (embsys_flash_write((logIndex+1)*FLASH_BLOCK_SIZE - sizeof(LogEntry),sizeof(LogEntry),(unsigned char*)&entry))

/* File structures */
#pragma pack(1)
typedef union
{
	unsigned char data;
	struct
	{
		unsigned char type		   		: 1; /* 0 => log, 1 => sectors */
		unsigned char emptyEraseUnit	: 1; /* 1 => empty */
		unsigned char valid		   		: 1; /* 1 => whole eu valid */
		unsigned char reserved	   		: 5;
	} bits;
}EraseUnitMetadata;
typedef struct
{
	EraseUnitMetadata metadata;
	unsigned int eraseNumber;
	unsigned int canary;
}EraseUnitHeader;
typedef struct
{
	/* offset of the file's data from the beginning of the block */
	unsigned int offset;
	/* file data's size */
	unsigned int size;
	union
	{
		unsigned char data;
		struct
		{
			unsigned char lastDesc		: 1;
			unsigned char validDesc	    : 1;
			unsigned char obsoleteDes	: 1;
			unsigned char validOffset	: 1;
			unsigned char reserved	    : 4;
		} bits;
	}metadata;
	char fileName[FILE_NAME_MAX_LEN];
}SectorDescriptor;
typedef union
{
	unsigned int data;
	struct
	{
		unsigned int eraseUnitIndex		: 4;
		unsigned int eraseNumber		: 25;
		unsigned int valid				: 1;
		unsigned int copyComplete		: 1;
		unsigned int eraseComplete		: 1;
	} bits;
}LogEntry;
#pragma pack()

typedef struct
{
	unsigned int eraseNumber;
	unsigned int bytesFree;
	unsigned int deleteFilesTotalSize;
	unsigned int validDescriptors;
    unsigned int totalDescriptors;
	unsigned int nextFreeOffset;
    EraseUnitMetadata metadata;
}EraseUnitLogicalHeader;

/* Module variables */
EraseUnitLogicalHeader eraseUnitList[ERASE_UNITS_NUMBER];
unsigned char logIndex;
unsigned int fileCount = 0;
TX_EVENT_FLAGS_GROUP fsGlobalEventFlags;
TX_MUTEX fsMutex;
bool fs_ready = false;

/* Flash prototypes */
void flashReadDoneCB(unsigned char* buffer, unsigned int count);
void flashDoneCB(void);

/* Integrity related prototypes */
FS_STATUS installFileSystem();
FS_STATUS loadFilesystem();
bool isSegmentUninitialized(unsigned char* pSeg,unsigned int size);
FS_STATUS CompactBlock(unsigned char eraseUnitIndex,LogEntry entry);
FS_STATUS CompleteCompaction(unsigned char firstLogIndex, bool secondLogExists,
	unsigned char secondLogIndex, bool corruptedEUExists, unsigned char corruptederaseUnitIndex);
void fixLogEntry(LogEntry* pEntry);

/* Meta file system prototypes */
FS_STATUS ParseEraseUnitContent(unsigned int i);
bool findFreeEraseUnit(unsigned int fileSize,unsigned char* eraseUnitIndex);
bool findEraseUnitToCompact(unsigned char* eraseUnitIndex,unsigned size);
FS_STATUS getNextValidSectorDescriptor(unsigned char eraseUnitIndex,unsigned int* pNextActualSectorIndex,SectorDescriptor* pSecDesc);
FS_STATUS getSectorDescriptor(const char* filename,SectorDescriptor *pSecDesc,unsigned char *pEraseUnitIndex,unsigned int* pSecActualIndex);

/* File manipulation prototypes */
FS_STATUS writeFile(const char* filename, unsigned length, const char* data,unsigned char eraseUnitIndex);
FS_STATUS eraseFileFromFS(const char* filename);
FS_STATUS CopyDataInsideFlash(unsigned int srcAddr,unsigned int srcSize,unsigned int destAddr);

/* initialize the persistence module */
FS_STATUS embsys_persistence_init()
{
	TX_STATUS status;
	FS_STATUS fStatus;

	/* create event flags, mutex, initialize the flash module and load the file system */
	if ((status=tx_event_flags_create(&fsGlobalEventFlags,"fs global event flags")) != TX_SUCCESS)
		return FS_ERROR;   
	if ((status=tx_mutex_create(&fsMutex,"fs global lock",TX_NO_INHERIT)) != TX_SUCCESS)
		return FS_ERROR;
	embsys_flash_init(flashDoneCB, flashReadDoneCB);
	fStatus = loadFilesystem();
	if (fStatus == FS_SUCCESS)
		fs_ready = true;

	return fStatus;
}

/* write a file to flash */
FS_STATUS embsys_persistence_write(const char* filename, unsigned length, const char* data)
{
	LogEntry entry;
	FS_STATUS status = MAXIMUM_FLASH_SIZE_EXCEEDED;
	unsigned char eraseUnitIndex,oldLogIndex;

	if (filename == NULL || data == NULL)
	  return COMMAND_PARAMETERS_ERROR;
	if (length > MAX_FILE_SIZE)
	  return MAXIMUM_FILE_SIZE_EXCEEDED;
	if (fileCount > MAX_FILES_CAPACITY)
	  return MAXIMUM_FILES_CAPACITY_EXCEEDED;
	if (!fs_ready)
		return FS_NOT_READY;
	if (tx_mutex_get(&fsMutex,TX_NO_WAIT) != TX_SUCCESS)
		return FS_IS_BUSY;
		   
	do
	{
		/* try to find free space */
		if (findFreeEraseUnit(length,&eraseUnitIndex))
		{
			status = eraseFileFromFS(filename);
			if (status != FS_SUCCESS && status != FILE_NOT_FOUND)
			{
				status = FAILURE_ACCESSING_FLASH;
				break;
			}
			status =  writeFile(filename,length,data,eraseUnitIndex);
		}
		/* try to find EraseUnit that will have enough space after compaction */
		else if (findEraseUnitToCompact(&eraseUnitIndex,length+sizeof(SectorDescriptor)))
		{
			entry.data = FLASH_UNINIT_4_BYTES;
			oldLogIndex = logIndex;
			if (CompactBlock(eraseUnitIndex,entry) == FS_SUCCESS)
			{
				/* erase any previous file with the same name */
				status = eraseFileFromFS(filename);
				if (status != FS_SUCCESS && status != FILE_NOT_FOUND)
				{
					status =  FAILURE_ACCESSING_FLASH;
					break;
				}

				/* write the actual file */
				status =  writeFile(filename,length,data,oldLogIndex);
			}
		}
	}while(false);

	tx_mutex_put(&fsMutex);
	return status;
}

/* read a file from the flash */
FS_STATUS embsys_persistence_read(const char* filename, unsigned* length, char* data)
{
	SectorDescriptor sec;
	FS_STATUS status = FS_SUCCESS;
	unsigned char secEraseUnitIndex;
   
	if (filename == NULL || length == NULL || data == NULL)
	   return COMMAND_PARAMETERS_ERROR;
	if (!fs_ready)
		return FS_NOT_READY;	
	if (tx_mutex_get(&fsMutex,TX_NO_WAIT) != TX_SUCCESS)
		return FS_IS_BUSY;
   
	do
	{
		status = getSectorDescriptor(filename,&sec,&secEraseUnitIndex,NULL);
		if (status != FS_SUCCESS)
			break;

		/* if the buffer is too small, notify a FS_ERROR */
		if (sec.size > *length)
		{
			status =  FS_ERROR;
			break;
		}
		*length = sec.size;

		/* read the file content */
		embsys_flash_read(FLASH_BLOCK_SIZE*secEraseUnitIndex + sec.offset, sec.size, (unsigned char*)data);
	}while(false);
   
   tx_mutex_put(&fsMutex);
   return status;
}

/* remove a file from flash */
FS_STATUS embsys_persistence_erase(const char* filename)
{
    FS_STATUS status;
	if(!fs_ready)
		return FS_NOT_READY;
	
	/* lock the mutex and erase the file from flash */
	if(tx_mutex_get(&fsMutex,TX_NO_WAIT) != TX_SUCCESS)
		return FS_IS_BUSY;
    status = eraseFileFromFS(filename);
	tx_mutex_put(&fsMutex);
	
    return status;
}

/* fetch the name of a file given its ID */
FS_STATUS embsys_persistence_get_filename_by_index(unsigned index, unsigned* length, char* name)
{
	SectorDescriptor desc;
	unsigned int actualSecDesc;
	unsigned char euIdx = 0,fileNameLen = 0;
	FS_STATUS status = FS_SUCCESS;

	if (!fs_ready)
		return FS_NOT_READY;
	if (tx_mutex_get(&fsMutex, TX_NO_WAIT) != TX_SUCCESS)
		return FS_IS_BUSY;

	do
	{
		if (index >= fileCount)
		{
			status = FILE_NOT_FOUND;
			break;
		}
		while (index >= eraseUnitList[euIdx].validDescriptors)
		{
			index -= eraseUnitList[euIdx].validDescriptors;
			++euIdx;
		}
		actualSecDesc = 0;
		
		for(unsigned int i = 0; i <= index; ++i)
			if ((status = getNextValidSectorDescriptor(euIdx,&actualSecDesc,&desc)) != FS_SUCCESS)
				break;
		while(fileNameLen < FILE_NAME_MAX_LEN && desc.fileName[fileNameLen] != '\0')
			++fileNameLen;

		if (fileNameLen > *length)
		{
			status = FS_ERROR;
			break;
		}

		memcpy(name, desc.fileName, fileNameLen);
		*length = fileNameLen;
	}while(false);

	tx_mutex_put(&fsMutex);
	return status;
}

/* return the count of files in flash */
FS_STATUS embsys_persistence_count(unsigned* file_count)
{
	if(!fs_ready)
		return FS_NOT_READY;
	
	/* lock the mutex and retrieve the file count */
	if(tx_mutex_get(&fsMutex,TX_NO_WAIT) != TX_SUCCESS)
		return FS_IS_BUSY;
	*file_count = fileCount;
	tx_mutex_put(&fsMutex);

	return FS_SUCCESS;
}

/* install the file system on the flash, all previous content gets deleted */
FS_STATUS installFileSystem()
{
	unsigned int euAddress = 0;
    unsigned long int actualFlags;
	
	EraseUnitHeader header;
    memset(&header,0xFF,sizeof(EraseUnitHeader));
	header.canary = CANARY_VALUE;
	header.eraseNumber = 0;
	header.metadata.bits.emptyEraseUnit = 1;

	/* first, erase the flash */
	embsys_flash_delete_all();
	tx_event_flags_get(&fsGlobalEventFlags,1,TX_AND_CLEAR,&actualFlags,TX_WAIT_FOREVER);
	
	/* then write each EraseUnitHeader and update the data structure of the EU's logical metadata */
	for(unsigned int i = 0 ; i < ERASE_UNITS_NUMBER ; ++i)
	{
		header.metadata.bits.valid = 1;
		if (i > 0)
		{
			header.metadata.bits.type = DATA;
            eraseUnitList[i].nextFreeOffset = FLASH_BLOCK_SIZE;
            eraseUnitList[i].bytesFree = FLASH_BLOCK_SIZE-sizeof(EraseUnitHeader);
		}
        else
        {
            header.metadata.bits.type = LOG;
            eraseUnitList[i].nextFreeOffset = FLASH_BLOCK_SIZE - sizeof(LogEntry);
            eraseUnitList[i].bytesFree = FLASH_BLOCK_SIZE-sizeof(EraseUnitHeader) - sizeof(LogEntry);
        }
        
		embsys_flash_write(euAddress,sizeof(EraseUnitHeader),(unsigned char*)&header);
		header.metadata.bits.valid = 0;
		embsys_flash_write(euAddress + ((unsigned long)&(((EraseUnitHeader*)0)->metadata.data)), 1, (unsigned char*)&header.metadata.data);

		eraseUnitList[i].eraseNumber = 0;
		eraseUnitList[i].validDescriptors = 0;
		eraseUnitList[i].totalDescriptors = 0;
		eraseUnitList[i].deleteFilesTotalSize = 0;
		eraseUnitList[i].metadata = header.metadata;
		euAddress+=FLASH_BLOCK_SIZE;
	}
	
	logIndex = 0;
    fileCount = 0;
	return FS_SUCCESS;
}

/* parse an EraseUnit on the flash and update the EU list accordingly */
FS_STATUS ParseEraseUnitContent(unsigned int i)
{
   unsigned int baseAddress = i*FLASH_BLOCK_SIZE, secDescOffset = sizeof(EraseUnitHeader);
   SectorDescriptor sd;

   do
   {
        embsys_flash_read(baseAddress+secDescOffset,sizeof(SectorDescriptor),(unsigned char*)&sd);
        
        /* check if we wrote the size and offset succesfully */
        if (sd.metadata.bits.validOffset == 0)
        {
            eraseUnitList[i].bytesFree-=sd.size;
            eraseUnitList[i].nextFreeOffset-=sd.size;

            /* if this file is not valid, add its size */
            if (sd.metadata.bits.validDesc == 1 || sd.metadata.bits.obsoleteDes == 0)
                eraseUnitList[i].deleteFilesTotalSize+=sd.size;
        }

        if (sd.metadata.bits.validDesc == 0 && sd.metadata.bits.obsoleteDes == 1)
            ++eraseUnitList[i].validDescriptors;
        ++eraseUnitList[i].totalDescriptors;
        secDescOffset += sizeof(SectorDescriptor);
    }while(!sd.metadata.bits.lastDesc && secDescOffset + sizeof(SectorDescriptor) <= FLASH_BLOCK_SIZE);

    eraseUnitList[i].bytesFree -= sizeof(SectorDescriptor) * eraseUnitList[i].totalDescriptors;
    eraseUnitList[i].deleteFilesTotalSize += sizeof(SectorDescriptor) * (eraseUnitList[i].totalDescriptors - eraseUnitList[i].validDescriptors);
    return FS_SUCCESS;
}

/* check if a given data segment is non-initialized i.e all Fs */
bool isSegmentUninitialized(unsigned char* pSeg,unsigned int size)
{
	while(size-- != 0)
		if (*pSeg++ != 0xFF)
			return false;
	return true;
}

/* fix an invalid LogEntry, assuming it is initialized */
void fixLogEntry(LogEntry* pEntry)
{
    LogEntry entry;
    unsigned int minEN;
    unsigned char i;

    /* check entry invalidity */
    if (pEntry->bits.valid != 0)
    {
        entry.data = FLASH_UNINIT_4_BYTES;
        minEN = MAX_UINT_32;

        /* iterate over the LogEntry and pick one that fits the partial log entry, or lowest number if collision */
        for(i = 0 ; i < ERASE_UNITS_NUMBER ; ++i)
        {
            if (i == logIndex)
                continue;

            /* set current EraseUnit info */
            entry.bits.eraseNumber = eraseUnitList[i].eraseNumber + 1;
            entry.bits.eraseUnitIndex = i;

            /* check if pEntry zeros bits are subset of entry */
            if ((entry.data & pEntry->data) == entry.data && minEN < eraseUnitList[i].eraseNumber)
            {
                minEN = eraseUnitList[i].eraseNumber;
                *pEntry = entry;
            }
        }
    }
}

/* check if there was a crash during compaction and complete it in that case */
FS_STATUS CompleteCompaction(unsigned char firstLogIndex, bool secondLogExists,
	unsigned char secondLogIndex, bool corruptedEUExists, unsigned char corruptederaseUnitIndex)
{
    LogEntry firstEntry,secondEntry;
    READ_LOG_ENTRY(firstLogIndex,firstEntry);

    if (secondLogExists)
    {
        READ_LOG_ENTRY(secondLogIndex,secondEntry);

        /* both entries cannot be non-initialized */
        if (firstEntry.data == FLASH_UNINIT_4_BYTES && secondEntry.data == FLASH_UNINIT_4_BYTES)
            return FS_ERROR;

        /* if the first entry is non-initialized and the second is, the first is the real one and we are trying to erase the second */
        if (firstEntry.data != FLASH_UNINIT_4_BYTES)
        {
            logIndex = firstLogIndex;
            firstEntry.bits.eraseUnitIndex = secondLogIndex;
            firstEntry.bits.valid = 0;
        }
        /* if the first entry is initialized and the second is not, the second is the real one and we are trying to erase the first */
        else
        {
            logIndex = secondLogIndex;
            secondEntry.bits.eraseUnitIndex = firstLogIndex;
            secondEntry.bits.valid = 1;
        }
        firstEntry.data &= secondEntry.data;
    }

    /* no valid entry, therefore FS didn't crash during block compaction, do nothing */
    if (firstEntry.data == FLASH_UNINIT_4_BYTES)
        return FS_SUCCESS;

    /* fix the invalid log entry */
    fixLogEntry(&firstEntry);

    /* check for corrupted EraseUnit */
    if (corruptedEUExists && firstEntry.bits.eraseUnitIndex != corruptederaseUnitIndex)
        return FS_ERROR;

    return CompactBlock(firstEntry.bits.eraseUnitIndex,firstEntry);
}

/* load structure from flash */
FS_STATUS loadFilesystem()
{
	unsigned char corruptedCanariesCount = 0,firstLogIndex=1,secondLogIndex = 0,corruptederaseUnitIndex = 0, logCount = 0;
	unsigned int euAddress = 0;
    fileCount = 0;
    EraseUnitHeader header;
    memset(eraseUnitList,0x00,sizeof(eraseUnitList));
    
    /* read the EraseUnit headers and parse the sector descriptor list */
	for(unsigned int i = 0; i < ERASE_UNITS_NUMBER; ++i)
	{
		unsigned int dataBytesInUse = 0;
		embsys_flash_read(euAddress,sizeof(EraseUnitHeader),(unsigned char*)&header);
      
        /* check for valid header structure. we allow one corrupted EraseUnit (if the module crashes during a block erase) */
		if (header.metadata.bits.valid == 1 || header.canary != CANARY_VALUE)
		{
			++corruptedCanariesCount;
			if (corruptedCanariesCount > 1)
				break;
            corruptederaseUnitIndex = i;
			euAddress+=FLASH_BLOCK_SIZE;
			continue;
		}
      
		eraseUnitList[i].eraseNumber = header.eraseNumber;
		eraseUnitList[i].metadata.data = header.metadata.data;
		eraseUnitList[i].bytesFree = FLASH_BLOCK_SIZE - sizeof(EraseUnitHeader) - sizeof(LogEntry);
        eraseUnitList[i].nextFreeOffset = FLASH_BLOCK_SIZE - sizeof(LogEntry);

		/* note valid logs */
		if (header.metadata.bits.type == LOG)
		{
			logCount++;
			if (logCount > 2)
				break;
			if (logCount == 2)
				secondLogIndex = i;
			else
				firstLogIndex = i;
		}
		/* regular sector */
        else if (eraseUnitList[i].metadata.bits.emptyEraseUnit == 0)
		{
			ParseEraseUnitContent(i);
            fileCount+=eraseUnitList[i].validDescriptors;
		}
		euAddress+=FLASH_BLOCK_SIZE;
	}

   /* if no log found, or corrupted flash, install the system */
   if (logCount == 0 || logCount > 2 || corruptedCanariesCount > 1)
      return installFileSystem();
      
   if (logCount == 1)
       logIndex = firstLogIndex;

   /* if we have two logs, we are in the middle of compaction and at the end of the compaction we set the true logIndex */
   return CompleteCompaction(firstLogIndex,logCount > 1, secondLogIndex, corruptedCanariesCount>0, corruptederaseUnitIndex);
}

void flashReadDoneCB(unsigned char* buffer, unsigned int count)
{
}
void flashDoneCB(void)
{
	tx_event_flags_set(&fsGlobalEventFlags,1,TX_OR);
}

/* writes a file to a given EraseUnit */
FS_STATUS writeFile(const char* filename, unsigned length, const char* data,unsigned char eraseUnitIndex)
{
    SectorDescriptor desc;
    EraseUnitHeader header;

    /* calculate the absolute address of the next free sector descriptor */
    unsigned int sectorDescAddr = FLASH_BLOCK_SIZE*eraseUnitIndex + sizeof(EraseUnitHeader) 
    	+ sizeof(SectorDescriptor)*eraseUnitList[eraseUnitIndex].totalDescriptors;

    /* calculate the absolute address of the content to be written */
    unsigned int dataAddr = FLASH_BLOCK_SIZE*eraseUnitIndex + eraseUnitList[eraseUnitIndex].nextFreeOffset - length;
    memset(&desc,0xFF,sizeof(SectorDescriptor));

    /* mark previous sector/EUHeader as non-last/empty */
    if (eraseUnitList[eraseUnitIndex].totalDescriptors != 0)
    {
	    memset(&desc,0xFF,sizeof(SectorDescriptor));
	    desc.metadata.bits.lastDesc = 0;
	    embsys_flash_write(sectorDescAddr - sizeof(SectorDescriptor),sizeof(SectorDescriptor),(unsigned char*)&desc);
        desc.metadata.bits.lastDesc = 1;
    }
    else
    {
	    memset(&header,0xFF,sizeof(EraseUnitHeader));
	    header.metadata.bits.emptyEraseUnit = 0;
	    embsys_flash_write(FLASH_BLOCK_SIZE*eraseUnitIndex,sizeof(EraseUnitHeader),(unsigned char*)&header);
    }
    
    /* fill sector descriptor info, write it and mark metadata as valid */
    strncpy(desc.fileName,filename,FILE_NAME_MAX_LEN);
    desc.offset = eraseUnitList[eraseUnitIndex].nextFreeOffset - length;
    desc.size = length;
    embsys_flash_write(sectorDescAddr,sizeof(SectorDescriptor),(unsigned char*)&desc);
    desc.metadata.bits.validOffset = 0;
    embsys_flash_write(sectorDescAddr,sizeof(SectorDescriptor),(unsigned char*)&desc);

    /* write file content and mark file as valid */
    embsys_flash_write(dataAddr,length,(unsigned char*)data);
    desc.metadata.bits.validDesc = 0;
    embsys_flash_write(sectorDescAddr,sizeof(SectorDescriptor),(unsigned char*)&desc);
    
    /* update FS info */
    eraseUnitList[eraseUnitIndex].bytesFree -= (sizeof(SectorDescriptor) + length);
    eraseUnitList[eraseUnitIndex].nextFreeOffset-=length;
    eraseUnitList[eraseUnitIndex].totalDescriptors++;
    eraseUnitList[eraseUnitIndex].validDescriptors++;
    ++fileCount;
    return FS_SUCCESS;
}

/* search for an EraseUnit with free space for a file descriptor, file content and work according to a first fit algorithm */
bool findFreeEraseUnit(unsigned int fileSize,unsigned char* eraseUnitIndex)
{
	for(unsigned char i = 0 ; i < ERASE_UNITS_NUMBER ; ++i)
		if (logIndex != i && eraseUnitList[i].bytesFree >= (fileSize + sizeof(SectorDescriptor)))
		{
			*eraseUnitIndex = i;
			return true;
		}
	return false;
}


/* search for an EraseUnit with enough free space after compaction to store a given file size and a sector descriptor */
bool findEraseUnitToCompact(unsigned char* eraseUnitIndex,unsigned size)
{
    unsigned int minEN = MAX_UINT_32, maxFreeBytes = 0;
    bool found = false;

    for(unsigned char i = 0 ; i < ERASE_UNITS_NUMBER ; ++i)
    {
        if (i == logIndex)
            continue;

        /* if few EU share the minimal EraseNumber, choose the one which will yield the largest space aftet compaction */
        if (((unsigned int)(eraseUnitList[i].deleteFilesTotalSize+eraseUnitList[i].bytesFree) >= size) 
        	&& (eraseUnitList[i].eraseNumber <= minEN 
        		|| (eraseUnitList[i].eraseNumber == minEN 
        			&& maxFreeBytes <= (eraseUnitList[i].deleteFilesTotalSize+eraseUnitList[i].bytesFree))))
        {
            minEN = eraseUnitList[i].eraseNumber;
            maxFreeBytes = eraseUnitList[i].deleteFilesTotalSize+eraseUnitList[i].bytesFree;
            *eraseUnitIndex = i;
            found = true;
        }
    }
    return found;
}

/* copy content inside flash to another place */
FS_STATUS CopyDataInsideFlash(unsigned int srcAddr,unsigned int srcSize,unsigned int destAddr)
{
	unsigned int bytesToRead;
	unsigned char data[FDATA_MAX_SIZE];

	/* read and write data in FDATA_MAX_SIZE chunks */
	while (srcSize > 0)
	{
		bytesToRead = (srcSize > FDATA_MAX_SIZE)?FDATA_MAX_SIZE:srcSize;

		embsys_flash_read(srcAddr,bytesToRead,data);
		embsys_flash_write(destAddr,bytesToRead,data);

		srcAddr+=bytesToRead;
		destAddr+=bytesToRead;
		srcSize-=bytesToRead;
	}
	return FS_SUCCESS;
}

/* compact an EraseUnit and supply the LogEntry as argument if the previous compaction failed */
FS_STATUS CompactBlock(unsigned char eraseUnitIndex,LogEntry entry)
{
	EraseUnitHeader header;
	unsigned long int actualFlags;
    unsigned int actualNextFileDescIdx = 0,secDescAddr,dataOffset=0;
    unsigned char i;
	FS_STATUS status;
    SectorDescriptor sec;

    /* if the LogEntry is uninitialized, initialize it with data */
    if (entry.bits.valid == 1)
    {
        entry.bits.eraseUnitIndex = eraseUnitIndex;
        entry.bits.eraseNumber = eraseUnitList[eraseUnitIndex].eraseNumber+1;
        WRITE_LOG_ENTRY(logIndex,entry);
    }

    /* mark the entry as valid if needed */
    if (entry.bits.valid == 1)
    {
        entry.bits.valid = 0;
        WRITE_LOG_ENTRY(logIndex,entry);
    }
    
    /* if we files are not copied yet */
    if (entry.bits.copyComplete == 1)
    {
        memset(&header,0xFF,sizeof(EraseUnitHeader));

        /* mark the header as non empty */
        if (eraseUnitList[eraseUnitIndex].validDescriptors > 0)
        {
            header.metadata.bits.emptyEraseUnit = 0;
            eraseUnitList[logIndex].metadata.bits.emptyEraseUnit = 0;
        }

    	embsys_flash_write(logIndex*FLASH_BLOCK_SIZE,sizeof(EraseUnitHeader),(unsigned char*)&header);

        /* set initial address and offset */
        secDescAddr = logIndex*FLASH_BLOCK_SIZE + sizeof(EraseUnitHeader);
        dataOffset = FLASH_BLOCK_SIZE - sizeof(LogEntry);

        /* copy files */
        for(i = 0 ; i < eraseUnitList[eraseUnitIndex].validDescriptors ; ++i)
        {
            getNextValidSectorDescriptor(eraseUnitIndex,&actualNextFileDescIdx,&sec);
            dataOffset -= sec.size;

            /* copy the content to the new block */
            if ((status = CopyDataInsideFlash(FLASH_BLOCK_SIZE*eraseUnitIndex + sec.offset,sec.size,FLASH_BLOCK_SIZE*logIndex + dataOffset)) != FS_SUCCESS)
				return status;

            sec.offset = dataOffset;

            /* set the file's sector descriptor */
            embsys_flash_write(secDescAddr,sizeof(SectorDescriptor),(unsigned char*)&sec);
            secDescAddr+=sizeof(SectorDescriptor);
        }
        entry.bits.copyComplete = 0;
        embsys_flash_write(logIndex*FLASH_BLOCK_SIZE,sizeof(EraseUnitHeader),(unsigned char*)&header);
    }
    
    /* erase the block if need be */
    if (entry.bits.eraseComplete == 1)
    {
        embsys_flash_delete(FLASH_BLOCK_SIZE*eraseUnitIndex);
		tx_event_flags_get(&fsGlobalEventFlags,1,TX_AND_CLEAR,&actualFlags,TX_WAIT_FOREVER);
        entry.bits.eraseComplete = 0;
		embsys_flash_write(logIndex*FLASH_BLOCK_SIZE,sizeof(EraseUnitHeader),(unsigned char*)&header);
    }
    
    /* set log header to this erased block */
    memset(&header,0xFF,sizeof(EraseUnitHeader));
    header.canary = CANARY_VALUE;
    header.eraseNumber = entry.bits.eraseNumber;
    header.metadata.bits.type = LOG;
    header.metadata.bits.valid = 0;
    embsys_flash_write(eraseUnitIndex*FLASH_BLOCK_SIZE,sizeof(EraseUnitHeader),(unsigned char*)&header);
    
    /* change the old log's type to data type */
    memset(&header,0xFF,sizeof(EraseUnitHeader));
    header.metadata.bits.type = DATA;
	embsys_flash_write(logIndex*FLASH_BLOCK_SIZE,sizeof(EraseUnitHeader),(unsigned char*)&header);

    eraseUnitList[eraseUnitIndex].eraseNumber++;
    eraseUnitList[logIndex].bytesFree = eraseUnitList[eraseUnitIndex].bytesFree + eraseUnitList[eraseUnitIndex].deleteFilesTotalSize;
    eraseUnitList[logIndex].deleteFilesTotalSize = 0;
    eraseUnitList[logIndex].nextFreeOffset = dataOffset;
    eraseUnitList[logIndex].totalDescriptors = eraseUnitList[logIndex].validDescriptors = eraseUnitList[eraseUnitIndex].validDescriptors;
    eraseUnitList[eraseUnitIndex].metadata.bits.type = LOG;
    
    logIndex = eraseUnitIndex;
	return FS_SUCCESS;
}

/* get the next valid sector descriptor inside a given EraseUnit */
FS_STATUS getNextValidSectorDescriptor(unsigned char eraseUnitIndex,unsigned int* pNextActualSectorIndex,SectorDescriptor* pSecDesc)
{
	unsigned int baseAddress = FLASH_BLOCK_SIZE*eraseUnitIndex,secDescOffset = sizeof(EraseUnitHeader) ,sectorIndexInc = 0;

	/* if the sector index is relevant, advance the index */
	if (pNextActualSectorIndex != NULL && *pNextActualSectorIndex > 0)
		secDescOffset += sizeof(SectorDescriptor)*(*pNextActualSectorIndex);

	do
	{
		embsys_flash_read(baseAddress + secDescOffset,sizeof(SectorDescriptor),(unsigned char*)pSecDesc);
		secDescOffset+=sizeof(SectorDescriptor);
		++sectorIndexInc;
	}while(pSecDesc->metadata.bits.validDesc != 0 || pSecDesc->metadata.bits.obsoleteDes == 0 && secDescOffset + sizeof(SectorDescriptor) < FLASH_BLOCK_SIZE);
	--sectorIndexInc;

	/* update the actual index if the next has been provided */
	if (pNextActualSectorIndex != NULL)
		*pNextActualSectorIndex+=(sectorIndexInc + 1);
	return FS_SUCCESS;
}

/* search for a sector descriptor that matchs a given file name */
FS_STATUS getSectorDescriptor(const char* filename,SectorDescriptor *pSecDesc,unsigned char *pEraseUnitIndex,unsigned int* pSecActualIndex)
{
	unsigned int secIdx,nextActualSecIdx;
	bool found = false;
	unsigned char euIdx;

	for(euIdx = 0 ; euIdx < ERASE_UNITS_NUMBER ; ++euIdx)
	{
		/* skip the log erase unit since we cannot save content in there */
		if (logIndex == euIdx)
			continue;
		nextActualSecIdx = 0;

		/* parse the sector descriptor and check if the given filename matches */
		for(secIdx = 0 ; secIdx < eraseUnitList[euIdx].validDescriptors ; ++secIdx)
		{
			if (getNextValidSectorDescriptor(euIdx,&nextActualSecIdx,pSecDesc) != FS_SUCCESS)
				break;
			
			/* check if filename matches */
			char fsFileName[FILE_NAME_MAX_LEN+1] = {'\0'};
			memcpy(fsFileName, pSecDesc->fileName, FILE_NAME_MAX_LEN);
			if(strcmp(fsFileName,filename)==0)
			{
				found = true;
				break;
			}
		}
		if (found)
			break;
	}

	/* if match found, update the indexes */
	if (found)
	{
		if (pEraseUnitIndex != NULL)
			*pEraseUnitIndex = euIdx;
		if (pSecActualIndex != NULL)
			*pSecActualIndex = nextActualSecIdx-1;
	}
	return found?FS_SUCCESS:FILE_NOT_FOUND;
}

/* erase a given filename from FS */
FS_STATUS eraseFileFromFS(const char* filename)
{
	SectorDescriptor sec;
	FS_STATUS status;
	unsigned int actualSecIndex;
	unsigned char secEraseUnitIndex;

	status = getSectorDescriptor(filename,&sec,&secEraseUnitIndex,&actualSecIndex);
	if (status != FS_SUCCESS)
		return status;

	/* mark the file as obsolete and write it to the flash */
	sec.metadata.bits.obsoleteDes = 0;
	embsys_flash_write(FLASH_BLOCK_SIZE*secEraseUnitIndex + sizeof(EraseUnitHeader) 
		+ sizeof(SectorDescriptor)*actualSecIndex,sizeof(SectorDescriptor),(unsigned char*)&sec);

	--eraseUnitList[secEraseUnitIndex].validDescriptors;
	eraseUnitList[secEraseUnitIndex].deleteFilesTotalSize+=(sec.size + sizeof(SectorDescriptor));
	--fileCount;
	return FS_SUCCESS;
}

