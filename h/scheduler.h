#ifndef SCHEDULER
#define SCHEDULER

#include "../h/initial.h"

void scheduler()
{
    /*Remove the pcb from the head of the Ready Queue and store the pointer to the pcb in the Current Process field*/
    curr_proc = removeProcQ(&ready_queue);  /*increase process_cnt: NO*/


    setTIMER(LDIT(5000)); /*local Timer (PLT)*/

    LDST(&curr_proc->p_s);

    /*REDO ORDER OF CHECKING*/

    /*If the Process Count is zero invoke the HALT BIOS service/instruction.*/
    if (process_cnt == 0)
        HALT();

    /*If the Process Count > 0 and the Soft-block Count > 0 enter a Wait State*/
    if (process_cnt > 0 && softblock_cnt > 0)
    {
        setSTATUS(getSTATUS() | 0b00000000000000000000000000000001); /*macro: INTRPTENABLED*/    /*enable interrupt*/ /*use hex*/
        setSTATUS(getSTATUS() & 0b11110111111111111111111111111111); /*macro: PLTDISABLED*/   /*disable PLT*/
        WAIT();
    }

    /*Deadlock for Pandos is defined as when the Process Count > 0 and the Soft-block Count is zero.*/
    if (process_cnt > 0 && softblock_cnt == 0)
        PANIC();

}
#endif