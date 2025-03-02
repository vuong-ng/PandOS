#include "../h/exceptions.h"
#include "../h/initial.h"
#include "../h/scheduler.h"
#include "../h/interrupts.h"
#include "../h/pcb.h"
#include "../h/asl.h"

static fooBarCount = 0;
void fooBar()
{
    /*STST(&curr_proc->p_s);   (done by hardware, dont have to do)*/ /*store curr proc's state in BIOS Data Page right after exception raised (in 0x0FFFF000 ?)*/
    /*setCAUSE(curr_proc->p_s.s_cause); */  /*set Cause register*/
    fooBarCount++;

    unsigned int cause = ((state_t*) BIOSDATAPAGE)->s_cause; 
    unsigned int cause_original = ((state_t*) BIOSDATAPAGE)->s_cause;

    /*debug(fooBarCount, cause, 20,20);*/
    
    /*4-way branch, all traps get same handler*/
    switch ((cause >> 2) & 0b00000000000000000000000000011111)
    {
    /*Interrupt*/
    case 0:
    {
        /*while there is an unhandled interrupt, call interrupt handler*/
        /*if there's unresolved interrupt, fooBar will be called again*/
        interruptHandler(cause_original);

        /*return back to curr_proc*/
        break;
    }

    /*TLB exceptions*/
    case 1:
    case 2:
    case 3:
    {
        passUp(PGFAULTEXCEPT);
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
        /*debug(process_cnt,5,5,5);*/
        trapHandler();
        break; 
    }
        

    /*syscalls*/
    case 8:
    {
        syscallHandler();
        break;
    }
        
    
    default:
        break;
    }

    /*LDST();*/
}

void trapHandler()
{
    passUp(GENERALEXCEPT);
}

void syscallHandler()
{

    /*check user/kernel mode and call trap handler if necessary*/
    unsigned int status = 0; /*((state_t*) BIOSDATAPAGE)->s_status;*/
    status >>= 3; /*Status.KUp*/
    status &= 0x00000001; /*check mode, macro: */
    if (status == 1) /*in user-mode, trap into kernel*/
    {
        /*(((state_t*) BIOSDATAPAGE)->s_cause) &= 0b11111111111111111111111110000011;*/
        /*(((state_t*) BIOSDATAPAGE)->s_cause) |= 0b00000000000000000000000000101000;*/ /*set Cause.ExcCode to 10 - RI*/
        /*trapHandler();*/
    }
        
    int a0 = ((state_t*) BIOSDATAPAGE)->s_a0;
    cpu_t quantum_end_time;
    switch (a0)
    {
    /*Create Process (SYS1)*/
    case CREATETHREAD:
    {
        /*debug(123,123,123,123);*/
        /*initialize state of new process based on current process's state ( = processor state??) saved in a1 */
        state_t* a1 = ((state_t*) BIOSDATAPAGE)->s_a1;
        support_t* a2 = ((state_t*) BIOSDATAPAGE)->s_a2;
        pcb_PTR new_proc = allocPcb();
        /*debug(123,123,123,123);*/
        /* new_proc == NULL -> insufficient resources, put err code -1 in v0*/
        if (new_proc == NULL)
        {
            curr_proc->p_s.s_v0 = -1;
            increasePC();
            LDST((state_t*) BIOSDATAPAGE);
        }
        else 
            curr_proc->p_s.s_v0 = 0;
        /*debug(123,123,123,123);*/

        /*step 1: p_s from a1*/
        copyState(&(new_proc->p_s), a1);
        /*debug(123,123,123,123);*/
        
        /*step 2: p_supportStruct from a2*/
        if(a2 != NULL)
        {
            support_t new_sptr;
            new_proc->p_supportStruct = &new_sptr;
            copySupport(new_proc->p_supportStruct, a2);
        }
            

        /*debug(123,123,ready_queue,curr_proc);*/
        /*step 3: place newProc on readyQueue */
        insertProcQ(&ready_queue, new_proc);
        process_cnt++; /* increment process count*/
        /*debug(curr_proc,123,ready_queue,123);*/
        /*step 4:make it a child of current process*/
        insertChild(curr_proc, new_proc);

        /*step 5: p_time set to zero*/
        new_proc->p_time = (cpu_t) 0;

        /*step 6: p_semAdd to NULL*/
        new_proc->p_semAdd = NULL;
        
        /*for non-blocking syscalls, increasePC & LDST from BIOS Data Page*/
        increasePC();
        LDST((state_t*) BIOSDATAPAGE);
         /*ret control to curr proc (do nothing here)*/
        break;
    }
        

    /*Terminate Process (SYS2)*/
    case TERMINATETHREAD:
    {
        /*debug(222,ready_queue, ready_queue->p_next, curr_proc);*/
        /*step 1: remove the process and all its descendants*/
        terminateProc(curr_proc->p_child);
        outChild(curr_proc);

        outProcQ(&ready_queue, curr_proc);

        /*debug(ready_queue, curr_proc,ready_queue->p_next,222);*/
        
        
        /*step 2: if terminated_proc blocked on sem, increment its sem
                  however if blocked on DEVICE sem, dont adjust sem ? (is device_sem an asl)*/
        /*blocked */
        if (curr_proc->p_semAdd != NULL)
        {
            
            /*sem is not device sem*/
            if(!(isDeviceSem(curr_proc->p_semAdd)))
                (*curr_proc->p_semAdd)++;
            softblock_cnt--;
        }
        
        freePcb(curr_proc);

        /*step 3: adjust proc count and soft-blocked cnt*/
        process_cnt--;
        /*debug(curr_proc,52,process_cnt,softblock_cnt);*/
        /*curr_proc terminated, call scheduler to schedule new pcb*/        
        scheduler();
        break;
    }
        
    /*Passeren (P) (SYS3)*/
    case PASSERN:
    /*P: decrease value of semaphore, if < 0, block the process (sleep)*/
    {
        
        /*copy processor state into curr proc state*/
        copyState(&(curr_proc->p_s), (state_t*) BIOSDATAPAGE);

        int* semAdd = ((state_t*) BIOSDATAPAGE)->s_a1;
        (*semAdd)--;
        /*if sem >= 0 -> running (not blocked), return control to current process*/
        if (*(semAdd) >= 0)
        {
            /*debug(333,333,333,333);*/
            /*insertProcQ(&ready_queue, removeBlocked(semAdd));*/ /*safety measures*/
            
            increasePC();
            /*free sem back to semdFree list*/
            /**/
            
            LDST((state_t*) BIOSDATAPAGE);
        }
            

        /*else: process blocked on ASL, call Scheduler*/
        else
        {
            
            increasePC();   
            copyState(&(curr_proc->p_s), (state_t*) BIOSDATAPAGE);
            
            /*(blocking) Update the accumulated CPU time for the Current Process*/
            STCK(quantum_end_time);
            curr_proc->p_time += (quantum_end_time - quantum_start_time);
            
            /*debug(9998, process_cnt, softblock_cnt, curr_proc);*/
            /*debug(9998, semAdd, curr_proc,9998);*/
            if(insertBlocked(semAdd, curr_proc) == FALSE)  /*blocked successfully*/
            {
                /*debug(ready_queue, curr_proc, 9998,9998);*/
                softblock_cnt++;
                scheduler();
                
            }
            else /*can't block, return to curr*/
                /*LDST((state_t*) BIOSDATAPAGE);*/
            {
                /*debug(9998,0,9998,0);*/
                PANIC();
            }
                
            /*debug(39,40,41,42);*/
            scheduler();
        }
        break;
    }
        
    /*Verhogen (V) (SYS4)*/
    case VERHOGEN:
    /*V: increase value of semaphore, if there's sleeping process (semaphore <= 0), wake it up*/
    {
        /*debug(process_cnt,((state_t*) BIOSDATAPAGE)->s_a2,4,4);*/

        int* semAdd = ((state_t*) BIOSDATAPAGE)->s_a1;

        (*semAdd)++;
        
        if (*(semAdd) <= 0)
        {
            
            pcb_PTR removed = removeBlocked(semAdd);
            /*debug(9999,9999,removed,softblock_cnt);*/
            insertProcQ(&ready_queue, removed);
            if(removed != NULL)
                softblock_cnt--;
        }
            
    
        /*return control to current process*/
        increasePC();
        LDST((state_t*) BIOSDATAPAGE);
        break;
    }
        
    /*Wait for IO Device (SYS5)*/
    case WAITIO:
    
    {
        /*copy processor state into curr proc state*/
        copyState(&(curr_proc->p_s), (state_t*) BIOSDATAPAGE);
        

        int interrupt_line = curr_proc->p_s.s_a1;
        int device_number = curr_proc->p_s.s_a2;
        int terminal_read = curr_proc->p_s.s_a3;
        int* device_semAdd;

        /*not terminal (HANDLED LATER)*/
        if(interrupt_line != TERMINT)
        {
            device_semAdd = &(device_sem[(interrupt_line - 3) * DEVPERINT + device_number]);
        }
        else
        {
            device_semAdd = &(device_sem[(interrupt_line - 3) * DEVPERINT + device_number * 2 + terminal_read]);
        }
            
        /*perform P on device semaphore*/
        (*device_semAdd)--;
        /*(good practice) check if *device_semAdd < 0*/
        
        
        /*block process*/
        
        increasePC();
        
        copyState(&(curr_proc->p_s), (state_t*) BIOSDATAPAGE);
        
         /*(blocking) Update the accumulated CPU time for the Current Process*/
        STCK(quantum_end_time);
        curr_proc->p_time += (quantum_end_time - quantum_start_time);

        /*(always) block the Current Process on the ASL, call scheduler*/
        if(!insertBlocked(device_semAdd, curr_proc))
            softblock_cnt++;
        else
            PANIC();
        /*debug(terminal_read,process_cnt,softblock_cnt,5);*/
        scheduler();

        break;
    }
        
    /*Get CPU Time (SYS6)*/
    case GETCPUTIME:
    {
        /*step 1: put current proc's p_time in v0 of saved exception state + used quantum*/
        STCK(quantum_end_time);
        ((state_t*) BIOSDATAPAGE)->s_v0 = curr_proc->p_time + (quantum_end_time - quantum_start_time); 

        /*return control to current process*/
        increasePC();
        LDST((state_t*) BIOSDATAPAGE);
        break;
    }
        
    /*Wait For Clock (SYS7)*/
    case WAITCLOCK:
    {
        /*copy processor state into curr proc state*/
        /*copyState(&(curr_proc->p_s), (state_t*) BIOSDATAPAGE);*/

        /*step 1: performs P on pseudo-clock sem*/

        int* pseudo_clk_semAdd = &device_sem[PSEUDOCLK];
        (*pseudo_clk_semAdd)--;


        /*step 3: block current process in ASL, then call Scheduler*/

        increasePC();
        copyState(&(curr_proc->p_s), (state_t*) BIOSDATAPAGE);

        STCK(quantum_end_time);
        curr_proc->p_time += (quantum_end_time - quantum_start_time);
        if(insertBlocked(pseudo_clk_semAdd, curr_proc) == FALSE)
        {
            softblock_cnt++;
            scheduler();
        }
        else 
            PANIC();
        
        
        
        /*step 2: performs V (increase) every 100 millisecond on pseudo-clock sem*/

        break;
    }
        
    /*Get SUPPORT Data (SYS8)*/
    case GETSPTPTR:
    {
        /*step 1: return p_supportStruct if exists, otherwise NULL*/
        ((state_t*) BIOSDATAPAGE)->s_a2 = curr_proc->p_supportStruct;       /*or return in v0*/

        /*return control to current process*/
        increasePC();
        LDST((state_t*) BIOSDATAPAGE);
        break;
    }
        
    default:
    {
        increasePC();
        passUp(GENERALEXCEPT);
        break;
    }
        
    }
}


void passUp(int passup_type)
{
    /*debug(9997,9997,9997,softblock_cnt);*/
    if(curr_proc->p_supportStruct == NULL)
    {

        /*step 1: remove the process and all its descendants*/
        terminateProc(curr_proc->p_child);
        outChild(curr_proc);

        /*step 2: if terminated_proc blocked on sem, increment its sem
                  however if blocked on DEVICE sem, dont adjust sem ? (is device_sem an asl)*/
        /*blocked */
        if (curr_proc->p_semAdd != NULL)
        {
            /*sem is not device sem*/
            if(!(isDeviceSem(curr_proc->p_semAdd)))
                (*curr_proc->p_semAdd)++;
            softblock_cnt--;
        }
        freePcb(curr_proc);

        /*step 3: adjust proc count and soft-blocked cnt*/
        process_cnt--;

        /*curr_proc terminated, call scheduler to schedule new pcb*/        
        scheduler();
    }   
        
    else
    {
        /*debug(15,17,19,21);*/
        /*copy saved exception state from BIOS Data Page to correct sup_exceptState of curr_proc*/
        state_t* sup_exceptState_p = &(curr_proc->p_supportStruct->sup_exceptState[passup_type]);


        copyState(sup_exceptState_p, (state_t*) BIOSDATAPAGE);

        /*perform LDCXT using field from correct sup_exceptContext field of curr_proc*/
        context_t* sup_exceptContext_p = &(curr_proc->p_supportStruct->sup_exceptContext[passup_type]);
        LDCXT(sup_exceptContext_p->c_stackPtr, sup_exceptContext_p->c_status, sup_exceptContext_p->c_pc);
    }
}




/*HELPER FUNCTIONS*/
void increasePC()  
{
    ((state_t*) BIOSDATAPAGE)->s_pc += 4;
}
void updateTime(pcb_PTR proc)
{

}
int isDeviceSem(int* semAdd)
{
    int i, found = FALSE;
    for (i = 0; i < 49; i++)
        found |= (&device_sem[i] == semAdd);
    return found;
}

/*copy src to dest*/
void copyState(state_t* dest, state_t* src)
{
    dest->s_entryHI = src->s_entryHI;
    dest->s_cause   = src->s_cause;
    dest->s_status  = src->s_status;
    dest->s_pc      = src->s_pc;
    int i;
    debug(1,1,1,1);

    for(i = 0; i < 22; i++)
        dest->s_reg[i] = src->s_reg[i];

    dest->s_reg[22] = src->s_reg[22];   /*something fishy going on...*/

    for (i = 23; i < STATEREGNUM; i++)
    {
        dest->s_reg[i] = src->s_reg[i];
    }
        
}

void copySupport(support_t* dest, support_t* src)
{
    /*debug(357,357,357,357);
    debug(dest,src,357,357);
    debug(dest->sup_asid, src->sup_asid, 357, 357);*/
    dest->sup_asid = src->sup_asid;
    copyState(&(dest->sup_exceptState[0]), &(src->sup_exceptState[0]));
    copyState(&(dest->sup_exceptState[1]), &(src->sup_exceptState[1]));

    dest->sup_exceptContext[0].c_pc = src->sup_exceptContext[0].c_pc;
    dest->sup_exceptContext[0].c_stackPtr = src->sup_exceptContext[0].c_stackPtr;
    dest->sup_exceptContext[0].c_status = src->sup_exceptContext[0].c_status;

    dest->sup_exceptContext[1].c_pc = src->sup_exceptContext[1].c_pc;
    dest->sup_exceptContext[1].c_stackPtr = src->sup_exceptContext[1].c_stackPtr;
    dest->sup_exceptContext[1].c_status = src->sup_exceptContext[1].c_status;
}

/*sys3, sys5, sys7 (blocking syscalls) function: 
- increase PC by 4
- load saved processor state into current proc's pcb
- update accumulated CPU time for current proc
- block current proc on ASL, call insertBlocked
- call Scheduler*/
 
void blocking_syscalls(pcb_PTR curr_proc)
{


}



/*remove (outChild) proc's sibs and all its descendants*/
/*three cases: curr_proc, on ready_queue, blocked*/
void terminateProc(pcb_PTR proc)
{
    if (proc == NULL)
        return;

    /*blocked */
    if (proc->p_semAdd != NULL)
    {
        /*sem is not device sem*/
        if(!(isDeviceSem(proc->p_semAdd)))
            *(proc->p_semAdd)++;
        softblock_cnt--;
    }
    process_cnt--;

    terminateProc(proc->p_sib);
    terminateProc(proc->p_child);

    outChild(proc);
    outProcQ(&ready_queue, proc);
    freePcb(proc);
    return;
}


