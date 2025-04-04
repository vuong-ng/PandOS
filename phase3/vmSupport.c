#include "vmSupport.h"


void initSwapStructs()
{
    /*The Swap Pool table and Swap Pool semaphore*/
    int i = 0;
    for(i = 0; i < UPROCMAX * 2; i++)  
    {
        swap_pool_table[i].asid = -1;
        swap_pool_table[i].vpn = 0;
        swap_pool_table[i].matching_pte_ptr = NULL;
    } 
        
    swap_pool_sem = 1;
}



void pager()
{

    support_t* sPtr = SYSCALL(GETSPTPTR, 0, 0, 0);  /*pointer to the Current Process’s Support Structure*/
    unsigned int TLB_excp_cause = sPtr->sup_exceptState[PGFAULTEXCEPT].s_cause;
    int cause_exccode = (TLB_excp_cause >> GETEXCCODE) & CLEAR26MSB;

    if (cause_exccode == 1) /*TLB-Modification Exception*/
    {
        ;  /*program trap*/
    }

    SYSCALL(PASSERN, &swap_pool_sem, 0, 0);

    unsigned int VPN_missing = (sPtr->sup_exceptState[PGFAULTEXCEPT].s_entryHI >> PFN);

    static int page_replacement = 0;
    int frame_index = page_replacement % (UPROCMAX * 2);     /*frame in swap pool to load data from backing store into*/
    page_replacement += 1;

    memaddr frame = SWAPPOOLADDR + frame_index;     /*the actual physical frame in RAM*/

    pte_t* matching_pte = swap_pool_table[frame_index].matching_pte_ptr;
    if(swap_pool_table[frame_index].asid != -1)  /*entry in swap pool occupied*/
    {
        if ( ((*matching_pte).EntryLo & ~D) >> 10 == 1)   /*if dirty*/
        {
            setSTATUS(getSTATUS() & ~TEBITON);  /*atomic execution - disable interrupt*/
            (*matching_pte).EntryLo &= ~V;  /*mark PTE invalid*/
            setSTATUS(getSTATUS() | TEBITON);   /*atomic execution - enable interrupt*/


            setSTATUS(getSTATUS() & ~TEBITON);  /*atomic execution - disable interrupt*/
            /*update TLB as needed (how to determine if entry is cached in TLB)*/
            /*
            TLBP();
            setENTRYHI( (*(swap_pool_table[frame_index].matching_pte_ptr)).EntryHi);
            setENTRYLO( (*(swap_pool_table[frame_index].matching_pte_ptr)).EntryLo );
            TLBWI();
            */
            TLBCLR();
            setSTATUS(getSTATUS() | TEBITON);   /*atomic execution - enable interrupt*/

            
            
            int dev_no = swap_pool_table[frame_index].asid;                                                        /*process's flash determined by its asid*/
            device_t* flash_dev_reg = DEVREGBASE + (FLASHINT - 3) * DEVREGINTSCALE + dev_no * DEVREGDEVSCALE;
            flash_dev_reg->d_data0 = (*matching_pte).EntryLo >> PFN;        /*PFN in RAM to be written to backing store*/
            
            flash_dev_reg->d_command = (ALLOFF | swap_pool_table[frame_index].vpn) << 8 | WRITEBLK    ;   /*write, block is vpn occupying the swap entry*/
            SYSCALL(WAITIO, FLASHINT, dev_no, 0);


        }
    }

    
    
    int dev_no = sPtr->sup_asid;
    device_t* flash_dev_reg = DEVREGBASE + (FLASHINT - 3) * DEVREGINTSCALE + dev_no * DEVREGDEVSCALE;  /*backing store of current process*/
    (*flash_dev_reg).d_data0 = &frame; /*addr to read backing store content into (frame i)*/

    setSTATUS(getSTATUS() & ~TEBITON);
    (*flash_dev_reg).d_command = (ALLOFF | VPN_missing) << 8 | READBLK;   /*read, block is logical addr of missing page number (denoted p)*/
    SYSCALL(WAITIO, FLASHINT, dev_no, 0);
    setSTATUS(getSTATUS() | TEBITON);   /*atomic execution - enable interrupt*/

    swap_pool_table[frame_index].asid = sPtr->sup_asid;     /*the frame now belongs to the current process*/
    swap_pool_table[frame_index].vpn = VPN_missing;    /*update VPN*/

    swap_pool_table[frame_index].matching_pte_ptr = &(sPtr->sup_privatePgTbl[VPN_missing - VPNSHIFT]);  /*page table entry whose VPN is VPN_missing*/

    
    setSTATUS(getSTATUS() & ~TEBITON);  /*atomic execution - disable interrupt*/
    (*(swap_pool_table[frame_index].matching_pte_ptr)).EntryLo |= V;
    (*(swap_pool_table[frame_index].matching_pte_ptr)).EntryHi &= 0xFFF;  /*reset PFN bits*/
    (*(swap_pool_table[frame_index].matching_pte_ptr)).EntryHi |= frame;

    /*
    setENTRYHI((*(swap_pool_table[frame_index].matching_pte_ptr)).EntryHi);
    setENTRYLO((*(swap_pool_table[frame_index].matching_pte_ptr)).EntryLo);
    TLBWI();
    */
    TLBCLR();
    setSTATUS(getSTATUS() | TEBITON);   /*atomic execution - enable interrupt*/


    SYSCALL(VERHOGEN, &swap_pool_sem, 0, 0);

    LDST((state_t*) BIOSDATAPAGE);




}