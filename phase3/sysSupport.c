/******************************** SYSSUPPORT.C *********************************
* Written by Long Pham & Vuong Nguyen
* Module: General Syscall Support Level for Pandos OS Phase 3
* 
* Components:
* - Exception Handling: Manages general exceptions and syscalls in Support level
* - System Call Handler: Implements syscall from 9 to 13
* - Program Trap Handler: Handle program trap
*
* Core Functions:
* - sptGeneralExceptionHandler(): Routes exceptions to appropriate handlers
* - sptSyscallHandler(): Processes system calls from user processes
* - sptTrapHandler(): Handles program traps and errors
*
* Global State:
* - masterSemaphore: Master semaphore for phase 3 to grateful terminate test()
*
* Exception Types Handled:
* - System calls (cause code 8)
* - Program traps (other cause codes)
***************************************************************************/

#include "../h/sysSupport.h"

/*********************************************************
* Purpose: Routes exceptions to appropriate handlers based on cause
* Parameters: None
* Returns: None
* Logic:
* 1. Gets current process's support structure via SYS8
* 2. Extracts exception cause code
* 3. Routes to:
*    - sptSyscallHandler for syscalls (cause code Exc.Code 8)
*    - sptTrapHandler for all other exceptions
*********************************************************/
void sptGeneralExceptionHandler()
{
    /*gain access to the support structure of the current process by calling SYS8*/
    support_t* sPtr = SYSCALL(GETSPTPTR, 0, 0, 0);

    /*get the cause of the general exception in the saved exception state*/
    unsigned int cause = sPtr->sup_exceptState[GENERALEXCEPT].s_cause;

    switch ((cause >> GETEXCCODE) & CLEAR26MSB)
    {
        case 8:
        {
            /*if the cause code Exc.Code is 8, the exception is a syscall*/
            sptSyscallHandler(sPtr);
            break;
        }
        default:
        {
            /*otherwise, call program trap on the process*/
            sptTrapHandler(sPtr, NULL);
            break;
        }
    }
}

/*********************************************************
* Purpose: Performs V on mastersemaphores and terminates user process
* Parameters:
* - sPtr: Pointer to current process's support structure
* - swap_pool_sem: Pointer to swap pool semaphore
* Returns: None
* Core Logic:
* 1. Releases master semaphore
* 2. Releases swap pool semaphore if held
* 3. Checks and releases device mutexes of the current uproc:
*    - Standard devices: releases if held
*    - Terminal devices: releases both read/write if held
* 4. Calls TERMINATETHREAD syscall
*********************************************************/
void terminate(support_t* sPtr, int* swap_pool_sem)
{
    /* V the swap pool semaphore before terminate the Uproc*/
    SYSCALL(VERHOGEN, &masterSemaphore, 0, 0);

    /*if the Uproc is currently holding the swap pool mutex sempahore, V the swap pool semaphore*/
    if(swap_pool_sem != NULL)
        SYSCALL(VERHOGEN, swap_pool_sem, 0, 0);

    /*check if any of the terminating process's device mutex sempahore is held (using asid)*/
    /*get device number*/
    int dev_no = sPtr->sup_asid - 1;         
    int i = 0, all_mutexes_checked = 0; 

    /*examine all peripheral devices of the terminating Uproc, unblock blocked devices.*/        
    while (!all_mutexes_checked)
    {
        if(i != (TERMINT - 3) * DEVPERINT)
        {
            /* if the device is not terminal, only need check its only 1 subdevice mutex semaphore*/
            if(mutex_device_sem[i + dev_no] <= 0)  /*if device line i mutex of process is held*/
                SYSCALL(VERHOGEN, &mutex_device_sem[i + dev_no], 0, 0);
                
            i += DEVPERINT;
        }
        else
        {
            /*if the device is terminal, has to check both subdevices, read and write*/
            if(mutex_device_sem[i + dev_no * SUBDEVPERTERM + TERMWRITE] <= 0)
                SYSCALL(VERHOGEN, &mutex_device_sem[i + dev_no * SUBDEVPERTERM + TERMWRITE], 0, 0);
                
            if(mutex_device_sem[i + dev_no * SUBDEVPERTERM + TERMREAD] <= 0)
                SYSCALL(VERHOGEN, &mutex_device_sem[i + dev_no * SUBDEVPERTERM + TERMREAD], 0, 0);

            /*mark that all devices have been checked since terminal is the last device*/
            all_mutexes_checked = 1;
        }
    }

    /*terminate the uproc by calling syscall 2*/
    SYSCALL(TERMINATETHREAD, 0, 0, 0);
}

/*********************************************************
* Purpose: Handles program traps by terminating process
* Parameters:
* - sPtr: Pointer to process's support structure
* - swap_pool_sem: Pointer to swap pool semaphore
* Returns: None
* Core Logic:
* 1. Calls terminate() with:
*    - Current process support structure
*    - Swap pool semaphore if process holds it
* Side Effects:
* - Terminates current process
* - Releases held semaphores
*********************************************************/
void sptTrapHandler(support_t* sPtr, int* swap_pool_sem)
{
    /* call sys9 in program trap*/
    terminate(sPtr, swap_pool_sem);
}

/*********************************************************
* Purpose: Performs write operations to printer
* Parameters:
* - dev_reg: Pointer to device registers
* - dev_no: Device number to operate on
* - data0: Character data to print
* - command: Printer operation command 
* Returns: int - Status of printer operation
* Core Logic:
* 1. Writes character to device data0 register
* 2. Atomically:
*    - Sets command register to print char command
*    - Call sys5 waiting for I/O operation
* 3. Re-enables interrupts
*********************************************************/
int printerOperation(device_t* dev_reg, unsigned int dev_no, unsigned int data0, unsigned int command)
{
    /* write each character in the data0 field */
    dev_reg->d_data0 = data0;

    /*  disable interrupt to write command field and issue syscall 5 atomically */
    disableInterrupts();

    /*  write command field with print command*/
    dev_reg->d_command = command;
    int status = SYSCALL(WAITIO, PRNTINT, dev_no, 0);
    enableInterrupts();
    return status;
}

/*********************************************************
* Purpose: Performs write operation to terminal device
* Parameters:
* - dev_reg: Pointer to device registers
* - dev_no: Terminal device number
* - *str_VA: character to write to terminal
* Returns: int - Status of write operation
* Core Logic:
* 1. Disables interrupts for atomic operation
* 2. Sets command field to be transmit command (lower-ordered bits) with character (higher-ordered bits)
* 3. Waits for I/O completion by calling syscall 5
* 4. Re-enables interrupts
*********************************************************/
int terminalWrite(device_t* dev_reg, unsigned int dev_no, char* str_VA)
{
    /*  disable interrupt to write command field and issue syscall 5 atomically */
    disableInterrupts();

    /*  write command field with transmit command and the character */
    dev_reg->t_transm_command = ((*str_VA) << SETCMDBLKNO) | TRANSMITCHAR;
    int status = SYSCALL(WAITIO, TERMINT, dev_no, TERMWRITE);

    enableInterrupts();
    return status;
}

/*********************************************************
* Purpose: Performs read operation from terminal device
* Parameters:
* - dev_reg: Pointer to device registers
* - dev_no: Terminal device number
* Returns: int - Status of read operation
* Core Logic:
* 1. Disables interrupts for atomic operation
* 2. Sets terminal receive command into command field
* 3. Waits for I/O completion by calling sys5
* 4. Re-enables interrupts
*********************************************************/
int terminalRead(device_t* dev_reg, unsigned int dev_no)
{
    /*  disable interrupt to write command field and issue syscall 5 atomically */
    disableInterrupts();

    /*  write receive code into the command field of the terminal */
    dev_reg->t_recv_command = RECEIVECHAR;
    int status = SYSCALL(WAITIO, TERMINT, dev_no, TERMREAD);
    enableInterrupts();
    return status;
}

/*********************************************************
* Purpose: Handles Support level syscalls (9-13)
* Parameters:
* - sPtr: Pointer to current process's support structure
* Returns: void
* 
* Syscalls Handled:
* - TERMINATE (9): Terminate process
* - GET_TOD (10): Get time of day
* - WRITEPRINTER (11): Write to printer device
* - WRITETERMINAL (12): Write to terminal
* - READTERMINAL (13): Read from terminal
*
* 1. Gets syscall number from a0
* 2. For each syscall:
*   TERMINATE
*    - Performs cleanup via terminate() helper
*    - Releases all held resources
*    - Terminates process
*
*   GET_TOD:
*    - Gets current CPU time via STCK
*    - Returns time value in v0 register
*
*   READ/WRITE from/to devices syscalls
*    - Get virtual address of the string from a1, 
*          or (optionally) string length from a2
*    - Acquires needed device mutexes
*    - Performs device I/O
*    - Return count of successfully transmitted/read characters
*       of device operation to v0 of succeed, 
*       else return negative of status
* 3. Increase PC and loads saved exception state
*
* Error Handling:
* - Invalid parameters -> process termination
* - Device errors -> negative status returned
*********************************************************/
void sptSyscallHandler(support_t* sPtr)
{
    /*get current process's support struct by calling syscall 8*/
    int a0 = sPtr->sup_exceptState[GENERALEXCEPT].s_a0;
    switch(a0)
    {
        case TERMINATE:
        {
            terminate(sPtr, NULL);
            break;
        }
        case GET_TOD:
        {
            cpu_t tod;
            STCK(tod);

            /*call macro function to calculate time of day and return in v0 of exception state*/
            sPtr->sup_exceptState[GENERALEXCEPT].s_v0 = tod;
            break;
        }

        /*printer*/
        case WRITEPRINTER:
        {
            /*get the virtual address of the first character of the string to be transmitted in a1*/
            char* str_VA = sPtr->sup_exceptState[GENERALEXCEPT].s_a1;

            /* get the length of the string in a2*/
            int str_length = sPtr->sup_exceptState[GENERALEXCEPT].s_a2;

            /* terminate if the length of string is less than 0 or larger than 128, or if the address is outside of the requesting U-proc’s logical address space*/
            if (str_length < 0 || str_length > MAXSTRLEN || str_VA < KUSEG)    
                terminate(sPtr, NULL);

            /*Device number is determined by the process id*/
            int dev_no = sPtr->sup_asid - 1;     

            /*get the register for the printer*/
            device_t* printer_dev_reg = DEVREGBASE + (PRNTINT - 3) * DEVREGINTSCALE + dev_no * DEVREGDEVSCALE;

            /*get mutex semaphore of printer*/
            int* mutex_printer_sem = &mutex_device_sem[(PRNTINT - 3) * DEVPERINT + dev_no];
            SYSCALL(PASSERN, mutex_printer_sem, 0, 0);
            int error = 0, printed = 0;

            while((printed < str_length) && (error == 0))
            /*read the string while printed string is less than the string length and the printer status after each read is READY */
            {
                int status = printerOperation(printer_dev_reg, dev_no, *str_VA, PRINTCHR);
                if(status != READY)
                {
                    /*if the operation ends with a status other than “Device Ready”, return negative value of status*/
                    error = 1;
                    printed = -status;    
                }
                else
                {
                    /*else, continue to read the string and count the number of characters that are transmitted successfully*/
                    printed++;
                    str_VA++;   
                }
            }
            SYSCALL(VERHOGEN, mutex_printer_sem, 0, 0);

            /*return the count of transmitted characters or the negative of status in v0*/
            sPtr->sup_exceptState[GENERALEXCEPT].s_v0 = printed;
            break;
        }

        case WRITETERMINAL:
        {
            /* get the virtual address of the first character of the string to be transmitted in a1*/
            char* str_VA = sPtr->sup_exceptState[GENERALEXCEPT].s_a1;

            /*get the length of the string in a2*/
            int str_length = sPtr->sup_exceptState[GENERALEXCEPT].s_a2;

            /* terminate if the length of string is less than 0 or larger than 128, or if the address is outside of the requesting U-proc’s logical address space*/
            if (str_length < 0 || str_length > MAXSTRLEN || str_VA < KUSEG)    
                terminate(sPtr, NULL);
            
            /*Device number is determined by the process id */
            int dev_no = sPtr->sup_asid - 1;    

            /*get the mutex semaphore of the terminal write sub-device*/
            int* mutex_termwrite_sem = &(mutex_device_sem[(TERMINT - 3) * DEVPERINT + dev_no * SUBDEVPERTERM + TERMWRITE]);
            SYSCALL(PASSERN, mutex_termwrite_sem, 0, 0);

            /*get the register of the terminal*/
            device_t* termwrite_dev_reg = DEVREGBASE + (TERMINT - 3) * DEVREGINTSCALE + dev_no * DEVREGDEVSCALE;    
            int written = 0, error = 0, status = 0;
            
            /*write the string while transmitted string is less than the string length and the status after each read is "Character Transmitted" */
            while((written < str_length) && (error == 0))
            {
                status = terminalWrite(termwrite_dev_reg, dev_no, str_VA);
                if((status & TERMSTATMASK) != CHARTRANSMITTED)
                {
                    error = 1;
                    /* if status is not "Character Transmitted, return negative of device's status*/
                    written = 0 - (status & TERMSTATMASK);  
                }
                else
                {
                    written++;
                    /*move to next character*/
                    str_VA++;  
                }
            }
            SYSCALL(VERHOGEN, mutex_termwrite_sem, 0, 0);

            /*return the count of transmitted characters or the negative of status in v0*/
            sPtr->sup_exceptState[GENERALEXCEPT].s_v0 = written;
            break;
        }
        case READTERMINAL:
        {
            /* get the virtual address of a string buffer where the data read should be placed in a1*/
            char* str_addr = sPtr->sup_exceptState[GENERALEXCEPT].s_a1;

            /*terminate if the address of the string is less than the logical address space of the u-proc*/
            if (str_addr < KUSEG)    
                terminate(sPtr, NULL);
            
            /* device number is determined by the process id */
            int dev_no = sPtr->sup_asid - 1; 

            /*get mutex semaphore of terminal read sub-device*/
            int* mutex_termread_sem = &mutex_device_sem[(TERMINT - 3) * DEVPERINT + dev_no * SUBDEVPERTERM + TERMREAD];
            SYSCALL(PASSERN, mutex_termread_sem, 0, 0);
            
            /*get register of the terminal*/
            device_t* termread_dev_reg = DEVREGBASE + (TERMINT - 3) * DEVREGINTSCALE + dev_no * DEVREGDEVSCALE;
            int read = 0, done = 0, error = 0, status = 0;

            /*read the character in the string while the received character is not \n and there is no error in terminal receiving status*/
            while (done == 0 && error == 0)                       
            {
                status = terminalRead(termread_dev_reg, dev_no);
                if((status & TERMSTATMASK) != CHARRECEIVED)
                {
                    /*if the terminal receiving status is not "Character Received", return the negative of terminal read sub-device status*/
                    error = 1;
                    read = 0 - (status & TERMSTATMASK);  /*negative of device's status*/
                }
                else
                {
                    /*if reach '\n' character, mark the process as done*/
                    if ((status >> GETCHAR) == NEWLINE) 
                        done = 1;
                    else
                    {
                        /*write the received character to the address where the data should be placed*/
                        *str_addr = (status >> GETCHAR);  
                        str_addr++;
                    }
                    read++;
                }
            }
            SYSCALL(VERHOGEN, mutex_termread_sem, 0, 0);

            /*return the count of received characters or the negative of terminal read sub-device status in v0*/
            sPtr->sup_exceptState[GENERALEXCEPT].s_v0 = read;
            break;
        }
        case DELAY:
        {
            delay(sPtr);
            break;
        }
        default:
        {
            sptTrapHandler(sPtr, NULL);
        }

    }

    sPtr->sup_exceptState[GENERALEXCEPT].s_pc += 4;                 /*increase PC*/
    LDST((state_t*) (&(sPtr->sup_exceptState[GENERALEXCEPT])));     /*return to current process*/
}