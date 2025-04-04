/******************************* EXCEPTIONS.h *********************************
* Written by Long Pham & Vuong Nguyen
* Module: Exception and system call handling declarations for Pandos OS
*
* Exception Handlers:
* - generalExceptionHandler(): Routes exceptions by type
* - uTLB_RefillHandler(): Handles TLB refill events
* - syscallHandler(): Handles system calls (SYS1-SYS8)
* - trapHandler(): Handles program traps
*
* Process Management:
* - passUpOrDie(): Passes up exception or terminates process
* - killProcess(): Terminates single process
* - killDescendants(): Recursively terminates process tree
*
* Helper Functions:
* - increasePC(): Advances program counter by 4
* - updateTime(): Updates process CPU time
* - copyState(): Copies processor state
* - isDeviceSem(): Checks if semaphore belongs to device
*
* Dependencies:
* - types.h: System type definitions
* - const.h: System constants
*****************************************************************************/

#ifndef EXCEPTIONS
#define EXCEPTIONS
#include "../h/types.h"

extern void generalExceptionHandler();          
/* Routes different types of exceptions to their specific handlers */

extern void uTLB_RefillHandler();
/* Handles TLB refill events from BIOS */

/*extern void syscallHandler();*/
/* Processes 8 types of system calls (SYS1-SYS8) */

/*extern void trapHandler();*/
/* Handles program traps by passing up or terminating */

extern void passUpOrDie(int passup_type);
/* Passes exception up to support level or terminates process */

extern void killProcess(pcb_PTR proc);
/* Terminates a single process and cleans up resources */

extern void killDescendants(pcb_PTR first_child);
/* Recursively terminates a process and all its descendants */

extern void Passeren(int* semAdd);
/* Performs P (wait) operation on semaphore */

extern pcb_PTR Verhogen(int* semAdd);
/* Performs V (signal) operation on semaphore */

/*Helper Functions*/
extern void increasePC();               /* Advances program counter by 4 */
extern void updateTime(pcb_PTR proc);   /* Updates CPU time used by a process */
extern void copyState(state_t* dest, state_t* src);     /* Copies processor state from source to destination */
extern int isDeviceSem(int* semAdd);    /* Checks if semaphore belongs to a device */
extern void syscallReturnToCurr();   /*increase PC and transfer control to current process*/



#endif
