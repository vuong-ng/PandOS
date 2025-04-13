#include "../h/sysSupport.h"

void sptGeneralExceptionHandler()
{
    
    support_t* sPtr = SYSCALL(GETSPTPTR, 0, 0, 0);

    unsigned int cause = sPtr->sup_exceptState[GENERALEXCEPT].s_cause;
    debug(36, (cause >> GETEXCCODE) & CLEAR26MSB, sPtr->sup_exceptState[GENERALEXCEPT].s_a0, 1);

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

void sptTrapHandler(support_t* sPtr, int* swap_pool_sem)
{

   terminate(sPtr, swap_pool_sem);

}

void terminate(support_t* sPtr, int* swap_pool_sem)
{
    SYSCALL(VERHOGEN, &masterSemaphore, 0, 0);

    debug(sPtr->sup_asid,10,10,10);
    if(swap_pool_sem != NULL)
        SYSCALL(VERHOGEN, swap_pool_sem, 0, 0);

    /*check if any of the to-be-terminated process's mutex is held (using asid)*/
    int dev_no = sPtr->sup_asid - 1;
    int i, all_mutexes_checked = 0;
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
            sPtr->sup_exceptState[GENERALEXCEPT].s_v0 = SYSCALL(GETCPUTIME, 0, 0, 0);
            break;
        }

        /*printer*/
        case WRITEPRINTER:
        {
            char* str_VA = sPtr->sup_exceptState[GENERALEXCEPT].s_a1;
            int str_length = sPtr->sup_exceptState[GENERALEXCEPT].s_a2;

             /*address must lie in logical addr space*/
            if (str_length <= 0 || str_length > 128 || str_VA < KUSEG)    
                terminate(sPtr, NULL);

            int dev_no = sPtr->sup_asid - 1;     /*which printer determined by the process id*/
            device_t* printer_dev_reg = DEVREGBASE + (PRNTINT - 3) * DEVREGINTSCALE + dev_no * DEVREGDEVSCALE;
            int* mutex_printer_sem = &mutex_device_sem[(PRNTINT - 3) * DEVPERINT + dev_no];


            SYSCALL(PASSERN, mutex_printer_sem, 0, 0);
            /*print each character*/
            int error = 0, printed = 0;
            while((printed < str_length) && (error == 0))
            {
                printer_dev_reg->d_data0 = (unsigned int) (*str_VA);

                disableInterrupts();
                printer_dev_reg->d_command = PRINTCHR;
                int status = SYSCALL(WAITIO, PRNTINT, dev_no, 0);
                enableInterrupts();

                if((status & 0xFF) != READY)
                {
                    error = 1;
                    printed = -(status & 0xFF);     /*negative of device's status*/
                }
                    
                else
                {
                    printed++;
                    str_VA++;   /*move 1 byte to next character*/
                }
                
            }
            SYSCALL(VERHOGEN, mutex_printer_sem, 0, 0);
            sPtr->sup_exceptState[GENERALEXCEPT].s_v0 = printed;
            break;

        }

        case WRITETERMINAL:
        {
            int dev_no = sPtr->sup_asid - 1;     /*which terminal determined by the process id*/
            int* mutex_termwrite_sem = &(mutex_device_sem[(TERMINT - 3) * DEVPERINT + dev_no * SUBDEVPERTERM + TERMWRITE]);
            
            char* str_VA = sPtr->sup_exceptState[GENERALEXCEPT].s_a1;
            int str_length = sPtr->sup_exceptState[GENERALEXCEPT].s_a2;
            int status;
            

            device_t* termwrite_dev_reg = DEVREGBASE + (TERMINT - 3) * DEVREGINTSCALE + dev_no * DEVREGDEVSCALE;
            

            /*address must lie in logical addr space*/
            if (str_length <= 0 || str_length > 128 || str_VA < KUSEG)    
                terminate(sPtr, NULL);

            
            
            int written = 0, error = 0;
            SYSCALL(PASSERN, mutex_termwrite_sem, 0, 0);
            while((written < str_length) && (error == 0))
            {
                
                disableInterrupts();
                termwrite_dev_reg->t_transm_command = ((*str_VA) << SETCMDBLKNO) | TRANSMITCHAR;
                status = SYSCALL(WAITIO, TERMINT, dev_no, TERMWRITE);
                enableInterrupts();
                if((status & TERMSTATMASK) != CHARTRANSMITTED)
                {
                    error = 1;
                    written = 0 - (status & 0xFF);  /*negative of device's status*/
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
            debug(1,1,1,13);
            int dev_no = sPtr->sup_asid - 1;     /*which printer determined by the process id*/

            int* mutex_termread_sem = &mutex_device_sem[(TERMINT - 3) * DEVPERINT + dev_no * SUBDEVPERTERM + TERMREAD];

            SYSCALL(PASSERN, mutex_termread_sem, 0, 0);
            char* str_addr = sPtr->sup_exceptState[GENERALEXCEPT].s_a1;

             /*address must lie in logical addr space*/
            if (str_addr < KUSEG)    
                terminate(sPtr, NULL);
            
            int status;
        
            device_t* termread_dev_reg = DEVREGBASE + (TERMINT - 3) * DEVREGINTSCALE + dev_no * DEVREGDEVSCALE;
            
            int read = 0, done = 0, error = 0;
            while (done == 0 && error == 0)                       
            {
                disableInterrupts();
                termread_dev_reg->t_recv_command = RECEIVECHAR;
                status = SYSCALL(WAITIO, TERMINT, dev_no, TERMREAD);
                enableInterrupts();

                if((status & 0xFF) != CHARRECEIVED)
                {
                    error = 1;
                    read = 0 - (status & 0xFF);  /*negative of device's status*/
                }
                else
                {
                    if ((status >> 8) == 0x0A) 
                        done = 1;
                    else
                    {
                        *str_addr = status >> 8;
                        str_addr++;
                    }
                    read++;
                }
                
            }
            

            sPtr->sup_exceptState[GENERALEXCEPT].s_v0 = read;
            SYSCALL(VERHOGEN, mutex_termread_sem, 0, 0);
            break;
        }
    }

    /*return to current process*/
    sPtr->sup_exceptState[GENERALEXCEPT].s_pc += 4;  /*increase PC*/
    LDST(&(sPtr->sup_exceptState[GENERALEXCEPT]));

}