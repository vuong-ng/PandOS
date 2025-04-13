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

void pager()
{
    support_t* sPtr = SYSCALL(GETSPTPTR, 0, 0, 0);  /*pointer to the Current Process’s Support Structure*/
    unsigned int TLB_excp_cause = sPtr->sup_exceptState[PGFAULTEXCEPT].s_cause;
    unsigned int cause_exccode = (TLB_excp_cause >> GETEXCCODE) & CLEAR26MSB;

    if (cause_exccode == 1) /*TLB-Modification Exception*/
    {
        sptTrapHandler(sPtr, NULL);  /*program trap*/
    }

    SYSCALL(PASSERN, &swap_pool_sem, 0, 0);
    unsigned int missing_VPN = (sPtr->sup_exceptState[PGFAULTEXCEPT].s_entryHI >> PFN);
    unsigned int frame_index = getVictim();     /*frame in swap pool to load data from backing store into*/
    memaddr replaced_PFN = SWAPPOOLADDR + (frame_index * PAGESIZE);     /*the actual physical frame in RAM*/      /*should be 0x20020000*/

    pte_t* matching_pte = swap_pool_table[frame_index].matching_pte_ptr;

    if(swap_pool_table[frame_index].asid != -1)  /*entry in swap pool occupied, assumed dirty*/
    {
            debug(7,7,7,7);
            disableInterrupts();  /*atomic execution - disable interrupt*/
            matching_pte->EntryLo &= ~V;  /*mark PTE invalid*/
            
            /*update TLB as needed (how to determine if entry is cached in TLB?)*/
            /*
            TLBP();
            if(Index == 0)
            setENTRYHI( (*(swap_pool_table[frame_index].matching_pte_ptr)).EntryHi);
            setENTRYLO( (*(swap_pool_table[frame_index].matching_pte_ptr)).EntryLo );
            TLBWI();
            */
            TLBCLR();
            enableInterrupts();   /*atomic execution - enable interrupt*/
            
            int dev_no = swap_pool_table[frame_index].asid - 1;                                                        /*process's flash determined by its asid*/
            device_t* flash_dev_reg = DEVREGBASE + (FLASHINT - 3) * DEVREGINTSCALE + dev_no * DEVREGDEVSCALE;
            int* mutex_flash_sem = &mutex_device_sem[(FLASHINT - 3) * DEVPERINT + dev_no];

            SYSCALL(PASSERN, mutex_flash_sem, 0, 0);

            flash_dev_reg->d_data0 = replaced_PFN;        /*PFN in RAM to be written to backing store*/
            
            disableInterrupts();

            flash_dev_reg->d_command = ALLOFF | (((swap_pool_table[frame_index].vpn) % PAGETABLESIZE) << SETCMDBLKNO) | WRITEBLK;   /*write, block is vpn occupying the swap entry*/
            int status = SYSCALL(WAITIO, FLASHINT, dev_no, 0);
            enableInterrupts();

            SYSCALL(VERHOGEN, mutex_flash_sem, 0, 0);

            /*treat error as trap*/
            if(status != READY)
            {
                sptTrapHandler(sPtr, swap_pool_sem);
            }
                

    }

    
    int dev_no = sPtr->sup_asid - 1;
    device_t* flash_dev_reg = DEVREGBASE + (FLASHINT - 3) * DEVREGINTSCALE + dev_no * DEVREGDEVSCALE;  /*backing store of current process*/
    int* mutex_flash_sem = &mutex_device_sem[(FLASHINT - 3) * DEVPERINT + dev_no];

    /*debug(sPtr->sup_asid, replaced_PFN, frame_index, (FLASHINT - 3) * DEVPERINT + (dev_no));*/


    SYSCALL(PASSERN, mutex_flash_sem, 0, 0);
    flash_dev_reg->d_data0 = replaced_PFN; /*addr to read backing store content into (frame i in swap pool)*/

    disableInterrupts();
    flash_dev_reg->d_command = ALLOFF | ((missing_VPN % PAGETABLESIZE) << SETCMDBLKNO) | READBLK;   /*read, block is logical addr of missing page number (denoted p)*/
    int status = SYSCALL(WAITIO, FLASHINT, dev_no, 0);
    enableInterrupts();   /*atomic execution - enable interrupt*/

    debug(sPtr->sup_asid, flash_dev_reg->d_command, ALLOFF | ((missing_VPN % PAGETABLESIZE) << SETCMDBLKNO) | READBLK, flash_dev_reg->d_data0);
    SYSCALL(VERHOGEN, mutex_flash_sem, 0, 0);

    /*treat error as trap*/
    if(status != READY)
    {
        sptTrapHandler(sPtr, swap_pool_sem);
    }
        

    swap_pool_table[frame_index].asid = sPtr->sup_asid;     /*the frame now belongs to the current process*/
    swap_pool_table[frame_index].vpn = missing_VPN;    /*update VPN*/
    swap_pool_table[frame_index].matching_pte_ptr = &(sPtr->sup_privatePgTbl[missing_VPN % PAGETABLESIZE]);  /*page table entry whose VPN is VPN_missing*/
    
    disableInterrupts();  /*atomic execution - disable interrupt*/
    (swap_pool_table[frame_index].matching_pte_ptr)->EntryLo = ALLOFF | (replaced_PFN) | V | D; /*no PFN shift here*/

    /*
    setENTRYHI((*(swap_pool_table[frame_index].matching_pte_ptr)).EntryHi);
    setENTRYLO((*(swap_pool_table[frame_index].matching_pte_ptr)).EntryLo);
    TLBWI();
    */
    TLBCLR();
    enableInterrupts();   /*atomic execution - enable interrupt*/

    SYSCALL(VERHOGEN, &swap_pool_sem, 0, 0);

    LDST(&(sPtr->sup_exceptState[PGFAULTEXCEPT]));
}