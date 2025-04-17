/******************************** VMSUPPORT.C *********************************
* Written by Long Pham & Vuong Nguyen
* Module: Pager for Pandos OS Phase 3
* 
* Components:
* - Swap Pool Management:  swap pool semaphore: used to guarantee mutal exclusion to swap pool access 
*                          swap pool table : used to manage RAM frames
* - Page Replacement: implements page replacement algorithm (FIFO)
* - Flash Operations: Perform reading and writing from/to flash device
*
* Global State:
* - swap_pool_table[UPROCMAX * 2]: manage swap pool entries
* - swap_pool_sem: Semaphore guarantee mutual exclusion for swap pool access
*
* Core Functions:
* - initSwapStructs(): Initializes swap pool data structures
* - getVictim(): Implements FIFO page replacement algorithm
* - flashOperation(): Handles reading and writing from/to flash device
* - pager(): Handles TLB exception 
*
* Data Structures:
* - swap_frame_t: Entry for each swap pool frame containing:
*   * asid: Process identifier (-1 for unoccupied)
*   * vpn: Virtual page number
*   * matching_pte_ptr: Pointer to corresponding page table entry occupying the RAM frame
***************************************************************************/
#include "../h/vmSupport.h"

/*each entry per Swap Pool frame, recording information about the logical page occupying it*/
HIDDEN swap_frame_t swap_pool_table [UPROCMAX * 2]; 

/*initialize the swap pool semaphore*/
HIDDEN int swap_pool_sem;

/*********************************************************
* Purpose: Initializes swap pool data structures
* Parameters: None
* Returns: void
* Other operations: 
* - Initializes swap_pool_table entries with asid = -1
* - Sets swap_pool_sem to 1
*********************************************************/
void initSwapStructs()
{
    /*Intialize swap pool table and swap pool semaphore*/
    int i;
    for(i = 0; i < UPROCMAX * 2; i++)  
    {
        /*unoccupied swap pool entry has asid set to -1*/
        swap_pool_table[i].asid = -1;
        swap_pool_table[i].vpn = 0;
        swap_pool_table[i].matching_pte_ptr = NULL;
    } 
    /*initialize swap pool semaphore to 1*/
    swap_pool_sem = 1;
}

/*********************************************************
* Purpose: Implements FIFO page replacement algorithm
* Parameters: None
* Returns: int - Index of frame to be replaced
* Algorithm logic: Returns next frame index based on FIFO order
*********************************************************/
int getVictim()
{
    /*initialize static variable for indexing replaced frame*/
    static int victim = 0;          

    /*frame in swap pool to load data from backing store into*/
    int frame_index = victim % (UPROCMAX * 2);  
    victim++;
    return frame_index;
}


/*********************************************************
* Purpose: Performs read/write operations on flash device
* Parameters:
* - dev_no: Flash device number
* - data0: Starting physical address of 4K block to write in data0 field
* - blocknumber: Block number write into command field
* - command: Operation command (read/write)
* Returns: int - Status of the flash operation
* Core logic:
* - Acquires flash device mutex and device registers
* - Write the flash device’s DATA0 field with the appropriate starting physical address of the 4k block to be read (or written)
* - Write the flash device’s COMMAND field with the device block number (high order three bytes) 
*   and the command to read (or write) in the lower order byte.
* - Call syscall WAITIO
* - Releases device mutex
*********************************************************/
int flashOperation(unsigned int dev_no, unsigned int data0, unsigned int blocknumber, unsigned int command)
{
    /*get mutex semaphore for flash device*/
    int* mutex_flash_sem = &mutex_device_sem[(FLASHINT - 3) * DEVPERINT + dev_no];
    SYSCALL(PASSERN, mutex_flash_sem, 0, 0);

    /*get flash register*/
    device_t* flash_dev_reg = DEVREGBASE + (FLASHINT - 3) * DEVREGINTSCALE + dev_no * DEVREGDEVSCALE;

    /*write the flash device’s data0 field with the appropriate starting physical address of the 4k block to be read or written*/
    flash_dev_reg->d_data0 = data0;

    /*disable interrupt to write command field and call syscall 5 atomically*/
    disableInterrupts();

    /*write the flash device’s command field with the device block number and the command to read or write */
    flash_dev_reg->d_command = ALLOFF | (blocknumber << SETCMDBLKNO) | command;
    int status = SYSCALL(WAITIO, FLASHINT, dev_no, 0);
    enableInterrupts();

    SYSCALL(VERHOGEN, mutex_flash_sem, 0, 0);
    return status;
}

/*********************************************************
* Purpose: Handles page faults exceptions
* Parameters: None
* Returns: void 
* Other operations:
* - Updates TLB entries
* - Modifies page tables
* - Updates swap pool table
* - Performs flash I/O operations
* Core logic:
* 1. Handles TLB modification exceptions
* 2. Gets victim frame using FIFO
* 3. If victim frame occupied:
*    - Updates the TLB and the page table entry occupying the frame
*    - Update the frame content to flash device
* 4. Reads the content of missing page from flash into the chosen frame
* 5. Updates swap pool entry and page table entry
* 6. Loads back the saved exception state
*********************************************************/
void pager()
{
    /*get the Current Process’s Support Structure*/
    support_t* sPtr = SYSCALL(GETSPTPTR, 0, 0, 0);  
    /*get the cause from the saved exception state*/
    unsigned int TLB_excp_cause = sPtr->sup_exceptState[PGFAULTEXCEPT].s_cause;

    /*if the TLB-Modification Exception, call program trap*/
    if (((TLB_excp_cause >> GETEXCCODE) & CLEAR26MSB) == TLBMOD) 
        sptTrapHandler(sPtr, NULL); 

    SYSCALL(PASSERN, &swap_pool_sem, 0, 0);

    /*get missing VPN from saved page fault excpetion state*/
    unsigned int missing_VPN = (sPtr->sup_exceptState[PGFAULTEXCEPT].s_entryHI >> PAGESHIFT);

    /*frame in swap pool to load data from backing store into*/
    unsigned int frame_index = getVictim();     

    /*get address of the victim (replaced) frame*/
    memaddr replaced_PFN = SWAPPOOLADDR + (frame_index * PAGESIZE); 

    /*get pointer that RAM frame in swap pool table*/
    swap_frame_t* victim_frame = &(swap_pool_table[frame_index]);

    if(victim_frame->asid != UNOCCUPIED)  
    {
        /*if the picked frame is occupied, update the TLB, page table entry that occupies the frame, and the swap pool entry*/
        /*disable interrupt to update page table and TLB atomically*/
        disableInterrupts(); 

        /*mark page table entry as invalid*/
        (victim_frame->matching_pte_ptr)->EntryLo &= ~V;  

        /*update TLB*/
        TLBCLR();
        enableInterrupts();

        /* write the contents of chosen frame to the correct location on Uproc's flash device.*/
        int status = flashOperation((victim_frame->asid) - 1, replaced_PFN, (victim_frame->vpn) % PAGETABLESIZE, WRITEBLK);

        /*treat any error from the write flash device operation as program trap*/
        if(status != READY)
            sptTrapHandler(sPtr, swap_pool_sem);
    }

    /*Read the contents of the Current Process’s flash device missing logical page into the chosen frame*/
    int status = flashOperation(sPtr->sup_asid - 1, replaced_PFN, missing_VPN % PAGETABLESIZE, READBLK);

    /*treat error as trap*/
    if(status != READY)
        sptTrapHandler(sPtr, swap_pool_sem);
    
    /*update swap pool entry*/
    victim_frame->asid = sPtr->sup_asid;     /*the frame is now occupied by the current process*/
    victim_frame->vpn = missing_VPN;         /*update VPN for the frame*/

     /*update pointer to the page table entry of the missing VPN*/
    victim_frame->matching_pte_ptr = &(sPtr->sup_privatePgTbl[missing_VPN % PAGETABLESIZE]); 
    
    /* disable interrupt to update page table and TLB atomically*/
    disableInterrupts();
    /*update page table page frame number, turn on V and D bit*/
    (victim_frame->matching_pte_ptr)->EntryLo = ALLOFF | (replaced_PFN) | V | D; 

    /*update TLB*/
    TLBCLR();
    enableInterrupts();

    /*release the swap pool semaphore*/
    SYSCALL(VERHOGEN, &swap_pool_sem, 0, 0);
    LDST((state_t*) (&(sPtr->sup_exceptState[PGFAULTEXCEPT])));
}