#ifndef SCHEDULER
#define SCHEDULER

#include "../h/types.h"

/*records time intervals (TOD)*/
extern cpu_t time_start; 

extern void scheduler();
extern void switchContext(pcb_PTR to_be_executed);

#endif