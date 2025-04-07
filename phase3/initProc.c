#include "../h/initProc.h"

HIDDEN support_t uproc_support [UPROCMAX + 1];
HIDDEN int halt_sem = 0;

int mutex_device_sem[DEVSEMNO];

void disableInterrupts(){setSTATUS(getSTATUS() & ~IECBITON);}
void enableInterrupts(){setSTATUS(getSTATUS() | IECBITON);}

void test()
{
    /*Initialize the Level 4/Phase 3 data structures*/
    initSwapStructs();

    /*initialize device semaphores*/
    int i;
    for(i = 0; i < DEVSEMNO; i++)   
        mutex_device_sem[i] = 1;


    /*launching UProcs*/
    for(i = 1; i <= UPROCMAX; i++)
    {
        /*initialize the process state*/
        state_t uproc_state;
        uproc_state.s_pc = TEXTSTART;
        uproc_state.s_t9 = TEXTSTART;
        uproc_state.s_sp = KUSEGSTACK;               
        uproc_state.s_status = ALLOFF | TEBITON | IECBITON | IMON | KUCBITON; /*user-mode with all interrupts and the processor Local Timer enabled*/
        uproc_state.s_entryHI = ALLOFF | (i << ASID);                   /*EntryHi.ASID set to the process’s unique ID*/

        /*initialize the support structure*/
        uproc_support[i].sup_asid = i;

        uproc_support[i].sup_exceptContext[PGFAULTEXCEPT].c_pc = (memaddr) pager;   /*TLB handler addr.*/
        uproc_support[i].sup_exceptContext[GENERALEXCEPT].c_pc = (memaddr) sptGeneralExceptionHandler; /*General excp handler addr.*/
        /*kernel-mode with all interrupts and the Processor Local Timer enabled.*/
        uproc_support[i].sup_exceptContext[PGFAULTEXCEPT].c_status = ALLOFF | IMON | IECBITON | TEBITON;
        uproc_support[i].sup_exceptContext[GENERALEXCEPT].c_status = ALLOFF | IMON | IECBITON | TEBITON;

        /*utilize the two stack spaces allocated in the Support Structure*/
        uproc_support[i].sup_exceptContext[PGFAULTEXCEPT].c_stackPtr = &(uproc_support[i].sup_stackTLB[499]);  /*not sure*/
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


    /*terminate (PANIC)*/
    SYSCALL(PASSERN, &halt_sem, 0, 0);


}


void uTLB_RefillHandler()
{
    debug(0,1,2,3);
    unsigned int page_number_missing = ((((state_t*) BIOSDATAPAGE)->s_entryHI) >> PFN);
    pte_t pte = curr_proc->p_supportStruct->sup_privatePgTbl[page_number_missing % PAGETABLESIZE];

    setENTRYHI(pte.EntryHi);          
    setENTRYLO(pte.EntryLo);  
    TLBWR();

    LDST((state_t*) BIOSDATAPAGE);

}
        