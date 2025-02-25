#include "../h/exceptions.h"


void fooBar()
{
    /*STST(&curr_proc->p_s);   (done by hardware, dont have to do)*/ /*store curr proc's state in BIOS Data Page right after exception raised (in 0x0FFFF000 ?)*/
    /*setCAUSE(curr_proc->p_s.s_cause); */  /*set Cause register*/
    
    unsigned int cause = CP0_exception_s->s_cause; 

    /*getCause gets real register (almost never needed)*/
    
    /*4-way branch, all traps get same handler*/
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
        trapHandler();
        break; 
    }
        

    /*syscalls*/
    case 8:
        syscall_handler();
        break;
    
    default:
        break;
    }

    /*LDST();*/
}

void uTLB_RefillHandler () 
{
    setENTRYHI(0x80000000);
    setENTRYLO(0x00000000);
    TLBWR();
    LDST ((state_PTR) 0x0FFFF000);
}

void trapHandler()
{
    passUp(GENERALEXCEPT);
}

void syscallHandler()
{
    /*check user/kernel mode and call trap handler if necessary*/
    int status = curr_proc->p_s.s_status;
    status >>= 1;
    status &= 0x00000001; /*check mode, macro: */
    if (status == 1) /*in user-mode, trap into kernel*/
    {
        (CP0_exception_s->s_cause) &= 0b11111111111111111111111110000011;
        (CP0_exception_s->s_cause) |= 0b00000000000000000000000000101000; /*set Cause.ExcCode to 10 - RI*/
        trapHandler();
    }
        
    int a0 = curr_proc->p_s.s_a0;
    switch (a0)
    {
    /*Create Process (SYS1)*/
    case CREATETHREAD:
    {
        /*initialize state of new process based on current process's state ( = processor state??) saved in a1 */
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
        copyState(&(new_proc->p_s), a1);


        /*step 2: p_supportStruct from a2*/
        new_proc->p_supportStruct = a2;
        /*(todo): initialize all support_struct fields*/


        /*step 3: place newProc on readyQueue */
        insertProcQ(&ready_queue, new_proc);
        process_cnt++; /* increment process count*/

        /*step 4:make it a child of current process*/
        insertChild(curr_proc, new_proc);

        /*step 5: p_time set to zero*/
        new_proc->p_time = (cpu_t) 0;

        /*step 6: p_semAdd to NULL*/
        new_proc->p_semAdd = NULL;
        
         /*ret control to curr proc (do nothing here)*/
        break;
    }
        

    /*Terminate Process (SYS2)*/
    case TERMINATETHREAD:
    {
        /*step 1: remove the process and all its descendants*/
        terminateProc(curr_proc->p_child);
        pcb_PTR terminated_proc = outChild(curr_proc);

        /*step 2: if terminated_proc blocked on sem, increment its sem
                  however if blocked on DEVICE sem, dont adjust sem ? (is device_sem an asl)*/
        if (terminated_proc->p_semAdd != NULL)
        {
            /*(todo) check if terminated_proc is blocked on a device sem*/

            /*NOT device sem*/
            (*(terminated_proc->p_semAdd))++;
        }

        /*step 3: adjust proc count and soft-blocked cnt*/
        process_cnt--;
        softblock_cnt--;

        /*curr_proc terminated, call scheduler to schedule new pcb*/
        scheduler();
        break;
    }
        
    /*Passeren (P) (SYS3)*/
    case PASSERN:
    /*P: decrease value of semaphore, if < 0, block the process (sleep)*/
    {
        /*copy processor state into curr proc state*/
        copyState(&(curr_proc->p_s), CP0_exception_s);

        /*Update the accumulated CPU time for the Current Process*/



        int* semAdd = curr_proc->p_s.s_a1;
        (*semAdd)--;
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
    case VERHOGEN:
    /*V: increase value of semaphore, if there's sleeping process (semaphore <= 0), wake it up*/
    {
        int* semAdd = curr_proc->p_s.s_a1;
        (*semAdd)++;
        pcb_PTR removed;
        if (*(semAdd) <= 0)
            removed = removeBlocked(semAdd);
        break;
    }
        
    /*Wait for IO Device (SYS5)*/
    case WAITIO:
    
    {
        /*copy processor state into curr proc state*/
        copyState(&(curr_proc->p_s), CP0_exception_s);
        
        /*(todo) Update the accumulated CPU time for the Current Process*/



        int interrupt_line = curr_proc->p_s.s_a1;
        int device_number = curr_proc->p_s.s_a2;
        int terminal_read = curr_proc->p_s.s_a3;
        int* device_semAdd;

        /*not terminal (HANDLED LATER)*/
        if(interrupt_line != TERMINT)
            device_semAdd = device_sem[(interrupt_line - 3) * DEVPERINT + device_number];
        else
            device_semAdd = device_sem[(interrupt_line - 3) * DEVPERINT + device_number * 2 + terminal_read];

        /*perform P on device semaphore*/
        (*device_semAdd)--;
        /*(good practice) check if *device_semAdd < 0*/

         /*(always) block the Current Process on the ASL, call scheduler*/
        int insert_successful = insertBlocked(device_semAdd, curr_proc);
        scheduler();

        break;
    }
        
    /*Get CPU Time (SYS6)*/
    case GETCPUTIME:
    {
        /*step 1: put current proc's p_time in v0 + used quantum*/
        cpu_t quantum_end_time;
        STCK(quantum_end_time);
        curr_proc->p_s.s_v0 = curr_proc->p_time + (quantum_end_time - quantum_start_time); 

        break;
    }
        
    /*Wait For Clock (SYS7)*/
    case WAITCLOCK:
    {
        /*copy processor state into curr proc state*/
        copyState(&(curr_proc->p_s), CP0_exception_s);

        /*(todo) Update the accumulated CPU time for the Current Process*/


        /*step 1: performs P on pseudo-clock sem*/

        int* pseudo_clk_semAdd = device_sem[PSEUDOCLK];
        (*pseudo_clk_semAdd)--;


        /*step 3: block current process in ASL, then call Scheduler*/

        int insert_successful = insertBlocked(pseudo_clk_semAdd, curr_proc);
        scheduler();
        /*step 2: performs V (increase) every 100 millisecond on pseudo-clock sem*/

        break;
    }
        
    /*Get SUPPORT Data (SYS8)*/
    case GETSPTPTR:
    {
        /*step 1: return p_supportStruct if exists, otherwise NULL*/
        curr_proc->p_s.s_a2 = curr_proc->p_supportStruct;
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
        copyState(sup_exceptState_p, CP0_exception_s);

        /*perform LDCXT using field from correct sup_exceptContext field of curr_proc*/
        context_t* sup_exceptContext_p = &(curr_proc->p_supportStruct->sup_exceptContext[passup_type]);
        LDCXT(sup_exceptContext_p->c_stackPtr, sup_exceptContext_p->c_status, sup_exceptContext_p->c_pc);
    }
}




/*HELPER FUNCTIONS*/
void increasePC(pcb_PTR curr_proc)  {curr_proc->p_s.s_pc += 4;}
void updateTime(pcb_PTR proc)
{

}

/*copy src to dest*/
void copyState(state_t* dest, state_t* src)
{
    dest->s_entryHI = src->s_entryHI;
    dest->s_cause   = src->s_cause;
    dest->s_status  = src->s_status;
    dest->s_pc      = src->s_pc;
    int i;
    for (i = 0; i < STATEREGNUM; i++)
        dest->s_reg[i] = src->s_reg[i];
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



/*remove (outChild) proc's sibs and all its descendants*/
/*three cases: curr_proc, on ready_queue, blocked*/
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


