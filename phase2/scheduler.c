#include "../h/initial.h"
#include "../h/scheduler.h"
#include "../h/pcb.h"

cpu_t quantum_start_time; /*records starting time of current process's quantum*/

void scheduler()
{
    /*Remove the pcb from the head of the Ready Queue and store the pointer to the pcb in the Current Process field*/
    
    /*ready queue not empty*/
    if(emptyProcQ(ready_queue))
    {
        debug(2,4,6,8);
        /*REDO ORDER OF CHECKING*/
        /*If the Process Count is zero invoke the HALT BIOS service/instruction.*/
        if (process_cnt == 0)
        {
            HALT();
        }

        /*If the Process Count > 0 and the Soft-block Count > 0 enter a Wait State*/
        if (process_cnt > 0 && softblock_cnt > 0)
        {
            setSTATUS(getSTATUS() | 0b00000000000000000000000000000001); /*macro: INTRPTENABLED*/    /*enable interrupt*/
            setSTATUS(getSTATUS() & 0b11110111111111111111111111111111); /*macro: PLTDISABLED*/   /*disable PLT*/
            WAIT();
        }

        /*Deadlock for Pandos is defined as when the Process Count > 0 and the Soft-block Count is zero.*/
        if (process_cnt > 0 && softblock_cnt == 0)
        {
            PANIC();
        }
    }

    /*ready queue empty*/
    else
    {   
        debug(1,3,5,7);
        curr_proc = removeProcQ(&ready_queue); 
        setTIMER(LDIT(5000)); /*local Timer (PLT)*/
        STCK(quantum_start_time);  /*start quantum of current process*/
        LDST(&curr_proc->p_s);
    }
}

void switchContext(pcb_PTR to_be_executed)
{
    /*load time*/
    cpu_t quantum_end_time;
    STCK(quantum_end_time);
    curr_proc->p_time += (quantum_end_time - quantum_start_time);

    /*load state of process to be executed, return processor to whatever interrupt state and mode was in 
    effect when the exception occured*/
    LDST(&to_be_executed->p_s); /*put saved state in next_proc's state*/
}