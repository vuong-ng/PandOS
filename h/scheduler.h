/******************************* SCHEDULER.h *********************************
*
* Module: Process scheduling declarations for Pandos OS
*
* Global Variables:
* - time_start: Records time intervals for CPU accounting
*
* External Functions:
* - scheduler(): Selects and dispatches next process to run
* - switchContext(): Switches execution to new process
*
* Scheduling Policy:
* - Round-robin with fixed time quantum
* - Ready queue for runnable processes
* - Special cases for empty queue:
*   * System halt when no processes exist
*   * Wait state when processes blocked
*   * Deadlock detection when blocked but not waiting
*
* Dependencies:
* - types.h: System type definitions
*****************************************************************************/

#ifndef SCHEDULER
#define SCHEDULER

#include "../h/types.h"

/*records time intervals (TOD)*/
extern cpu_t time_start; 

extern void scheduler();
/* Selects next process from ready queue and dispatches it to run */

extern void switchContext(pcb_PTR to_be_executed);
/* Updates CPU time for current process and switches to new process */

#endif