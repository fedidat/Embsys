#ifndef CLOCK_H_
#define CLOCK_H_

#define CLOCK_ADDR (0x100)

/**
 * Read the value of the clock, in milli-seconds
 */
unsigned int embsys_clock_read();

#endif /*CLOCK_H_*/
