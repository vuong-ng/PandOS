/***************************** INITPROC.C ****************************
* Written by Long Pham & Vuong Nguyen
* Module: User Process Initialization for Pandos OS Phase 3
* Components:
* - Interrupt Control: Functions to enable/disable interrupts
* - Process Setup: Function to initialize and launch user processes
* - Device Management: Mutex semaphores for devices
*
* Global State:
* - uproc_support[UPROCMAX + 1]: Support structures array for user processes, plus 1 for sentinel 
* - mutex_device_sem[DEVSEMNO]: Device mutex semaphores
* - masterSemaphore: master mutex semphore for phase 3
*********************************************************/

#include "../h/initProc.h"

/* initialize UPROCMAX number of support structures as a static array*/
HIDDEN support_t uproc_support [UPROCMAX + 1];

/*initialize peripheral device semaphores*/
int mutex_device_sem[DEVSEMNO];

/*********************************************************
* Purpose: Disables interrupts by clearing IEc bit
* Parameters: None
* Returns: void
*********************************************************/
void disableInterrupts(){setSTATUS(getSTATUS() & (~IECBITON));} 

/*********************************************************
* Purpose: Enables interrupts by setting IEc bit
* Parameters: None
* Returns: void
*********************************************************/
void enableInterrupts(){setSTATUS(getSTATUS() | IECBITON);}

int masterSemaphore;

/*********************************************************
* Purpose: Initializes U-proc state, support struture and 
*          launches user processes
* Parameters: None
* Returns: None
* Operations:
* 1. Initializes master semaphore
* 2. Initializes swap pool table
* 2. Intializes swap pool entries
* 4. Initilizes support structure for each u-proc
* 3. Initializes device mutex semaphores for each device
* 4. Launches UPROCMAX user processes
*********************************************************/
void test()
{
    /*initialize master semaphore*/
    masterSemaphore = 0;

    /*Initialize the Level 4/Phase 3 data structures: swap pool entries, swap pool semaphore*/
    initSwapStructs();

    /*initialize device mutex semaphores*/
    int i;
    for(i = 0; i < DEVSEMNO; i++)   
        mutex_device_sem[i] = 1;

    /*launching 8 process run in user-mode - Uproc*/
    for(i = 1; i <= UPROCMAX ; i++)
    {
        /*initialize the Uproc state*/
        state_t uproc_state;

        /*pc and t9 set to 0x800.000B0, the address of the start of the .text section*/
        uproc_state.s_pc = TEXTSTART;
        uproc_state.s_t9 = TEXTSTART;

        /* SP set to 0xC000.0000 */
        uproc_state.s_sp = KUSEGSTACK;

        /* Uproc status set to user-mode with all interrupts and the processor Local Timer enabled*/
        uproc_state.s_status = ALLOFF | TEBITON | IEPBITON | IMON | KUPBITON; 

        /*EntryHi.ASID set to the process’s unique ID*/
        uproc_state.s_entryHI = ALLOFF | (i << ASID);                         

        /*initialize the support structure*/
        /*set asid in support_t to i */
        uproc_support[i].sup_asid = i;

        /*initialize context area*/
        /* set the PC of 2 exception:
        1. page fault exception's pc to the address of Support Level's TLB handler, 
        2.general exception's pc to the addr of Support Level's general exception */
        uproc_support[i].sup_exceptContext[PGFAULTEXCEPT].c_pc = (memaddr) pager;                      /*TLB handler addr.*/
        uproc_support[i].sup_exceptContext[GENERALEXCEPT].c_pc = (memaddr) sptGeneralExceptionHandler; /*General exception handler address.*/

        /*Set the two Status registers to: kernel-mode with all interrupts and the Processor Local Timer enabled*/
        uproc_support[i].sup_exceptContext[PGFAULTEXCEPT].c_status = ALLOFF | IMON | IEPBITON | TEBITON;
        uproc_support[i].sup_exceptContext[GENERALEXCEPT].c_status = ALLOFF | IMON | IEPBITON | TEBITON;

        /*utilize the two stack spaces allocated in the Support Structure to the address of the end of these areas*/
        uproc_support[i].sup_exceptContext[PGFAULTEXCEPT].c_stackPtr = &(uproc_support[i].sup_stackTLB[STACKSIZE - 1]);  
        uproc_support[i].sup_exceptContext[GENERALEXCEPT].c_stackPtr = &(uproc_support[i].sup_stackGen[STACKSIZE - 1]);


        /*initialize process's page table*/
        int j;
        for(j = 0; j < PAGETABLESIZE - 1; j++)
        {
            /*set asid in each page table entry to i - the asid is at bit 6 - bit 11*/
            /*VPN field (starts from bit 12 to bit 31) will be set to [0x80000000 ... 0x8001E000] to the first 31 entries*/
            uproc_support[i].sup_privatePgTbl[j].EntryHi = ALLOFF | ((VPNSHIFT + j) << PAGESHIFT) | (i << ASID);
            uproc_support[i].sup_privatePgTbl[j].EntryLo = ALLOFF | (D) & (~V) & (~G);
        }

        /*VPN of the stack page (page Table entry 31) should be set to 0xBFFFF*/
        uproc_support[i].sup_privatePgTbl[PAGETABLESIZE - 1].EntryHi = ALLOFF | (STACKPGVPN << PAGESHIFT) | (i << ASID);

        /*set all pages in PandOS uproc to be Dirty and Invalid in the beginning with Global bit off*/
        uproc_support[i].sup_privatePgTbl[PAGETABLESIZE - 1].EntryLo = ALLOFF | (D) & (~V) & (~G);


        /*call SYS1 to create process*/
        SYSCALL(CREATETHREAD, &uproc_state, &uproc_support[i], 0);
    }

    /*test P mastersemaphore 8 times when all U-proc is launched*/
    int t;
    for (t = 0; t < UPROCMAX; t++)
        SYSCALL(PASSERN, &masterSemaphore, 0, 0);
    
    /*after lauching all processes, `test` issue SYS2, triggering a HALT by the Nucleus*/
    SYSCALL(TERMINATETHREAD, 0, 0, 0);

}



        