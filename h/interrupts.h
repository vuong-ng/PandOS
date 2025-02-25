#ifndef INTERRUPTS
#define INTERRUPTS
#include "../h/initial.h"
#include "../h/exceptions.h"
#define CLEAR31MSB 0x00000001
#define GETINTLINE 8

extern void interruptHandler(unsigned int* cause);

#endif