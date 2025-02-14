#ifndef INITIAL
#define INITIAL

#include "../h/const.h"
#include "../h/types.h"

extern void test();


int process_cnt; /*number of started, but not yet terminated processes*/
int softblock_cnt; /*number of started, but not terminated processes that in are the “blocked” state due to an I/O or timer request*/
pcb_PTR ready_queue; /*tail pointer to a queue of pcbs that are in the “ready” state*/
pcb_PTR curr_proc; /*pointer to the pcb that is in the “running” state, i.e. the current executing process.*/
int* device_sem1;
int* device_sem2; 
int* clock_sem;



devregarea_t device_reg;







#endif