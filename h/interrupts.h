#ifndef INTERRUPTS
#define INTERRUPTS


#define CLEAR31MSB 0x00000001
#define GETINTLINE 8

extern void interruptHandler(unsigned int cause);

#endif