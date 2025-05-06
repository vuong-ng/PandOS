#include "../h/deviceSupportDMA.h"

#define SETCYLNUM  8
#define SETHEADNUM 16
#define SETSECTNUM 8

#define MAXCYLSHIFT 16
#define MAXHEADSHIFT 8
#define MAXSECTMASK 0xFF


int diskOperation(unsigned int dev_no, unsigned int data0, unsigned int sect_no, unsigned int command)
{
    /*get mutex semaphore for disk device*/
    int* mutex_disk_sem = &mutex_device_sem[(DISKINT - 3) * DEVPERINT + dev_no];
    SYSCALL(PASSERN, mutex_disk_sem, 0, 0);

    device_t* disk_dev_reg = DEVREGBASE + (DISKINT - 3) * DEVREGINTSCALE + dev_no * DEVREGDEVSCALE;
    disk_dev_reg->d_data0 = data0;

    unsigned int maxcyl = (disk_dev_reg->d_data1) >> MAXCYLSHIFT;
    unsigned int maxhead = ((disk_dev_reg->d_data1) >> MAXHEADSHIFT) & MAXSECTMASK;
    unsigned int maxsect = (disk_dev_reg->d_data1) & MAXSECTMASK;
    
    disableInterrupts();
    if(command == SEEKCYL)  /*seek*/
    {
        unsigned int cylnum = sect_no / (maxhead * maxsect);
        disk_dev_reg->d_command = (ALLOFF | (cylnum << SETCYLNUM) | command);
    }
    else if (command == DISKREADBLK || command == DISKWRITEBLK)  /*read/write*/
    {
        unsigned int headnum = (sect_no % (maxhead * maxsect)) / maxsect;
        unsigned int sectnum = (sect_no % (maxhead * maxsect)) % maxsect;
        disk_dev_reg->d_command = (ALLOFF | (headnum << SETHEADNUM) | (sectnum << SETSECTNUM) | command);
    }
    int status = SYSCALL(WAITIO, DISKINT, dev_no, 0);
    enableInterrupts();

    SYSCALL(VERHOGEN, mutex_disk_sem, 0, 0);

    return status;
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