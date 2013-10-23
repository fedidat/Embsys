#ifndef EMBSYS_UTILITIES_H_
#define EMBSYS_UTILITIES_H_

/* Converts a number from decimal int to hex char */
unsigned char decToHex(int num);

/* Converts a char from hex char to decimal int */
int hexToDec(unsigned char c);

/* Indiciates if a char is in hex */
int isCharHex(unsigned char c);

#endif /*EMBSYS_UTILITIES_H_*/

