#ifndef EMBSYS_7SEGMENTS_H_
#define EMBSYS_7SEGMENTS_H_

#define EMBSYS_7SEGMENTS_ADDR (0x104)

/**
 * Write a new value to the 7-segments display
 */
void embsys_7segments_write(char value);

#endif /*EMBSYS_7SEGMENTS_H_*/
