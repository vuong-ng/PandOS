#include "../h/exceptions.h"


void fooBar()
{
    /*STST(&curr_proc->p_s);   (done by hardware, dont have to do)*/ /*store curr proc's state in BIOS Data Page right after exception raised (in 0x0FFFF000 ?)*/
    /*setCAUSE(curr_proc->p_s.s_cause); */  /*set Cause register*/
    
    unsigned int cause = processor_0_exception_state->s_cause; 

    /*getCause gets real register (almost never needed)*/

    
    /*4-way branch is good, all traps get same handler...*/
    switch ((cause >> 2) & 0b00000000000000000000000000001111)
    {
    /*Interrupt*/
    case 0:
    {
        /*while there is an unhandled interrupt, call interrupt handler*/
        /*if there's unresolved interrupt, fooBar will be called again*/
        interruptHandler(&cause); 

        /*return back to curr_proc*/
        break;
    }

    /*TLB exceptions*/
    case 1:
    case 2:
    case 3:
    {
        passUp(PGFAULTEXCEPT);
        uTLB_RefillHandler();
        break;
    }
        

    /*program traps*/
    case 4:
    case 5:
    case 6:
    case 7:
    case 9:
    case 10:
    case 11:
    case 12:
    {
        passUp(GENERALEXCEPT);
        break; 
    }
        

    /*syscalls*/
    case 8:
        syscall_handler(curr_proc);
        break;
    
    default:
        break;
    }

    /*LDST(&curr_proc->p_s);*/
}

void uTLB_RefillHandler () 
{
    setENTRYHI(0x80000000);
    setENTRYLO(0x00000000);
    TLBWR();
    LDST ((state_PTR) 0x0FFFF000);
}

void syscallHandler(pcb_PTR curr_proc)
{
    int a0 = curr_proc->p_s.s_a0;

    /*check user/kernel mode and call trap handler if necessary*/
    int status = curr_proc->p_s.s_status;
    status >>= 1;
    status &= 0b00000000000000000000000000000001; /*check mode, macro: */
    if (status == 1) /*in user-mode, trap into kernel*/
    {
        (processor_0_exception_state->s_cause) &= 0b11111111111111111111111110000011;
        (processor_0_exception_state->s_cause) |= 0b00000000000000000000000000101000; /*set Cause.ExcCode to 10 - RI*/
        trapHandler();
    }
        

    switch (a0)
    {
    /*Create Process (SYS1)*/
    case 1:
    {
        /*initialize state of new process baseed on current process's state ( = processor state??) saved in a1 */
        state_t* a1 = curr_proc->p_s.s_a1;
        support_t* a2 = curr_proc->p_s.s_a2;
        pcb_PTR new_proc = allocPcb();

        /* new_proc == NULL -> insufficient resources, put err code -1 in v0*/
        if (new_proc == NULL)
        {
            new_proc->p_s.s_v0 = -1;
            return;
        }
        else 
            new_proc->p_s.s_v0 = 0;


        /*step 1: p_s from a1*/
        new_proc->p_s.s_entryHI = a1->s_entryHI;
        new_proc->p_s.s_cause = a1->s_cause;
        new_proc->p_s.s_status = a1->s_status;
        new_proc->p_s.s_pc = a1->s_pc;
        int i;
        for (i = 0; i < STATEREGNUM; i++)
            new_proc->p_s.s_reg[i] = a1->s_reg[i];


        /*step 2: p_supportStruct from a2*/
        new_proc->p_supportStruct = a2;
        /*(todo): initialize all support_struct fields*/


        /*step 3: place newProc on readyQueue */
        insertProcQ(&ready_queue, new_proc);
        process_cnt++; /* increment process count*/

        /*step 4:make it a child of current process*/
        insertChild(curr_proc, new_proc);

        /*step 5: p_time set to zero*/
        new_proc->p_time = 0;

        /*step 6: p_semAdd to NULL*/
        new_proc->p_semAdd = NULL;
        
         /*ret control to curr proc (do nothing here)*/
        break;
    }
        

    /*Terminate Process (SYS2)*/
    case 2:
    {
        /*step 1: remove the process and all its descendants*/
        terminateProc(curr_proc->p_child);
        pcb_PTR terminated_proc = outChild(curr_proc);

        /*step 2: if terminated_proc blocked on sem, increment its sem
                  however if blocked on DEVICE sem, dont adjust sem ? (is device_sem an asl)*/
        if (terminated_proc->p_semAdd != NULL)
        {
            /*check if terminated_proc is blocked on a device sem*/

            /*NOT device sem*/
            *(terminated_proc->p_semAdd) += 1;
        }

        /*step 3: adjust proc count and soft-blocked cnt*/
        process_cnt--;
        softblock_cnt--;

        /*curr_proc terminated, call scheduler to schedule new pcb*/
        scheduler();
        break;
    }
        
    /*Passeren (P) (SYS3)*/
    case 3:
    /*P: decrease value of semaphore, if < 0, block the process (sleep)*/
    {
        /*copy processor state into curr proc state*/
        curr_proc->p_s.s_entryHI = processor_0_exception_state->s_entryHI;
        curr_proc->p_s.s_cause = processor_0_exception_state->s_cause;
        curr_proc->p_s.s_status = processor_0_exception_state->s_status;
        curr_proc->p_s.s_pc = processor_0_exception_state->s_pc;
        int i;
        for (i = 0; i < STATEREGNUM; i++)
            curr_proc->p_s.s_reg[i] = processor_0_exception_state->s_reg[i];

        /*Update the accumulated CPU time for the Current Process*/



        int* semAdd = curr_proc->p_s.s_a1;
        *(semAdd) -= 1;
        /*if sem >= 0 -> running (not blocked), return control to current process*/
        if (*(semAdd) >= 0)
            return;

        /*else: process blocked on ASL, call Scheduler*/
        else
        {
            int insert_successful = insertBlocked(semAdd, curr_proc);
            scheduler();
        }
        break;
    }
        
    /*Verhogen (V) (SYS4)*/
    case 4:
    /*V: increase value of semaphore, if there's sleeping process, wake it up*/
    {
        int* semAdd = curr_proc->p_s.s_a1;
        *(semAdd) += 1;
        pcb_PTR removed = removeBlocked(semAdd);

        return;
        break;
    }
        
    /*Wait for IO Device (SYS5)*/
    case 5:
    /*
    Given an interrupt line (IntLineNo) and a device number (DevNo) one can
    compute the starting address of the device’s device register:
    devAddrBase = 0x1000.0054 + ((IntlineNo - 3) * 0x80) + (DevNo * 0x10)
    */
    {
        /*copy processor state into curr proc state*/
        curr_proc->p_s.s_entryHI = processor_0_exception_state->s_entryHI;
        curr_proc->p_s.s_cause = processor_0_exception_state->s_cause;
        curr_proc->p_s.s_status = processor_0_exception_state->s_status;
        curr_proc->p_s.s_pc = processor_0_exception_state->s_pc;
        int i;
        for (i = 0; i < STATEREGNUM; i++)
            curr_proc->p_s.s_reg[i] = processor_0_exception_state->s_reg[i];
        
        /*Update the accumulated CPU time for the Current Process*/



        int interrupt_line = curr_proc->p_s.s_a1;
        int device_number = curr_proc->p_s.s_a2;
        int terminal_read = curr_proc->p_s.s_a3;
        int* device_semAdd;

        /*not terminal (HANDLED LATER)*/
        if(interrupt_line != 7)
            device_semAdd = device_sem[(interrupt_line - 3) * 8 + device_number];
        else
            device_semAdd = device_sem[(interrupt_line - 3) * 8 + device_number * 2 + terminal_read];

        /*perform P on device semaphore*/
        *device_semAdd -= 1;
         /*(always) block the Current Process on the ASL, call scheduler*/
        int insert_successful = insertBlocked(device_semAdd, curr_proc);
        scheduler();

        break;
    }
        
    /*Get CPU Time (SYS6)*/
    case 6:
    {
        /*step 1: put current proc's p_time in v0 (?)*/
        cpu_t quantum_end_time;
        STCK(quantum_end_time);
        curr_proc->p_s.s_v0 = curr_proc->p_time + (quantum_end_time - quantum_start_time);  /* + cputime during current quantum (using TOD Clock)*/

        break;
    }
        
    /*Wait For Clock (SYS7)*/
    case 7:
    {
        /*copy processor state into curr proc state*/
        curr_proc->p_s.s_entryHI = processor_0_exception_state->s_entryHI;
        curr_proc->p_s.s_cause = processor_0_exception_state->s_cause;
        curr_proc->p_s.s_status = processor_0_exception_state->s_status;
        curr_proc->p_s.s_pc = processor_0_exception_state->s_pc;
        int i;
        for (i = 0; i < STATEREGNUM; i++)
            curr_proc->p_s.s_reg[i] = processor_0_exception_state->s_reg[i];

        /*Update the accumulated CPU time for the Current Process*/


        /*step 1: performs P on pseudo-clock sem*/

        int* pseudo_clk_semAdd = device_sem[48];
        *(pseudo_clk_semAdd) -= 1;


        /*step 3: block current process in ASL, then call Scheduler*/

        int insert_successful = insertBlocked(pseudo_clk_semAdd, curr_proc);
        scheduler();
        /*step 2: performs V (increase) every 100 millisecond on pseudo-clock sem*/

        break;
    }
        
    /*Get SUPPORT Data (SYS8)*/
    case 8:
    {
        /*step 1: return p_supportStruct if exists, otherwise NULL*/
        return curr_proc->p_supportStruct;

        break;
    }
        
    default:
    {
        passUp(GENERALEXCEPT);
        break;
    }
        
    }

    increasePC(curr_proc);


}


void passUp(int passup_type)
{
   if(curr_proc->p_supportStruct == NULL)
    {
        /*(todo) sys2 handling*/
    }   
        
    else
    {
        /*copy saved exception state from BIOS Data Page to correct sup_exceptState of curr_proc*/
        state_t* sup_exceptState_p = &(curr_proc->p_supportStruct->sup_exceptState[passup_type]);
        sup_exceptState_p->s_entryHI = processor_0_exception_state->s_entryHI;
        sup_exceptState_p->s_cause = processor_0_exception_state->s_cause;
        sup_exceptState_p->s_status = processor_0_exception_state->s_status;
        sup_exceptState_p->s_pc = processor_0_exception_state->s_pc;
        int i;
        for (i = 0; i < STATEREGNUM; i++)
            sup_exceptState_p->s_reg[i] = processor_0_exception_state->s_reg[i];

        /*perform LDCXT using field from correct sup_exceptContext field of curr_proc*/
        context_t* sup_exceptContext_p = &(curr_proc->p_supportStruct->sup_exceptContext[passup_type]);
        LDCXT(sup_exceptContext_p->c_stackPtr, sup_exceptContext_p->c_status, sup_exceptContext_p->c_pc);
    }
}




/*HELPER FUNCTIONS*/
void increasePC(pcb_PTR curr_proc)
{
    curr_proc->p_s.s_pc += 4;

}

/*sys3, sys5, sys7 (blocking syscalls) function: 
- increase PC by 4
- load saved processor state into current proc's pcb
- update accumulated CPU time for current proc
- block current proc on ASL, call insertBlocked
- call Scheduler*/
 
void blocking_syscalls(pcb_PTR curr_proc)
{
increasePC(curr_proc);
}



/*remove proc's sibs and all its descendants*/
void terminateProc(pcb_PTR proc)
{
    if (proc == NULL)
        return;
    pcb_PTR sib = proc->p_sib;
    if(sib != NULL)
        terminateProc(sib);
    terminateProc(proc->p_child);
    pcb_PTR terminated = outChild(proc);
    freePcb(terminated);
    return;
}


