#include "../h/asl.h"
#include "../h/pcb.h"
#include "../h/exceptions.h"
#include "../h/initial.h"
#include "../h/scheduler.h"
#include "../h/interrupts.h"

void interruptHandler(unsigned int cause)
{
    /*determine highest priority pending interrupt,
    Interrupt Line 0 skipped for uniprocessor*/
    
    if      ((cause >> (PLTINT + GETINTLINE)         & CLEAR31MSB) == ON){PLTInterruptHandler();}  /*Interrupt Line 1*/
    else if ((cause >> (INTERVALTMRINT + GETINTLINE) & CLEAR31MSB) == ON){IntervalTimerInterruptHandler();}  /*Interrupt Line 2*/
    else if ((cause >> (DISKINT + GETINTLINE)        & CLEAR31MSB )== ON){nonTimerInterruptHandler(DISKINT, getPendingDevice(INTERRUPTLINE3));}  /*Interrupt Line 3*/
    else if ((cause >> (FLASHINT + GETINTLINE)       & CLEAR31MSB) == ON){nonTimerInterruptHandler(FLASHINT, getPendingDevice(INTERRUPTLINE4));} /*Interrupt Line 4*/
    else if ((cause >> (NETWINT + GETINTLINE)        & CLEAR31MSB) == ON){nonTimerInterruptHandler(NETWINT, getPendingDevice(INTERRUPTLINE5));}  /*Interrupt Line 5*/
    else if ((cause >> (PRNTINT + GETINTLINE)        & CLEAR31MSB) == ON){nonTimerInterruptHandler(PRNTINT, getPendingDevice(INTERRUPTLINE6));}  /*Interrupt Line 6*/
    else if ((cause >> (TERMINT + GETINTLINE)        & CLEAR31MSB) == ON){nonTimerInterruptHandler(TERMINT, getPendingDevice(INTERRUPTLINE7));}  /*Interrupt Line 7*/
    else                                                                 {return;}                                                               /*No Pending Interrupt*/
}

int getPendingDevice(memaddr* int_line_bitmap)
{
    if      ((*int_line_bitmap >> 0 & CLEAR31MSB) == ON) return 0;
    else if ((*int_line_bitmap >> 1 & CLEAR31MSB) == ON) return 1;
    else if ((*int_line_bitmap >> 2 & CLEAR31MSB) == ON) return 2;
    else if ((*int_line_bitmap >> 3 & CLEAR31MSB) == ON) return 3;
    else if ((*int_line_bitmap >> 4 & CLEAR31MSB) == ON) return 4;
    else if ((*int_line_bitmap >> 5 & CLEAR31MSB) == ON) return 5;
    else if ((*int_line_bitmap >> 6 & CLEAR31MSB) == ON) return 6;
    else if ((*int_line_bitmap >> 7 & CLEAR31MSB) == ON) return 7;
    else                                                 return -1; /*no device pending left for this interrupt line*/
}

void nonTimerInterruptHandler(int interrupt_line, int dev_no)
{
    STCK(time_start); /*to store time spent in interrupt handling*/

    /*compute the starting address of the device’s device register*/
    device_t* device_register = DEVREGBASE + (interrupt_line - 3) * 0x80 + dev_no * 0x10;
    
    unsigned int saved_status;
    int* device_semAdd;

    /*terminal devices*/
    if(interrupt_line == TERMINT)
    {
        memaddr* TRANSM_COMMAND = &device_register->t_transm_command;
        memaddr* TRANSM_STATUS = &device_register->t_transm_status;
        memaddr* RECV_COMMAND = &device_register->t_recv_command;
        memaddr* RECV_STATUS = &device_register->t_recv_status;

        /*if transmit not ready (there is transmit interrupt), acknowledge (complete) */
        if((*TRANSM_STATUS & TERMSTATMASK) != READY)
        {
            saved_status = *TRANSM_STATUS;      /*save off status code from device register*/
            *TRANSM_COMMAND = ACK;              /*ACK transmit command*/
            device_semAdd = &device_sem[(interrupt_line - 3) * DEVPERINT + dev_no * SUBDEVPERTERM + TERMWRITE];
        }
        /*else if transmit ready (there is receive interrupt), acknowledge (complete)*/
        else if ((*TRANSM_STATUS & TERMSTATMASK) == READY)
        {
            saved_status = *RECV_STATUS;        /*save off status code from device register*/
            *RECV_COMMAND = ACK;                /*ACK receive command*/
            device_semAdd = &device_sem[(interrupt_line - 3) * DEVPERINT + dev_no * SUBDEVPERTERM + TERMREAD];
        }
    }

    /*not terminal device */
    else
    {
        saved_status = device_register->d_status;  /*save off status code from device register*/
        device_register->d_command = ACK;       /*ACK device command*/
        device_semAdd = &device_sem[(interrupt_line - 3) * DEVPERINT + dev_no];
    }
        
    /*perform V on Nucleus maintain semaphore of this device*/
    pcb_PTR unblocked_pcb = Verhogen(device_semAdd);
   
    if(unblocked_pcb != NULL)
    {
        unblocked_pcb->p_s.s_v0 = saved_status;  /*place saved off status code into the new unblocked pcb's v0 reg*/
        updateTime(unblocked_pcb);    /*charge to new unblocked pcb cuz it requested the IO*/
    }

    /*return to curr_proc  
    perform LDST on saved exception state on BIOS Data Page (processor 0 excp state)*/
    
    if(curr_proc != NULL) 
        LDST((state_t*) BIOSDATAPAGE); 
    /*if there's no curr_proc, Scheduler calls WAIT()*/
    else
        scheduler();
}

void PLTInterruptHandler()
{
    setTIMER(PLTSTART); /*acknowledge PLT interrupt by loading timer with new value*/

    /*copy processor state (BIOS Data Page) into pcb's p_s */
    copyState(&(curr_proc->p_s), (state_t*) BIOSDATAPAGE);

    /*update accumulated CPU time for curr_proc*/
    updateTime(curr_proc);

    insertProcQ(&ready_queue, curr_proc); /*Place the Current Process on the Ready Queue*/
    scheduler();    /*call scheduler*/
}

void IntervalTimerInterruptHandler()
{
    LDIT(CLOCKINTERVAL);  /*acknowledge interrupt by loading interval timer with 100 millisecs*/

    /*unblock all pcbs blocked on pseudo-clock (49) semaphore*/
    pcb_PTR p;
    while ((p = removeBlocked(&device_sem[PSEUDOCLK])) != NULL)
    {
        insertProcQ(&ready_queue, p);
        softblock_cnt--;
    }

    /*reset pseudo-clock semaphore to 0*/
    device_sem[PSEUDOCLK] = 0;

    /*return control to curr_proc*/
    if(curr_proc != NULL)
        LDST((state_t*) BIOSDATAPAGE);
    /*call scheduler if there's no current process to return to*/
    else
        scheduler();
} 

