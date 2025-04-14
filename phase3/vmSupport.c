#include "../h/vmSupport.h"
HIDDEN swap_frame_t swap_pool_table [UPROCMAX * 2]; /*each entry per Swap Pool frame, recording information about the logical page occupying it*/
HIDDEN int swap_pool_sem;


void initSwapStructs()
{
    /*The Swap Pool table and Swap Pool semaphore*/
    int i;
    for(i = 0; i < UPROCMAX * 2; i++)  
    {
        swap_pool_table[i].asid = -1;
        swap_pool_table[i].vpn = 0;
        swap_pool_table[i].matching_pte_ptr = NULL;
    } 
    swap_pool_sem = 1;
}

/*implement the page replacement algorithm*/
int getVictim()
{
    static int victim = 0;
    int frame_index = victim % (UPROCMAX * 2);     /*frame in swap pool to load data from backing store into*/
    victim++;
    return frame_index;
}
int flashOperation(unsigned int dev_no, unsigned int data0, unsigned int blocknumber, unsigned int command)
{
    int* mutex_flash_sem = &mutex_device_sem[(FLASHINT - 3) * DEVPERINT + dev_no];
    SYSCALL(PASSERN, mutex_flash_sem, 0, 0);
    device_t* flash_dev_reg = DEVREGBASE + (FLASHINT - 3) * DEVREGINTSCALE + dev_no * DEVREGDEVSCALE;
    flash_dev_reg->d_data0 = data0;
    disableInterrupts();
    flash_dev_reg->d_command = ALLOFF | (blocknumber << SETCMDBLKNO) | command;
    int status = SYSCALL(WAITIO, FLASHINT, dev_no, 0);
    enableInterrupts();
    SYSCALL(VERHOGEN, mutex_flash_sem, 0, 0);

    return status;

}

void pager()
{
    support_t* sPtr = SYSCALL(GETSPTPTR, 0, 0, 0);  /*pointer to the Current Process’s Support Structure*/
    unsigned int TLB_excp_cause = sPtr->sup_exceptState[PGFAULTEXCEPT].s_cause;
    if (((TLB_excp_cause >> GETEXCCODE) & CLEAR26MSB) == 1) /*TLB-Modification Exception*/
    {
        sptTrapHandler(sPtr, NULL);  /*program trap*/
    }

    SYSCALL(PASSERN, &swap_pool_sem, 0, 0);
    unsigned int missing_VPN = (sPtr->sup_exceptState[PGFAULTEXCEPT].s_entryHI >> PFN);
    unsigned int frame_index = getVictim();     /*frame in swap pool to load data from backing store into*/
    memaddr replaced_PFN = SWAPPOOLADDR + (frame_index * PAGESIZE);     /*the actual physical frame in RAM*/      /*should be 0x20020000*/
    swap_frame_t* victim_frame = &(swap_pool_table[frame_index]);

    if(victim_frame->asid != -1)  /*entry in swap pool occupied, assumed dirty*/
    {
            
            disableInterrupts();  /*atomic execution - disable interrupt*/
            (victim_frame->matching_pte_ptr)->EntryLo &= ~V;  /*mark PTE invalid*/
            TLBCLR();
            enableInterrupts();   /*atomic execution - enable interrupt*/

            int status = flashOperation((victim_frame->asid) - 1, replaced_PFN, (victim_frame->vpn) % PAGETABLESIZE, WRITEBLK);

            /*treat error as trap*/
            if(status != READY)
            {
                sptTrapHandler(sPtr, swap_pool_sem);
            }
    }

    int status = flashOperation(sPtr->sup_asid - 1, replaced_PFN, missing_VPN % PAGETABLESIZE, READBLK);

    /*treat error as trap*/
    if(status != READY)
    {
        sptTrapHandler(sPtr, swap_pool_sem);
    }
        
    victim_frame->asid = sPtr->sup_asid;     /*the frame now belongs to the current process*/
    victim_frame->vpn = missing_VPN;    /*update VPN*/
    victim_frame->matching_pte_ptr = &(sPtr->sup_privatePgTbl[missing_VPN % PAGETABLESIZE]);  /*page table entry whose VPN is VPN_missing*/
    
    disableInterrupts();  /*atomic execution - disable interrupt*/
    (victim_frame->matching_pte_ptr)->EntryLo = ALLOFF | (replaced_PFN) | V | D; 
    TLBCLR();
    enableInterrupts();   /*atomic execution - enable interrupt*/

    SYSCALL(VERHOGEN, &swap_pool_sem, 0, 0);
    LDST((state_t*) (&(sPtr->sup_exceptState[PGFAULTEXCEPT])));
}