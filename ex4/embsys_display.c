#include "embsys_display.h"

#define DISPLAY_DBUF 0x1F0
#define DISPLAY_DCMD 0x1F1
#define DISPLAY_DIER 0x1F2
#define DISPLAY_DICR 0x1F3
#define SELECTED_BIT 7

typedef union
{
	unsigned char data;
	struct
	{
		unsigned char charEncode	: 7;
		unsigned char selected	: 1;
	}byte;
}DisplayCharacter;

DisplayCharacter displayData[SCREEN_WIDTH*SCREEN_HEIGHT];
void (*flushCompleteCB)(void) = NULL;

unsigned int display_init(void (*flush_complete_cb)(void))
{
	if(!flush_complete_cb)
        	return 1;

	/* initialize registers, data array and data array */
	_sr(0,DISPLAY_DBUF);
	_sr(0,DISPLAY_DCMD);
	_sr(0,DISPLAY_DIER);
	_sr(0,DISPLAY_DICR);
	flushCompleteCB = flush_complete_cb;
	for(int i=0 ; i<SCREEN_WIDTH*SCREEN_HEIGHT ; ++i)
		displayData[i].byte.charEncode = ' ';

	return 0;
}

unsigned int display_set_row(unsigned int row_number, bool selected, char const line[], unsigned int length)
{
	/* check input and status */
	if(!line || length > SCREEN_WIDTH || row_number >= SCREEN_HEIGHT || _lr(DISPLAY_DCMD) & 0x1)
		return 1;
	if(length == 0)
		return 0;

	/* write the data to the array */
	for(int i=0 ; i<length ; ++i)
	{	
		displayData[SCREEN_WIDTH * row_number + i].byte.charEncode = line[i];
		displayData[SCREEN_WIDTH * row_number + i].byte.selected = selected?1:0;
	}
	return 0;
}

void display_flush()
{
	/* write data to buffer, enable completion interrupt, send write signal */
	_sr((unsigned int)&displayData, DISPLAY_DBUF);
	_sr(0x1, DISPLAY_DIER);
	_sr(0x1, DISPLAY_DCMD);
}

void displayISR()
{	
	/* clear interrupt, call back */
	_sr(0x1,DISPLAY_DICR);
	flushCompleteCB();
}

