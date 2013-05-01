#ifndef EMBSYS_CLOCK_H_
#define EMBSYS_CLOCK_H_

#define EMBSYS_CLOCK_ADDR (0x100)

/**
 * Read the value of the clock, in milli-seconds
 */
unsigned int embsys_clock_read();

#endif /*EMBSYS_CLOCK_H_*/
