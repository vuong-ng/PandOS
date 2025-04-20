#include "../h/delayDaemon.h"

HIDDEN delayd_t* delayd_h;
HIDDEN delayd_t* delaydFree_h;
HIDDEN int ADL_sem = 1;

#define DUMMYHEADVAL -1
#define DUMMYTAILVAL 2147000000

void insertADL(delayd_t* delayd)
{
    delayd_t* prev = delayd_h;
    delayd_t* curr = delayd_h->d_next;
    while ((curr->d_wakeTime != DUMMYTAILVAL) && !(prev->d_wakeTime <= delayd->d_wakeTime))
    {
        prev = prev->d_next;
        curr = curr->d_next;
    }
    prev->d_next = delayd;
    delayd->d_next = curr;
}

delayd_t* allocateDelayd(support_t* sPtr, int wakeTime)
{
    delayd_t* freeDelayd = delaydFree_h;
    delaydFree_h = delaydFree_h->d_next;

    /*populate the delay descriptor*/
    cpu_t tod;
    STCK(tod);
    freeDelayd->d_wakeTime = tod + wakeTime * 1000000;
    freeDelayd->d_supStruct = sPtr;
    return freeDelayd;
}

void delay(support_t* sPtr)
{
    /*support_t* sPtr = SYSCALL(GETSPTPTR, 0, 0, 0);*/
    int wakeTime = sPtr->sup_exceptState[GENERALEXCEPT].s_a1;
    if(wakeTime < 0)
        SYSCALL(TERMINATE, 0, 0, 0);
    
    SYSCALL(PASSERN, &ADL_sem, 0, 0);

    if (delaydFree_h == NULL) /*no delay descriptor available*/
    {
        SYSCALL(VERHOGEN, &ADL_sem, 0, 0);
        SYSCALL(TERMINATE, 0, 0, 0);
    }

    /*allocate 1 free delay descriptor from free list & populate the delay descriptor*/
    /*delayd_t* freeDelayd = delaydFree_h;
    delaydFree_h = delaydFree_h->d_next;
    cpu_t tod;
    STCK(tod);
    freeDelayd->d_wakeTime = tod + wakeTime * 1000000;
    freeDelayd->d_supStruct = sPtr;*/
    delayd_t* freeDelayd = allocateDelayd(sPtr, wakeTime);

    /*insert to ADL (sorted)*/
    /*delayd_t* prev = delayd_h;
    delayd_t* curr = delayd_h->d_next;
    while ((curr->d_wakeTime != DUMMYTAILVAL) && !(prev->d_wakeTime <= freeDelayd->d_wakeTime))
    {
        prev = prev->d_next;
        curr = curr->d_next;
    }
    prev->d_next = freeDelayd;
    freeDelayd->d_next = curr;*/
    insertADL(freeDelayd);


    disableInterrupts();
    SYSCALL(VERHOGEN, &ADL_sem, 0, 0);
    SYSCALL(PASSERN, &(sPtr->private_sem), 0, 0);
    enableInterrupts();

    
    /*LDST: executed inside support level syscall handler*/
    LDST((state_t*) &(sPtr->sup_exceptState[GENERALEXCEPT]));
    

}

void delayDaemon()
{
    while((1+1) == 2)
    {
        SYSCALL(WAITCLOCK, 0, 0, 0);
        SYSCALL(PASSERN, &ADL_sem, 0, 0);

        delayd_t* prev = delayd_h;
        delayd_t* curr = delayd_h->d_next;
        while(curr->d_wakeTime != DUMMYTAILVAL)
        {
            cpu_t tod;
            STCK(tod);
            if(tod >= curr->d_wakeTime)  /*wakeup time has passed*/
            {
                SYSCALL(VERHOGEN, &(curr->d_supStruct->private_sem), 0, 0);
                /*remove from ADL & return to free list*/
                prev->d_next = curr->d_next;
                curr->d_next = delaydFree_h;
                delaydFree_h = curr;

                /*move pointer*/
                curr = prev->d_next;
            }
            else
            {
                prev = prev->d_next;
                curr = curr->d_next;
            }
            
        }

        SYSCALL(VERHOGEN, &ADL_sem, 0, 0);
    }
    
}

void initADL()
{
    static delayd_t delaydTable[UPROCMAX + 2];

    /*connect delayd*/
    int i;
    for (i = 0; i < UPROCMAX + 2; i++)
    {
        if (i != UPROCMAX + 1){
            /*connect delay descriptors*/
            delaydTable[i].d_next = &delaydTable[i + 1];
        }
        else{
            /*set the last one s_next to NULL*/
            delaydTable[i].d_next = NULL;
        }
    }

    delaydFree_h = &delaydTable[0];    /*point head point of free list to the delayd linked list*/

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
    daemon_state.s_sp = *((int*) RAMBASEADDR) + *((int*) RAMBASESIZE) ;   /*RAMTOP*/
    daemon_state.s_status = ALLOFF | IEPBITON | IMON;
    daemon_state.s_entryHI = (0 << ASID); 
    SYSCALL(CREATETHREAD, &daemon_state, NULL, 0);     
}
