#ifndef INITIAL
#define INITIAL

#include "../h/const.h"
#include "../h/types.h"
#include "../h/pcb.h"
#include "../h/asl.h"

#include "../h/scheduler.h"

extern void test();

extern int process_cnt; /*number of started, but not yet terminated processes*/
extern int softblock_cnt; /*number of started, but not terminated processes that in are the “blocked” state due to an I/O or timer request*/
extern pcb_PTR ready_queue; /*tail pointer to a queue of pcbs that are in the “ready” state*/
extern pcb_PTR curr_proc; /*pointer to the pcb that is in the “running” state, i.e. the current executing process.*/
extern int device_sem [49];

#endif