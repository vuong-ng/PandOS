#include "../h/initial.h"
#include "../h/exceptions.h"
int main()
{
    /*Initialize Pass Up Vector*/

    passupvector_t* xxx = PASSUPVECTOR;
    xxx->tlb_refll_handler = (memaddr) uTLB_RefillHandler;
    xxx->tlb_refll_stackPtr = 0x20001000;
    xxx->execption_handler =(memaddr) fooBar;
    xxx->exception_stackPtr = 0x20001000;


    /*Initialize Level 2 variables*/
    initPcbs();
    initASL();

    /*Initialize Nucleus maintained variables*/
    process_cnt = 0;
    softblock_cnt = 0;
    ready_queue = mkEmptyProcQ();
    curr_proc = NULL;
    
    /*initialize device_sem*/
    /*all zeros*/

    /*Load the system-wide Interval Timer with 100 milliseconds (convert to microsec)*/
    LDIT(100000);

    /*Instantiate a single process*/
    curr_proc = allocPcb();
    insertProcQ(&ready_queue, curr_proc); /*place pcb in Ready Queue*/
    process_cnt++; /*increment process_cnt*/


    /*initializing the processor state */
    /*interrupt (0) enabled, Local Timer (27) enabled, kernel mode on (1)*/
    /*macros: INTRPTENABLED, PLTENABLED, KERNELON*/
    curr_proc->p_s.s_status = 0b00001000000000000000000000000100;
    curr_proc->p_s.s_sp = RAMBASEADDR + RAMBASESIZE;  /*set stack pointer to RAMTOP*/
    curr_proc->p_s.s_pc = (memaddr) test;   /*set pc to test*/
    curr_proc->p_s.s_t9 = (memaddr) test; 

    /*INFO: LDST load these into real registers (CP0)*/


    /*set all Proc Tree fields to NULL*/
    curr_proc->p_prnt = NULL;
    curr_proc->p_child = NULL;
    curr_proc->p_sib = NULL;
    curr_proc->p_sib_left = NULL;

    curr_proc->p_time = 0;
    curr_proc->p_semAdd = NULL;
    curr_proc->p_supportStruct = NULL;


    /*call the Scheduler*/
    scheduler();

    /*do nothing here, scheduler never returns*/ 

}