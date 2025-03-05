#include "../h/initial.h"
#include "../h/scheduler.h"
#include "../h/pcb.h"

/*records time intervals (Time Of Day)*/
cpu_t time_start; 

/*************************************************/
/* Scheduler                                     */
/* Purpose: dispatch the next process            */
/* to run based on ready queue and system state  */
/*                                               */
/* Cases handled:                                */
/* 1. Ready queue empty:                         */
/*    - No processes: HALT system                */
/*    - Blocked processes: WAIT for interrupt    */
/*    - No blocked processes: PANIC (deadlock)   */
/* 2. Ready queue not empty:                     */
/*    - Load time slice                         */
/*    - Start process execution                  */
/*                                               */
/* Parameters: none                              */
/* Returns: void (never returns directly)        */
/*************************************************/
void scheduler()
{
    /*get the next process in ready queue*/
    curr_proc = removeProcQ(&ready_queue); 

    if(curr_proc == NULL)
    /*ready queue empty*/
    {
        if (process_cnt == 0)
        /*If the Process Count is zero invoke the HALT BIOS service/instruction*/
            HALT();

        if (process_cnt > 0 && softblock_cnt > 0)
        /*If the Process Count > 0 and the Soft-block Count > 0 enter a Wait State*/
        {
            setSTATUS((IECBITON | IMON) & TEBITOFF);        /*enable interrupts and disable PLT*/
            WAIT();  
        }

        /*Deadlock for Pandos is defined as when the Process Count > 0 and the Soft-block Count is zero.*/
        if (process_cnt > 0 && softblock_cnt == 0)
        /* when the Process Count > 0 and the Soft-block Count is zero, enter Deadlock*/
            PANIC();
    }

    else
    /*ready queue not empty*/
    {   
        setTIMER(TIMESLICE);        /*load 5 milliseconds into the PLT*/
        STCK(time_start);           /*start quantum of current process*/
        LDST(&(curr_proc->p_s));    /*perform Load Process State stored in pcb of the Current Process*/
    }
}

/*************************************************/
/* Switch Context Handler                         */
/* Purpose: Updates CPU time accounting and       */
/* transfers control to a new process            */
/*                                               */
/* Parameters:                                    */
/* - to_be_executed: PCB of process to execute   */
/*                                               */
/* Returns: void (never returns directly)        */
/*************************************************/
void switchContext(pcb_PTR to_be_executed)
{
    /*initilize time variable*/
    cpu_t time_stop;

    /* Get current timestamp*/
    STCK(time_stop);

    /*Add delta between start/stop to process time*/
    curr_proc->p_time += (time_stop - time_start);

    /*load state of process to be executed, return processor to whatever interrupt state and mode was in 
    effect when the exception occured*/
    LDST(&to_be_executed->p_s);     /*put saved state in next process' state*/
}