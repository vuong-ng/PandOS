#include "../h/initial.h"
#include "../h/pcb.h"
#include "../h/asl.h"
int main()
{
    /*Initialize Level 2 variables*/
    initPcbs();
    initASL();

    /*Initialize Nucleus maintained variables*/
    process_cnt = 0;
    softblock_cnt = 0;
    ready_queue = mkEmptyProcQ();
    curr_proc = NULL;
    
    /*THAY OI CAI LON GI THE NAY*/
    *device_sem1 = 0;
    *device_sem2 = 0;

    /*Load the system-wide Interval Timer with 100 milliseconds*/
    device_reg.intervaltimer = 0x186A0;

    /*Instantiate a single process*/
    curr_proc = allocPcb();
    insertProcQ(&ready_queue, curr_proc); /*place pcb in Ready Queue*/
    process_cnt++; /*increment process_cnt*/


    /*initializing the processor state */
    /*interrupt (0) enabled, Local Timer (27) enabled, kernel mode on (1)*/
    curr_proc->p_s.s_status = 0b00001000000000000000000000000100 ; 
    curr_proc->p_s.s_sp = RAMBASEADDR + RAMBASESIZE;  /*set stack pointer to RAMTOP*/
    curr_proc->p_s.s_pc = (memaddr) test;   /*set pc to test*/
    curr_proc->p_s.s_t9 = (memaddr) test; 


    /*set all Proc Tree fields to NULL*/
    curr_proc->p_prnt = NULL;
    curr_proc->p_child = NULL;
    curr_proc->p_sib = NULL;
    curr_proc->p_sib_left = NULL;

    curr_proc->p_time = 0;
    curr_proc->p_semAdd = NULL;
    /*curr_proc->p_supportStruct = NULL;*/


    /*call the Scheduler*/

    


}