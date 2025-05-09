/******************************************************************************
 * Module: deviceSupportDMA.h
 * Description: Header file for DMA-based devices support
 *
 * Global functions:
 * - diskOperation(): perform an operation on the specified disk using its device register
 * - flashOperation(): perform an operation on the specified flash using its device register
 *
 * Dependencies:
 * - const.h: System constants
 * - types.h: System type definitions
 * - initProc.h: Process initialization
 ******************************************************************************/

#ifndef DEVICESUPPORTDMA
#define DEVICESUPPORTDMA

#include "const.h"
#include "types.h"
#include "initProc.h"

extern int diskOperation(unsigned int dev_no, unsigned int data0, unsigned int sect_no, unsigned int command);
extern int flashOperation(unsigned int dev_no, unsigned int data0, unsigned int blocknumber, unsigned int command);
#endif