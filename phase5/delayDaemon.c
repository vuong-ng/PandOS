#include "../h/delayDaemon.h"

#define DUMMYHEADVAL -1
#define DUMMYTAILVAL __INT_MAX__
#define DAEMONASID 0
#define MILLION	1000000

HIDDEN delayd_t* delayd_h = NULL;
HIDDEN delayd_t* delaydFree_h = NULL;
HIDDEN int ADL_sem = 1;


void insertADL(delayd_t* delayd)
{
    delayd_t* prev = delayd_h;
    delayd_t* curr = delayd_h->d_next;

    /*while not reached end of the ADL, and not in right order*/
    while (!(prev->d_wakeTime <= delayd->d_wakeTime))
    {
        prev = prev->d_next;
        curr = curr->d_next;
    }
    prev->d_next = delayd;
    delayd->d_next = curr;
}

delayd_t* allocateDelayd(support_t* sPtr, int wakeTime)
{
    /*take one from the free list*/
    delayd_t* freeDelayd = delaydFree_h;
    delaydFree_h = delaydFree_h->d_next;

    /*populate the delay descriptor*/
    cpu_t tod;
    STCK(tod);
    freeDelayd->d_wakeTime = tod + wakeTime * MILLION;
    freeDelayd->d_supStruct = sPtr;
    return freeDelayd;
}

void deallocateDelayd(delayd_t* delayd)
{
    delayd->d_next = delaydFree_h;
    delaydFree_h = delayd;
}

int ADLEmpty(delayd_t* delaydFree_h)
{
    return (delaydFree_h == NULL);
}

void delay(support_t* sPtr)
{
    int wakeTime = sPtr->sup_exceptState[GENERALEXCEPT].s_a1;

    /*Attempting to request a Delay for less than 0 seconds is an error and should result in the U-proc begin terminated*/
    if(wakeTime < 0)
        SYSCALL(TERMINATE, 0, 0, 0);
    
    SYSCALL(PASSERN, &ADL_sem, 0, 0);   /*obtain mutual exclusion over the ADL*/

    if (ADLEmpty(delaydFree_h)) /*no delay descriptor available*/
    {
        /*releasing mutual exclusion over the ADL and terminate*/
        SYSCALL(VERHOGEN, &ADL_sem, 0, 0);
        SYSCALL(TERMINATE, 0, 0, 0);
    }

    /*allocate 1 free delay descriptor from free list & populate the delay descriptor*/
    delayd_t* freeDelayd = allocateDelayd(sPtr, wakeTime);

    /*insert to ADL (sorted)*/
    insertADL(freeDelayd);

    disableInterrupts();
    SYSCALL(VERHOGEN, &ADL_sem, 0, 0);      /*release mutual exclusion over the ADL*/
    SYSCALL(PASSERN, &(sPtr->private_sem), 0, 0);   /*block the executing Uproc on its private semaphore*/
    enableInterrupts();

    
    /*return control (LDST) to the U-proc at the instruction immediately following the SYS18.*/
    LDST((state_t*) &(sPtr->sup_exceptState[GENERALEXCEPT]));

}

void delayDaemon()
{
    while(TRUE)
    {
        SYSCALL(WAITCLOCK, 0, 0, 0);    /* wait for 100 milliseconds*/
        SYSCALL(PASSERN, &ADL_sem, 0, 0);   /*gain mutual exclusion of the ADL*/

        delayd_t* prev = delayd_h;
        delayd_t* curr = delayd_h->d_next;
        while(curr->d_wakeTime != DUMMYTAILVAL)
        {
            cpu_t tod;
            STCK(tod);
            if(tod >= curr->d_wakeTime)  /*wakeup time has passed*/
            {
                SYSCALL(VERHOGEN, &(curr->d_supStruct->private_sem), 0, 0); /*wake the process up*/

                /*remove from ADL*/
                prev->d_next = curr->d_next;

                /*return to free list*/
                deallocateDelayd(curr);

                /*move pointer*/
                curr = prev->d_next;
            }
            else
            {
                /*move pointers*/
                prev = prev->d_next;
                curr = curr->d_next;
            }
            
        }

        SYSCALL(VERHOGEN, &ADL_sem, 0, 0);  /*release mutual exclusion over the ADL*/
    }
    
}

void initADL()
{
    static delayd_t delaydTable[UPROCMAX + 2];

    /*add each element from the static array of delay event descriptor nodes to the free list*/
    int i;
    for (i = 0; i < UPROCMAX + 2; i++)
    {
        if (i != UPROCMAX + 1){
            /*connect delay descriptors*/
            delaydTable[i].d_next = &delaydTable[i + 1];
        }
        else{
            /*set the last one d_next to NULL*/
            delaydTable[i].d_next = NULL;
        }
    }

    /*initialize the active list with two dummy nodes*/

    delaydFree_h = &delaydTable[0];    /*point head of free list to the first delayd of static array*/

    /*create dummy head node*/
    delayd_h = delaydFree_h;
    delaydFree_h = delaydFree_h->d_next;

    /*create dummy tail node*/
    delayd_t* dummy_tail = delaydFree_h;
    delaydFree_h = delaydFree_h->d_next;
    
    /*connect head and tail dummy node*/
    delayd_h->d_next = dummy_tail;
    dummy_tail->d_next = NULL;

    /*assign sentinel waketime for head and tail dummy node*/
    delayd_h->d_wakeTime = DUMMYHEADVAL;
    dummy_tail->d_wakeTime = DUMMYTAILVAL;


   /*launch the Delay Daemon*/
    state_t daemon_state;
    daemon_state.s_pc = (memaddr) delayDaemon;
    daemon_state.s_t9 = (memaddr) delayDaemon;
    daemon_state.s_sp = *((int*) RAMBASEADDR) + *((int*) RAMBASESIZE) - PAGESIZE ;   /*RAMTOP - (stack for test)*/
    daemon_state.s_status = ALLOFF | IEPBITON | IMON;   /*kernel-mode with all interrupts enabled*/
    daemon_state.s_entryHI = (DAEMONASID << ASID); 
    SYSCALL(CREATETHREAD, &daemon_state, NULL, 0);     
}
