#include "../h/sysSupport.h"

void sptGeneralExceptionHandler()
{
    support_t* sPtr = SYSCALL(GETSPTPTR, 0, 0, 0);

    unsigned int cause = sPtr->sup_exceptState[GENERALEXCEPT].s_cause;

    switch ((cause >> GETEXCCODE) & CLEAR26MSB)
    {
        case 8:
        {
            sptSyscallHandler(sPtr);
            break;
        }
        default:
        {
            sptTrapHandler(sPtr, NULL);
            break;
        }
    }
}

void terminate(support_t* sPtr, int* swap_pool_sem)
{
    SYSCALL(VERHOGEN, &masterSemaphore, 0, 0);

    if(swap_pool_sem != NULL)
        SYSCALL(VERHOGEN, swap_pool_sem, 0, 0);

    /*check if any of the to-be-terminated process's mutex is held (using asid)*/
    int dev_no = sPtr->sup_asid - 1;
    int i = 0, all_mutexes_checked = 0;
    while (!all_mutexes_checked)
    {
        if(i != (TERMINT - 3) * DEVPERINT)
        {
            if(mutex_device_sem[i + dev_no] <= 0)  /*if device line i mutex of process is held*/
                SYSCALL(VERHOGEN, &mutex_device_sem[i + dev_no], 0, 0);
                
            i += DEVPERINT;
        }
        else
        {
            if(mutex_device_sem[i + dev_no * SUBDEVPERTERM + TERMWRITE] <= 0)
                SYSCALL(VERHOGEN, &mutex_device_sem[i + dev_no * SUBDEVPERTERM + TERMWRITE], 0, 0);
                
            if(mutex_device_sem[i + dev_no * SUBDEVPERTERM + TERMREAD] <= 0)
                SYSCALL(VERHOGEN, &mutex_device_sem[i + dev_no * SUBDEVPERTERM + TERMREAD], 0, 0);

            all_mutexes_checked = 1;
        }
    }

    SYSCALL(TERMINATETHREAD, 0, 0, 0);
}

void sptTrapHandler(support_t* sPtr, int* swap_pool_sem)
{
    terminate(sPtr, swap_pool_sem);
}

int printerOperation(device_t* dev_reg, unsigned int dev_no, unsigned int data0, unsigned int command)
{
    dev_reg->d_data0 = data0;
    disableInterrupts();
    dev_reg->d_command = command;
    int status = SYSCALL(WAITIO, PRNTINT, dev_no, 0);
    enableInterrupts();
    return status;
}

int terminalWrite(device_t* dev_reg, unsigned int dev_no, char* str_VA)
{
    disableInterrupts();
    dev_reg->t_transm_command = ((*str_VA) << SETCMDBLKNO) | TRANSMITCHAR;
    int status = SYSCALL(WAITIO, TERMINT, dev_no, TERMWRITE);
    enableInterrupts();
    return status;
}

int terminalRead(device_t* dev_reg, unsigned int dev_no)
{
    disableInterrupts();
    dev_reg->t_recv_command = RECEIVECHAR;
    int status = SYSCALL(WAITIO, TERMINT, dev_no, TERMREAD);
    enableInterrupts();
    return status;
}

void sptSyscallHandler(support_t* sPtr)
{
    int a0 = sPtr->sup_exceptState[GENERALEXCEPT].s_a0;
    switch(a0)
    {
        case TERMINATE:
        {
            /*check if any of the to-be-terminated process's mutex is held (using asid)*/
            terminate(sPtr, NULL);
            break;
        }
        case GET_TOD:
        {
            cpu_t tod;
            STCK(tod);
            sPtr->sup_exceptState[GENERALEXCEPT].s_v0 = tod;
            break;
        }

        /*printer*/
        case WRITEPRINTER:
        {
            char* str_VA = sPtr->sup_exceptState[GENERALEXCEPT].s_a1;
            int str_length = sPtr->sup_exceptState[GENERALEXCEPT].s_a2;
            if (str_length < 0 || str_length > MAXSTRLEN || str_VA < KUSEG)    
                terminate(sPtr, NULL);
            int dev_no = sPtr->sup_asid - 1;     
            device_t* printer_dev_reg = DEVREGBASE + (PRNTINT - 3) * DEVREGINTSCALE + dev_no * DEVREGDEVSCALE;
            int* mutex_printer_sem = &mutex_device_sem[(PRNTINT - 3) * DEVPERINT + dev_no];
            SYSCALL(PASSERN, mutex_printer_sem, 0, 0);
            int error = 0, printed = 0;
            while((printed < str_length) && (error == 0))
            {
                int status = printerOperation(printer_dev_reg, dev_no, *str_VA, PRINTCHR);
                if(status != READY)
                {
                    error = 1;
                    printed = -status;    
                }
                else
                {
                    printed++;
                    str_VA++;   
                }
            }
            SYSCALL(VERHOGEN, mutex_printer_sem, 0, 0);
            sPtr->sup_exceptState[GENERALEXCEPT].s_v0 = printed;
            break;
        }

        case WRITETERMINAL:
        {
            char* str_VA = sPtr->sup_exceptState[GENERALEXCEPT].s_a1;
            int str_length = sPtr->sup_exceptState[GENERALEXCEPT].s_a2;
            /*address must lie in logical addr space*/
            if (str_length < 0 || str_length > MAXSTRLEN || str_VA < KUSEG)    
                terminate(sPtr, NULL);

            int dev_no = sPtr->sup_asid - 1;     /*which terminal determined by the process id*/

            int* mutex_termwrite_sem = &(mutex_device_sem[(TERMINT - 3) * DEVPERINT + dev_no * SUBDEVPERTERM + TERMWRITE]);
            SYSCALL(PASSERN, mutex_termwrite_sem, 0, 0);

            device_t* termwrite_dev_reg = DEVREGBASE + (TERMINT - 3) * DEVREGINTSCALE + dev_no * DEVREGDEVSCALE;    
            int written = 0, error = 0, status = 0;
            while((written < str_length) && (error == 0))
            {
                status = terminalWrite(termwrite_dev_reg, dev_no, str_VA);
                if((status & TERMSTATMASK) != CHARTRANSMITTED)
                {
                    error = 1;
                    written = 0 - (status & TERMSTATMASK);  /*negative of device's status*/
                }
                else
                {
                    written++;
                    str_VA++;   /*move 1 byte to next character*/
                }
            }
            SYSCALL(VERHOGEN, mutex_termwrite_sem, 0, 0);
            sPtr->sup_exceptState[GENERALEXCEPT].s_v0 = written;
            break;
        }
        case READTERMINAL:
        {
            char* str_addr = sPtr->sup_exceptState[GENERALEXCEPT].s_a1;
            if (str_addr < KUSEG)    
                terminate(sPtr, NULL);

            int dev_no = sPtr->sup_asid - 1; 
            int* mutex_termread_sem = &mutex_device_sem[(TERMINT - 3) * DEVPERINT + dev_no * SUBDEVPERTERM + TERMREAD];
            SYSCALL(PASSERN, mutex_termread_sem, 0, 0);
            
            device_t* termread_dev_reg = DEVREGBASE + (TERMINT - 3) * DEVREGINTSCALE + dev_no * DEVREGDEVSCALE;
            int read = 0, done = 0, error = 0, status = 0;
            while (done == 0 && error == 0)                       
            {
                status = terminalRead(termread_dev_reg, dev_no);
                if((status & TERMSTATMASK) != CHARRECEIVED)
                {
                    error = 1;
                    read = 0 - (status & TERMSTATMASK);  /*negative of device's status*/
                }
                else
                {
                    if ((status >> GETCHAR) == NEWLINE) 
                        done = 1;
                    else
                    {
                        *str_addr = (status >> GETCHAR);  
                        str_addr++;
                    }
                    read++;
                }
            }
            SYSCALL(VERHOGEN, mutex_termread_sem, 0, 0);
            sPtr->sup_exceptState[GENERALEXCEPT].s_v0 = read;
            break;
        }
    }

    sPtr->sup_exceptState[GENERALEXCEPT].s_pc += 4;  /*increase PC*/
    LDST((state_t*) (&(sPtr->sup_exceptState[GENERALEXCEPT])));     /*return to current process*/
}