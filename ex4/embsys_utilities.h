#ifndef EMBSYS_UTILITIES_H_
#define EMBSYS_UTILITIES_H_

#include <stdint.h>
#include <stdbool.h>

#ifndef NULL
#define NULL 0x0
#endif

#define TX_TICK_MS 5


typedef enum
{
    MESSAGE_LISTING_SCREEN	= 0,
    MESSAGE_DISPLAY_SCREEN	= 1,
    MESSAGE_EDIT_SCREEN		= 2,
    MESSAGE_NUMBER_SCREEN	= 3,
} screen_type;

typedef unsigned int TX_STATUS;

/* Converts a number from decimal int to hex char */
unsigned char decToHex(int num);

/* Converts a char from hex char to decimal int */
int hexToDec(unsigned char c);

/* Indiciates if a char is in hex */
int isCharHex(unsigned char c);

#endif /*EMBSYS_UTILITIES_H_*/

