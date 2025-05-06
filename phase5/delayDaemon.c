/***************************** DELAYDAEMON.C ****************************
* Written by Long Pham & Vuong Nguyen
* Module: Delay Daemon for Pandos OS 
* Components:
* - Active Delay List (ADL): Sorted list of delayed processes
* - SYS18 (delay()): Function to allocate delay descriptors and P the U-proc
* - Delay Daemon: Background process for wake-up processes
* - Initializing ADL (delay()): Initialize Active Delay List 
* 
* Global Variables:
* - delayd_h: Head pointer of active delay events list
* - delaydFree_h: Head pointer of free delay descriptors
* - ADL_sem: Mutual exclusion semaphore for ADL access
*
* Data Structures:
* - delayd_t: Delay event descriptor containing wake time and U-proc support struct
*********************************************************/


#include "../h/delayDaemon.h"

/*value for sentinel node at the head of active delay events list*/
#define DUMMYHEADVAL -1 

/*value for sentinel node at the end of active delay events list*/
#define DUMMYTAILVAL __INT_MAX__

#define DAEMONASID 0         /*daemon ASID*/
#define MILLION	1000000

HIDDEN delayd_t* delayd_h = NULL;     /*head pointer points to head of active delay events*/
HIDDEN delayd_t* delaydFree_h = NULL; /*head pointer points to head of free delay events*/
HIDDEN int ADL_sem = 1;               /*mutual exclusion semaphore for ADL*/

/*******************************************************************************
* Function: insertADL
* Description: Inserts delay event descriptor into Active Delay List (ADL) maintaining 
*              sorted order by wake time
*
* Parameters:
* - delayd: Pointer to delay descriptor to insert
*
* Algorithm:
* 1. Start at ADL head, Traverse list until finding correct position where:
*    prev->d_wakeTime <= delayd->d_wakeTime
* 3. Insert node to ADL
*******************************************************************************/
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

/*******************************************************************************
* Function: allocateDelayd
* Description: Allocates and initializes a delay event descriptor from free list
*              Populate the process its wake time 
*
* Parameters:
* - sPtr: Pointer to support structure of delayed process
* - wakeTime: Delay duration in seconds
*
* Returns:
* - Pointer to newly allocated delay descriptor
*******************************************************************************/
delayd_t* allocateDelayd(support_t* sPtr, int wakeTime)
{
    /*allocate one delayd_t from the free list*/
    delayd_t* freeDelayd = delaydFree_h;
    delaydFree_h = delaydFree_h->d_next;

    /*populate the delay descriptor*/
    cpu_t tod;
    STCK(tod);

    /*populate wake time for the process*/
    freeDelayd->d_wakeTime = tod + wakeTime * MILLION;
    freeDelayd->d_supStruct = sPtr;
    return freeDelayd;
}

/*******************************************************************************
* Function: deallocateDelayd
* Description: Returns a delay descriptor to the free list by adding it to the 
*              front of the list (LIFO order)
*
* Parameters:
* - delayd: Pointer to delay descriptor to deallocate
*
* Returns: None
*******************************************************************************/
void deallocateDelayd(delayd_t* delayd)
{
    delayd->d_next = delaydFree_h;
    delaydFree_h = delayd;
}

/*******************************************************************************
* Function: ADLEmpty
* Description: Returns if there is any free delayd_t left in the free list
*
* Parameters:
* - delaydFree_h: head of free list
*
* Returns:
* - TRUE or FALSE whether there is any free delayd_t left in the free list
*******************************************************************************/
int ADLEmpty(delayd_t* delaydFree_h)
{
    return (delaydFree_h == NULL);
}

/*******************************************************************************
* Function: delay (SYS18)
* Description: System call handler that delays a user process for specified time
*              by blocking it on its private semaphore and add its delay event description to ADL
*
* Parameters:
* - sPtr: Pointer to the support structure of process to delay
*
* Error Conditions:
* - Negative delay time: Process terminated
* - No free delay descriptors: Process terminated
*
* Algorithm:
* 1. Get delay time from support structure
* 2. Validate delay time >= 0
* 3. Obtain ADL mutex
* 4. Allocate & initialize delay descriptor
* 5. Insert delay descriptor into sorted ADL
* 6. Release mutex of ADL semaphore
* 7. Block process on private semaphore
*
* Note: Step 6 and 7 are done atomically
*******************************************************************************/
void delay(support_t* sPtr)
{
    /*obtain wake time from current process saved exception state*/
    int wakeTime = sPtr->sup_exceptState[GENERALEXCEPT].s_a1;

    /*Attempt to request a Delay for less than 0 seconds is an error, result in the U-proc begin terminated*/
    if(wakeTime < 0)
        SYSCALL(TERMINATE, 0, 0, 0);
    
    SYSCALL(PASSERN, &ADL_sem, 0, 0);   /*obtain mutual exclusion over the ADL*/

    if (ADLEmpty(delaydFree_h)) /*no free delay descriptor left*/
    {
        /*releasing mutual exclusion over the ADL and terminate*/
        SYSCALL(VERHOGEN, &ADL_sem, 0, 0);
        SYSCALL(TERMINATE, 0, 0, 0);
    }

    /*allocate 1 free delay descriptor from free list & populate the delay descriptor*/
    delayd_t* freeDelayd = allocateDelayd(sPtr, wakeTime);

    /*insert to ADL (sorted)*/
    insertADL(freeDelayd);

    /*disable interrupts to operate atomically*/
    disableInterrupts();
    SYSCALL(VERHOGEN, &ADL_sem, 0, 0);              /*release mutual exclusion over the ADL*/
    SYSCALL(PASSERN, &(sPtr->private_sem), 0, 0);   /*block the executing Uproc on its private semaphore*/
    enableInterrupts();

    
    /*return control (LDST) to the U-proc at the instruction immediately following the SYS18.*/
    LDST((state_t*) &(sPtr->sup_exceptState[GENERALEXCEPT]));

}

/*******************************************************************************
 * Function: delayDaemon
 * Description: Background daemon process that manages delayed processes by checking
 *              wake times and wakes processes when their wake time has passed
 *
 * Algorithm:
 * 1. Infinite loop:
 *    - Wait for clock tick (100ms)
 *    - Obtain ADL mutex
 *    - Examine the ADL
 *    - Removing all delay event descriptor nodes whose wake time has passe
 *      a) V process private semaphore
 *      b) Remove delay event descriptor from ADL
 *      c) Return descriptor to free list
 *    - Release ADL mutex
 *
 * Returns: None
 *******************************************************************************/
void delayDaemon()
{
    /*delay daemon is an infinite loop*/
    while(TRUE)
    {
        SYSCALL(WAITCLOCK, 0, 0, 0);        /* call Wait For Clock to wait for 100 milliseconds*/
        SYSCALL(PASSERN, &ADL_sem, 0, 0);   /*gain mutual exclusion of the ADL*/

        delayd_t* prev = delayd_h;
        delayd_t* curr = delayd_h->d_next;
        while(curr->d_wakeTime != DUMMYTAILVAL)
        {
            cpu_t tod;
            STCK(tod);
            /*wake up any process that its wake up time has passed*/
            if(tod >= curr->d_wakeTime) 
            {
                /*wake the process up*/
                SYSCALL(VERHOGEN, &(curr->d_supStruct->private_sem), 0, 0); 

                /*remove from ADL*/
                prev->d_next = curr->d_next;

                /*return to free list*/
                deallocateDelayd(curr);

                /*move to the next process*/
                curr = prev->d_next;
            }
            else
            {
                /*move to the next one*/
                prev = prev->d_next;
                curr = curr->d_next;
            }
            
        }
        /*release mutual exclusion over the ADL*/
        SYSCALL(VERHOGEN, &ADL_sem, 0, 0); 
    }
    
}

/*******************************************************************************
* Function: initADL
* Description: Initializes the delay data structure:
*              1. Creating delay descriptor static list
*              2. Setting up free list
*              3. Initializing ADL with sentinel nodes (head and tail)
*              4. Launching delay daemon process
*
* Initializing delay data Structures:
* - Static array of UPROCMAX+2 delay descriptors
* - ADL with dummy head (DUMMYHEADVAL) and tail (DUMMYTAILVAL)
* - Free list head pointers pointing to all free descriptors
*
* Initialzing Daemon U-proc:
* - Kernel mode with interrupts enabled
* - ASID 0
* - Stack set to unused frame at the end of RAM
* - PC and t9 at delayDaemon()
*******************************************************************************/
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

    /*initialize the active list with two dummy nodes head and tail*/

    /*point head of free list to the first delayd of static array*/
    delaydFree_h = &delaydTable[0];   

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

    /*initalize the daemon state*/
    state_t daemon_state;

    /* set PC and t9 to the function implementing the Delay Daemon*/
    daemon_state.s_pc = (memaddr) delayDaemon;
    daemon_state.s_t9 = (memaddr) delayDaemon;

    /*set SP to an unused frame at the end of RAM*/
    daemon_state.s_sp = *((int*) RAMBASEADDR) + *((int*) RAMBASESIZE) - PAGESIZE ; 

    /*delay daemon runs in kernel-mode with all interrupts enabled*/
    daemon_state.s_status = ALLOFF | IEPBITON | IMON;

    /*EntryHi.ASID is set to the kernel ASID : 0*/
    daemon_state.s_entryHI = (DAEMONASID << ASID); 

    /*create delay daemon*/
    SYSCALL(CREATETHREAD, &daemon_state, NULL, 0);     
}
