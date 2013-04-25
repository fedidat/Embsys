#ifndef SEVEN_SEGMENTS_H_
#define SEVEN_SEGMENTS_H_

#define SEVEN_SEGMENTS_ADDR (0x104)

/**
 * Write a new value to the 7-segments display
 */
void embsys_7segments_write(char value);

#endif /*SEVEN_SEGMENTS_H_*/
