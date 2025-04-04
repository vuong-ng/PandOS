#include "initProc.h"

static support_t uproc_support [UPROCMAX];

void test()
{
    /*Initialize the Level 4/Phase 3 data structures*/
    initSwapStructs();

    /*initialize device semaphores*/
    int i = 0;
    for(i = 0; i < DEVSEMNO; i++)   
        device_sem[i] = 1;


    /*launching UProcs*/
    for(i = 0; i < UPROCMAX; i++)
    {
        /*initialize the process state*/
        state_t uproc_state;
        uproc_state.s_pc = TEXTSTART;
        uproc_state.s_t9 = TEXTSTART;
        uproc_state.s_sp = KUSEGSTACK;               
        uproc_state.s_status | TEBITON | IECBITON | IMON | KUCBITON; /*user-mode with all interrupts and the processor Local Timer enabled*/
        uproc_state.s_entryHI & ALLOFF | (i << ASID);                   /*EntryHi.ASID set to the process’s unique ID*/

        /*initialize the support structure*/
        uproc_support[i].sup_asid = i;

        uproc_support[i].sup_exceptContext[PGFAULTEXCEPT].c_pc = (memaddr) /*TLB handler addr.*/;
        uproc_support[i].sup_exceptContext[GENERALEXCEPT].c_pc = (memaddr) /*General excp handler addr.*/;
        /*kernel-mode with all interrupts and the Processor Local Timer enabled.*/
        uproc_support[i].sup_exceptContext[PGFAULTEXCEPT].c_status = ALLOFF | IMON | IECBITON | TEBITON & ~KUCBITON;
        uproc_support[i].sup_exceptContext[GENERALEXCEPT].c_status = ALLOFF | IMON | IECBITON | TEBITON & ~KUCBITON;

        /*utilize the two stack spaces allocated in the Support Structure*/
        uproc_support[i].sup_exceptContext[PGFAULTEXCEPT].c_stackPtr = &(uproc_support[i].sup_stackTLB[499]);
        uproc_support[i].sup_exceptContext[GENERALEXCEPT].c_stackPtr = &(uproc_support[i].sup_stackGen[499]);

        /*initialize process's page table*/
        int j;
        for(j = 0; j < PAGETABLESIZE - 1; j++)
        {
            uproc_support[i].sup_privatePgTbl[j].EntryHi = ALLOFF | ((VPNSHIFT + i) << PFN) | (i << ASID);
            uproc_support[i].sup_privatePgTbl[j].EntryLo = ALLOFF | (D) & (~V) & (~G);
        }

        uproc_support[i].sup_privatePgTbl[PAGETABLESIZE - 1].EntryHi = ALLOFF | ((STACKPGVPN) << PFN) | (i << ASID);
        uproc_support[i].sup_privatePgTbl[PAGETABLESIZE - 1].EntryLo = ALLOFF | (D) & (~V) & (~G);
        /*VPN for the stack page set to starting address whose top end is 0xC000.0000 (SP)*/



        /*launch a process*/
        SYSCALL(CREATETHREAD, &uproc_state, &uproc_support[i], 0);
    }


    /*terminate*/

}


void uTLB_RefillHandler()
{
    int page_number_missing = ((((state_t*) BIOSDATAPAGE)->s_entryHI) >> PFN) & GET20LSB;
    pte_t pte = curr_proc->p_supportStruct->sup_privatePgTbl[page_number_missing - VPNSHIFT];

    setENTRYHI(pte.EntryHi);          
    setENTRYLO(pte.EntryLo);  
    TLBWR();

    LDST((state_t*) BIOSDATAPAGE);

}
        