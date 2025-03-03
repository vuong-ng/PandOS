#include "../h/exceptions.h"
#include "../h/initial.h"
#include "../h/scheduler.h"
#include "../h/interrupts.h"
#include "../h/pcb.h"
#include "../h/asl.h"

void generalExceptionHandler()
{
    unsigned int cause = ((state_t*) BIOSDATAPAGE)->s_cause; 
    switch ((cause >> GETEXCCODE) & CLEAR26MSB)
    {
    /*Interrupt*/
    case 0:
    {
        interruptHandler(cause);
        break;
    }

    /*TLB exceptions*/
    case 1:
    case 2:
    case 3:
    {
        passUpOrDie(PGFAULTEXCEPT);
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
    {
        syscallHandler();
        break;
    }
        
    default:
        break;
    }
}

void trapHandler()
{
    passUpOrDie(GENERALEXCEPT);
}

void syscallHandler()
{
    /*check user/kernel mode and call trap handler if necessary*/
    unsigned int status = ((state_t*) BIOSDATAPAGE)->s_status;
    status = (status >> GETKUP) & CLEAR31MSB;
    if (status == 1) /*in user-mode, trap into kernel*/
    {
        (((state_t*) BIOSDATAPAGE)->s_cause) &= 0b11111111111111111111111110000011;
        (((state_t*) BIOSDATAPAGE)->s_cause) |= 0b00000000000000000000000000101000; /*set Cause.ExcCode to 10 - RI*/
        trapHandler();
    }
        
    int a0 = ((state_t*) BIOSDATAPAGE)->s_a0;
    switch (a0)
    {
    /*Create Process (SYS1)*/
    case CREATETHREAD:
    {
        /*initialize state of new process based on current process's state saved in a1 */
        pcb_PTR new_proc = allocPcb();

        /* new_proc == NULL -> insufficient resources, put err code -1 in v0*/
        if (new_proc == NULL)
            curr_proc->p_s.s_v0 = -1;
        else 
        {
            curr_proc->p_s.s_v0 = 0;
            copyState(&(new_proc->p_s), ((state_t*) BIOSDATAPAGE)->s_a1); /*p_s from a1*/
            new_proc->p_supportStruct = ((state_t*) BIOSDATAPAGE)->s_a2;  /*p_supportStruct from a2*/
                
            insertProcQ(&ready_queue, new_proc);  /*place newProc on readyQueue*/
            insertChild(curr_proc, new_proc);  /*make new process a child of current process*/
            process_cnt++;                       

            new_proc->p_time = (cpu_t) 0;  /*p_time set to zero*/
            new_proc->p_semAdd = NULL;     /*p_semAdd to NULL*/
        }
        
        /*for non-blocking syscalls, increasePC & LDST from BIOS Data Page*/
        increasePC();
        LDST((state_t*) BIOSDATAPAGE);
        break;
    }

    /*Terminate Process (SYS2)*/
    case TERMINATETHREAD:
    {
        /*step 1: remove the process and all its descendants*/
        killDescendants(curr_proc->p_child);
        killProcess(curr_proc);
        /*curr_proc terminated, call scheduler to schedule new pcb*/        
        scheduler();
        break;
    }
        
    /*Passeren (P) (SYS3)*/
    case PASSERN:
    /*P: decrease value of semaphore, if < 0, block the process (sleep)*/
    {
        int* semAdd = ((state_t*) BIOSDATAPAGE)->s_a1;
        
        /*if sem >= 0 -> running (not blocked), return control to current process*/
        if ((*semAdd) - 1 >= 0)
        {
            (*semAdd)--;
            increasePC();   
            LDST((state_t*) BIOSDATAPAGE);
        }
            
        /*else: process blocked on ASL, call Scheduler*/
        else
            Passeren(semAdd);
        break;
    }
        
    /*Verhogen (V) (SYS4)*/
    case VERHOGEN:
    /*V: increase value of semaphore, if there's sleeping process (semaphore <= 0), wake it up*/
    {
        Verhogen(((state_t*) BIOSDATAPAGE)->s_a1);
            
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

        /*not terminal */
        if(interrupt_line != TERMINT)
            device_semAdd = &(device_sem[(interrupt_line - 3) * DEVPERINT + device_number]);
        else
            device_semAdd = &(device_sem[(interrupt_line - 3) * DEVPERINT + device_number * SUBDEVPERTERM + terminal_read]);
            
        /*perform P on device semaphore*/
        Passeren(device_semAdd);
        break;
    }
        
    /*Get CPU Time (SYS6)*/
    case GETCPUTIME:
    {
        /*step 1: put current proc's p_time in v0 of saved exception state + used quantum*/
        updateTime(curr_proc);
        ((state_t*) BIOSDATAPAGE)->s_v0 = curr_proc->p_time; 

        /*return control to current process*/
        increasePC();
        LDST((state_t*) BIOSDATAPAGE);
        break;
    }
        
    /*Wait For Clock (SYS7)*/
    case WAITCLOCK:
    {
        /*performs P on pseudo-clock semaphore*/
        Passeren(&device_sem[PSEUDOCLK]);
        break;
    }
        
    /*Get SUPPORT Data (SYS8)*/
    case GETSPTPTR:
    {
        /*step 1: return p_supportStruct if exists, otherwise NULL*/
        ((state_t*) BIOSDATAPAGE)->s_v0 = curr_proc->p_supportStruct;      

        /*return control to current process*/
        increasePC();
        LDST((state_t*) BIOSDATAPAGE);
        break;
    }
        
    default:
    {
        increasePC();
        passUpOrDie(GENERALEXCEPT);
        break;
    }
        
    }
}


void passUpOrDie(int passup_type)
{
    if(curr_proc->p_supportStruct == NULL)
    {
        killDescendants(curr_proc->p_child);
        killProcess(curr_proc);
        /*curr_proc terminated, call scheduler to schedule new pcb*/        
        scheduler();     
    }
        
    else
    {
        /*copy saved exception state from BIOS Data Page to correct sup_exceptState of curr_proc*/
        state_t* sup_exceptState_p = &(curr_proc->p_supportStruct->sup_exceptState[passup_type]);
        copyState(sup_exceptState_p, (state_t*) BIOSDATAPAGE);

        /*perform LDCXT using field from correct sup_exceptContext field of curr_proc*/
        context_t* sup_exceptContext_p = &(curr_proc->p_supportStruct->sup_exceptContext[passup_type]);
        LDCXT(sup_exceptContext_p->c_stackPtr, sup_exceptContext_p->c_status, sup_exceptContext_p->c_pc);
    }
}


void killProcess(pcb_PTR proc)
{
    /*if terminated_proc blocked on sem, increment its sem
                however if blocked on DEVICE sem, dont adjust sem*/
    /*blocked */
    if (proc->p_semAdd != NULL)
    {
        /*sem is not device sem*/
        if(!(isDeviceSem(proc->p_semAdd)))
        {
            (*proc->p_semAdd)++;
            outBlocked(proc);
            softblock_cnt--;
        }
    }
    else    
        outProcQ(&ready_queue, proc);

    outChild(proc);
    freePcb(proc);

    /*step 3: adjust proc count and soft-blocked cnt*/
    process_cnt--;
}

/*remove (outChild) proc's sibs and all its descendants*/
void killDescendants(pcb_PTR first_child)
{
    if (first_child == NULL)
        return;

    killDescendants(first_child->p_child);
    killDescendants(first_child->p_sib);

    killProcess(first_child);
    return;
}

void Passeren(int* semAdd)
{
    (*semAdd)--;

    increasePC();   
    copyState(&(curr_proc->p_s), (state_t*) BIOSDATAPAGE);
    
    /*(blocking) Update the accumulated CPU time for the Current Process*/
    updateTime(curr_proc);

    if(insertBlocked(semAdd, curr_proc) == FALSE)  /*blocked successfully*/
    {
        softblock_cnt++;
        scheduler();
    }
    else /*can't block due to lack of available semaphore*/
        PANIC();
}

pcb_PTR Verhogen(int* semAdd)
{
    pcb_PTR removed;
    (*semAdd)++;
    if ((*semAdd) <= 0)
    {
        removed = removeBlocked(semAdd);
        if(removed != NULL)
        {
            insertProcQ(&ready_queue, removed);
            softblock_cnt--;
        }
    }
    return removed;
}

/*HELPER FUNCTIONS*/
void increasePC()  
{
    ((state_t*) BIOSDATAPAGE)->s_pc += 4;
}

void updateTime(pcb_PTR proc)
{
    cpu_t time_stop;
    STCK(time_stop);
    proc->p_time += (time_stop - time_start);
}
int isDeviceSem(int* semAdd)
{
    int i, found = FALSE;
    for (i = 0; i < DEVSEMNO; i++)
        found |= (&device_sem[i] == semAdd);
    return found;
}

/*copy src state to dest state*/
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