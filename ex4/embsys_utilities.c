#include "embsys_utilities.h"

/* Converts a number from decimal int to hex char */
unsigned char decToHex(int num) 
{
	return ((num < 10)? ('0' + num) : ('A' + num - 10));
}

/* Converts a char from hex char to decimal int */
int hexToDec(unsigned char c)
{
	if(c >= 'a' && c <= 'f')
		c = c + 'A' - 'a';
	return ((c >= 'A')? (c - 'A' + 10) : (c - '0'));
}

/* Indiciates if a char is in hex */
int isCharHex(unsigned char c)
{
	return ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'));
}
