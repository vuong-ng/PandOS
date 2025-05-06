#ifndef DEVICESUPPORTDMA
#define DEVICESUPPORTDMA

#include "const.h"
#include "types.h"
#include "initProc.h"

extern int diskOperation(unsigned int dev_no, unsigned int data0, unsigned int sect_no, unsigned int command);
extern int flashOperation(unsigned int dev_no, unsigned int data0, unsigned int blocknumber, unsigned int command);
#endif