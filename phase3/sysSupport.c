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

void sptTrapHandler(support_t* sPtr, int* swap_pool_sem)
{
   terminate(sPtr, swap_pool_sem);

}

void terminate(support_t* sPtr, int* swap_pool_sem)
{
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
    sPtr->sup_exceptState[GENERALEXCEPT].s_pc += 4;  /*increase PC*/

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
            int mutex_printer_sem = mutex_device_sem[(PRNTINT - 3) * DEVPERINT + dev_no];


            SYSCALL(PASSERN, &mutex_printer_sem, 0, 0);
            /*print each character*/
            int error, printed = 0;
            while(printed < str_length && !error)
            {
                printer_dev_reg->d_data0 = *str_VA;

                disableInterrupts();
                printer_dev_reg->d_command = PRINTCHR;
                SYSCALL(WAITIO, PRNTINT, dev_no, 0);
                enableInterrupts();

                if((printer_dev_reg->d_status) & 0xFF != READY)
                {
                    error = 1;
                    printed = -((printer_dev_reg->d_status) & 0xFF);     /*negative of device's status*/
                }
                    
                else
                {
                    printed++;
                    str_VA++;   /*move 1 byte to next character*/
                }
                
            }
            SYSCALL(VERHOGEN, &mutex_printer_sem, 0, 0);

            sPtr->sup_exceptState[GENERALEXCEPT].s_v0 = printed;
            break;

        }

        case WRITETERMINAL:
        {
            char* str_VA = sPtr->sup_exceptState[GENERALEXCEPT].s_a1;
            int str_length = sPtr->sup_exceptState[GENERALEXCEPT].s_a2;

             /*address must lie in logical addr space*/
            if (str_length <= 0 || str_length > 128 || str_VA < KUSEG)    
                terminate(sPtr, NULL);

            int dev_no = sPtr->sup_asid - 1;     /*which printer determined by the process id*/
            device_t* termwrite_dev_reg = DEVREGBASE + (TERMINT - 3) * DEVREGINTSCALE + dev_no * DEVREGDEVSCALE;
            int mutex_termwrite_sem = mutex_device_sem[(TERMINT - 3) * DEVPERINT + dev_no * SUBDEVPERTERM + TERMWRITE];

            /*print each character*/

            SYSCALL(PASSERN, &mutex_termwrite_sem, 0, 0);
            int written, error = 0;
            while(written < str_length && !error)
            {
                disableInterrupts();
                termwrite_dev_reg->t_transm_command = (*str_VA) << 8;
                termwrite_dev_reg->t_transm_command |= TRANSMITCHAR;
                SYSCALL(WAITIO, TERMINT, dev_no, TERMWRITE);
                enableInterrupts();

                if((termwrite_dev_reg->t_transm_status) & 0xFF != CHARTRANSMITTED)
                {
                    error = 1;
                    written = -((termwrite_dev_reg->t_recv_status) & 0xFF);  /*negative of device's status*/
                }
                    
                else
                {
                    written++;
                    str_VA++;   /*move 1 byte to next character*/
                }
                
            }
            SYSCALL(VERHOGEN, &mutex_termwrite_sem, 0, 0);
            
            sPtr->sup_exceptState[GENERALEXCEPT].s_v0 = written;
            break;
        }
        case READTERMINAL:
        {
            char* str_addr = sPtr->sup_exceptState[GENERALEXCEPT].s_a1;

             /*address must lie in logical addr space*/
            if (str_addr < KUSEG)    
                SYSCALL(TERMINATE, 0, 0, 0);

            int dev_no = sPtr->sup_asid - 1;     /*which printer determined by the process id*/
            device_t* termread_dev_reg = DEVREGBASE + (TERMINT - 3) * DEVREGINTSCALE + dev_no * DEVREGDEVSCALE;
            int mutex_termread_sem = mutex_device_sem[(TERMINT - 3) * DEVPERINT + dev_no * SUBDEVPERTERM + TERMREAD];

            /*print each character*/

            SYSCALL(PASSERN, &mutex_termread_sem, 0, 0);
            int read, done, error = 0;
            while (!done && !error)                       
            {
                disableInterrupts();
                termread_dev_reg->t_recv_command = ALLOFF | RECEIVECHAR;
                SYSCALL(WAITIO, TERMINT, dev_no, TERMREAD);
                enableInterrupts();

                if((termread_dev_reg->t_recv_status) & 0xFF != CHARRECEIVED)
                {
                    error = 1;
                    read = -((termread_dev_reg->t_recv_status) & 0xFF);  /*negative of device's status*/
                }
                    
                else
                {
                    read++;
                    *str_addr = (termread_dev_reg->t_recv_status) >> 8;
                    if(*str_addr == 0x0A)
                        done = 1;
                    str_addr++;   /*move 1 byte to next character*/
                }
            }
            SYSCALL(VERHOGEN, &mutex_termread_sem, 0, 0);

            sPtr->sup_exceptState[GENERALEXCEPT].s_v0 = read;
            break;
        }
    }

    /*return to current process*/
    LDST(&(sPtr->sup_exceptState[GENERALEXCEPT]));

}