/******************************** INITPROC.H *********************************
* Written by Long Pham & Vuong Nguyen
* Module: User Process Initialization Header for Pandos OS Phase 3
* 
* Dependencies:
* - const.h: System constants
* - types.h: System data types
* - initial.h: System initialization
* - vmSupport.h: Virtual memory support
* - sysSupport.h: System call support
*
* Exposed Functions:
* - test(): Initializes and launches user processes
* - disableInterrupts(): Disables interrupts
* - enableInterrupts(): Enables interrupts
*
* Global Variables:
* - mutex_device_sem[DEVSEMNO]: Device mutex semaphores array
* - masterSemaphore: Master semaphore for phase 3 graceful termination of test() 
***************************************************************************/

#ifndef INITPROC
#define INITPROC

#include "const.h"      
#include "types.h"     

#include "initial.h"
#include "vmSupport.h"
#include "sysSupport.h"

/*test to initialize and launch u-proc*/
extern void test();

/* disables interrupts by clearing IEc bit */
extern void disableInterrupts();

/* enables interrupts by setting IEc bit */
extern void enableInterrupts();

/* create array of device mutex semaphores*/
extern int mutex_device_sem[DEVSEMNO];

/* create master semaphore for grateful termination of test()*/
extern int masterSemaphore;

#endif