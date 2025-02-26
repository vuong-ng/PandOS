#ifndef SCHEDULER
#define SCHEDULER

#include "../h/types.h"
extern cpu_t quantum_start_time; /*records starting time of current process's quantum*/

extern void scheduler();
extern void switchContext(pcb_PTR to_be_executed);
#endif