#include "../h/initial.h"
#include "../h/scheduler.h"
#include "../h/pcb.h"

/*records time intervals (TOD)*/
cpu_t time_start; 

void scheduler()
{
    /*get the next process in ready queue*/
    curr_proc = removeProcQ(&ready_queue); 

    /*ready queue empty*/
    if(curr_proc == NULL)
    {
        /*If the Process Count is zero invoke the HALT BIOS service/instruction*/
        if (process_cnt == 0)
            HALT();

        /*If the Process Count > 0 and the Soft-block Count > 0 enter a Wait State*/
        if (process_cnt > 0 && softblock_cnt > 0)
        {
            setSTATUS((IECBITON | IMON) & TEBITOFF);  /*enable interrupts and disable PLT*/
            WAIT();  
        }

        /*Deadlock for Pandos is defined as when the Process Count > 0 and the Soft-block Count is zero.*/
        if (process_cnt > 0 && softblock_cnt == 0)
            PANIC();
    }

    /*ready queue not empty*/
    else
    {   
        setTIMER(TIMESLICE); /*load 5 milliseconds into the PLT*/
        STCK(time_start);  /*start quantum of current process*/
        LDST(&(curr_proc->p_s));    
    }
}

void switchContext(pcb_PTR to_be_executed)
{
    /*load time*/
    cpu_t time_stop;
    STCK(time_stop);
    curr_proc->p_time += (time_stop - time_start);

    /*load state of process to be executed, return processor to whatever interrupt state and mode was in 
    effect when the exception occured*/
    LDST(&to_be_executed->p_s); /*put saved state in next_proc's state*/
}