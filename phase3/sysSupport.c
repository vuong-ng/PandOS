#include "sysSupport.h"
void syscallHandler(support_t* sPtr)
{
    int a0 = sPtr->sup_exceptState[GENERALEXCEPT].s_a0;
    increasePC(); 

    switch(a0)
    {
        case 9:
        {
            SYSCALL(TERMINATETHREAD, 0, 0, 0);
            break;
        }
        case 10:
        {
            SYSCALL(GETCPUTIME, 0, 0, 0);
            break;
        }

        /*printer*/
        case 11:
        {
            char* str_addr = sPtr->sup_exceptState[GENERALEXCEPT].s_a1;
            int str_length = sPtr->sup_exceptState[GENERALEXCEPT].s_a2;

             /*address must lie in logical addr space*/
            if (str_length < 0 || str_length > 128 || str_addr < 0x80000 || str_addr > 0x8001E)    
                SYSCALL(9, 0, 0, 0);

            int dev_no = sPtr->sup_asid;     /*which printer determined by the process id*/
            device_t* printer_dev_reg = DEVREGBASE + (PRNTINT - 3) * DEVREGINTSCALE + dev_no * DEVREGDEVSCALE;
            /*print each character*/

            int i, printed = 0;
            for(i = 0; i < str_length; i++)
            {
                printer_dev_reg->d_data0 = *str_addr;
                printer_dev_reg->d_command = PRINTCHR;
                SYSCALL(WAITIO, PRNTINT, dev_no, 0);

                if(printer_dev_reg->d_status == READY)
                    printed++;

                str_addr = str_addr + 1;   /*move 1 byte to next character*/
                
            }
            
            sPtr->sup_exceptState[GENERALEXCEPT].s_v0 = printed;
            break;

        }

        case 12:
        {
            char* str_addr = sPtr->sup_exceptState[GENERALEXCEPT].s_a1;
            int str_length = sPtr->sup_exceptState[GENERALEXCEPT].s_a2;

             /*address must lie in logical addr space*/
            if (str_length < 0 || str_length > 128 || str_addr < 0x80000 || str_addr > 0x8001E)    
                SYSCALL(9, 0, 0, 0);

            int dev_no = sPtr->sup_asid;     /*which printer determined by the process id*/
            device_t* termwrite_dev_reg = DEVREGBASE + (TERMINT - 3) * DEVREGINTSCALE + dev_no * DEVREGDEVSCALE + TERMWRITE;
            /*print each character*/

            int i, written = 0;
            for(i = 0; i < str_length; i++)
            {
                termwrite_dev_reg->t_transm_command = (*str_addr) << 8;
                termwrite_dev_reg->t_transm_command |= TRANSMITCHAR;
                SYSCALL(WAITIO, TERMINT, dev_no, TERMWRITE);

                if((termwrite_dev_reg->t_transm_status) & 0xFF == CHARTRANSMITTED)
                    written++;

                str_addr = str_addr + 1;   /*move 1 byte to next character*/
                
            }
            
            sPtr->sup_exceptState[GENERALEXCEPT].s_v0 = written;
            break;
        }
        case 13:
        {
            char* str_addr = sPtr->sup_exceptState[GENERALEXCEPT].s_a1;

             /*address must lie in logical addr space*/
            if (str_addr < 0x80000 || str_addr > 0x8001E)    
                SYSCALL(9, 0, 0, 0);

            int dev_no = sPtr->sup_asid;     /*which printer determined by the process id*/
            device_t* termwrite_dev_reg = DEVREGBASE + (TERMINT - 3) * DEVREGINTSCALE + dev_no * DEVREGDEVSCALE + TERMWRITE;
            /*print each character*/

            int read = 0;
            while ((termwrite_dev_reg->t_recv_status) >> 8 != '\0')
            {
                termwrite_dev_reg->t_recv_command |= RECEIVECHAR;
                SYSCALL(WAITIO, TERMINT, dev_no, TERMREAD);

                if((termwrite_dev_reg->t_recv_status) & 0xFF == CHARRECEIVED)
                    read++;

                *str_addr = (termwrite_dev_reg->t_recv_status) >> 8;

                str_addr = str_addr + 1;   /*move 1 byte to next character*/
                
            }
            
            sPtr->sup_exceptState[GENERALEXCEPT].s_v0 = read;
            break;
        }
    }

}