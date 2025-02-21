#ifndef INTERRUPTS
#define INTERRUPTS
#include "../h/initial.h"
#define CLEAR31MSB 0x00000001
#define RESETHANDLEDDEVICE 0b11111111111111111111111101111111

extern int interruptHandler(unsigned int* cause);

#endif