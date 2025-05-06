/******************************************************************************
 * Module: delayDaemon.h
 * Description: Header file for delay facility
 *
 * Global functions:
 * - delay(): SYS18 handler to delay a process
 * - initADL(): Initialize delay system and daemon
 *
 * Dependencies:
 * - const.h: System constants
 * - types.h: System type definitions
 * - initProc.h: Process initialization
 ******************************************************************************/

#ifndef DELAYDAEMON
#define DELAYDAEMON
#include "const.h"
#include "types.h"
#include "initProc.h"

/* SYS18 handler - delays process for specified time */
extern void delay(support_t* sPtr);

/* Initialize Active Delay List and delay daemon */
extern void initADL();
#endif