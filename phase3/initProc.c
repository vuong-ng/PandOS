#include "../h/initProc.h"



HIDDEN support_t uproc_support [UPROCMAX + 1];


int mutex_device_sem[DEVSEMNO];

void disableInterrupts(){setSTATUS(getSTATUS() & (~IECBITON));}
void enableInterrupts(){setSTATUS(getSTATUS() | IECBITON);}


int masterSemaphore;
void test()
{
    masterSemaphore = 0;
    int halt_sem = 0;
    /*Initialize the Level 4/Phase 3 data structures*/
    initSwapStructs();

    /*initialize device semaphores*/
    int i;
    for(i = 0; i < DEVSEMNO; i++)   
        mutex_device_sem[i] = 1;


    /*launching UProcs*/
    for(i = 1; i <= UPROCMAX ; i++)
    {
        /*initialize the process state*/
        state_t uproc_state;
        uproc_state.s_pc = TEXTSTART;
        uproc_state.s_t9 = TEXTSTART;
        uproc_state.s_sp = KUSEGSTACK;               
        uproc_state.s_status = ALLOFF | TEBITON | IEPBITON | IMON | KUPBITON; /*user-mode with all interrupts and the processor Local Timer enabled*/
        uproc_state.s_entryHI = ALLOFF | (i << ASID);                   /*EntryHi.ASID set to the process’s unique ID*/

        /*initialize the support structure*/
        uproc_support[i].sup_asid = i;

        uproc_support[i].sup_exceptContext[PGFAULTEXCEPT].c_pc = (memaddr) pager;   /*TLB handler addr.*/
        uproc_support[i].sup_exceptContext[GENERALEXCEPT].c_pc = (memaddr) sptGeneralExceptionHandler; /*General excp handler addr.*/
        /*kernel-mode with all interrupts and the Processor Local Timer enabled.*/
        uproc_support[i].sup_exceptContext[PGFAULTEXCEPT].c_status = ALLOFF | IMON | IEPBITON | TEBITON;
        uproc_support[i].sup_exceptContext[GENERALEXCEPT].c_status = ALLOFF | IMON | IEPBITON | TEBITON;

        /*utilize the two stack spaces allocated in the Support Structure*/
        uproc_support[i].sup_exceptContext[PGFAULTEXCEPT].c_stackPtr = &(uproc_support[i].sup_stackTLB[499]);  
        uproc_support[i].sup_exceptContext[GENERALEXCEPT].c_stackPtr = &(uproc_support[i].sup_stackGen[499]);




        /*initialize process's page table*/
        int j;
        for(j = 0; j < PAGETABLESIZE - 1; j++)
        {
            uproc_support[i].sup_privatePgTbl[j].EntryHi = ALLOFF | ((VPNSHIFT + j) << PFN) | (i << ASID);
            uproc_support[i].sup_privatePgTbl[j].EntryLo = ALLOFF | (D) /*& (~V) & (~G)*/;
        }

        uproc_support[i].sup_privatePgTbl[PAGETABLESIZE - 1].EntryHi = ALLOFF | (STACKPGVPN << PFN) | (i << ASID);
        uproc_support[i].sup_privatePgTbl[PAGETABLESIZE - 1].EntryLo = ALLOFF | (D) /*& (~V) & (~G)*/;
        /*VPN for the stack page set to starting address whose top end is 0xC000.0000 (SP)*/


        /*launch a process*/
        SYSCALL(CREATETHREAD, &uproc_state, &uproc_support[i], 0);
    }

    int t;
    for (t = 0; t < UPROCMAX; t++)
        SYSCALL(PASSERN, &masterSemaphore, 0, 0);
    
    SYSCALL(TERMINATETHREAD, 0, 0, 0);

}



        