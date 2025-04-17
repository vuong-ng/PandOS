/******************************** SYSSUPPORT.H *********************************
* Written by Long Pham & Vuong Nguyen
* Module: Support level syscall handler header for Pandos OS Phase 3
* 
* Dependencies:
* - initProc.h: U-proc initialization and launch processes
*
* Exposed Functions:
* - sptGeneralExceptionHandler(): Routes exceptions to appropriate handlers
* - sptTrapHandler(): Handles program traps and errors
*
* Exception Types Handled:
* - System calls (cause code 8)
* - Program traps
***************************************************************************/
#ifndef SYSSUPPORT
#define SYSSUPPORT

#include "initProc.h"

/* routes exceptions to appropriate handlers based on cause code */
extern void sptGeneralExceptionHandler();

/* handles program traps by terminating process */
extern void sptTrapHandler(support_t* sPtr, int* swap_pool_sem);
#endif