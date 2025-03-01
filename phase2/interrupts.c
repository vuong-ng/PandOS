#include "../h/asl.h"
#include "../h/pcb.h"

#include "../h/exceptions.h"
#include "../h/initial.h"
#include "../h/scheduler.h"
#include "../h/interrupts.h"

void interruptHandler(unsigned int cause)
{
    /*debug(10,10,10,10);*/
    /*step 1: determine highest priority pending interrupt*/
    int IP = cause;

    /*Interrupt Line 0*/
    if ((IP >> INTERPROCINT + GETINTLINE) & CLEAR31MSB == ON)
    {

    }
    /*Interrupt Line 1*/
    else if ((IP >> PLTINT + GETINTLINE) & CLEAR31MSB == ON)
        PLTInterruptHandler();
    /*Interrupt Line 2*/
    else if ((IP >> INTERVALTMRINT + GETINTLINE) & CLEAR31MSB == ON)
    {
        IntervalTimerInterruptHandler();
    }
    /*Interrupt Line 3*/
    else if ((IP >> DISKINT + GETINTLINE) & CLEAR31MSB  == ON)
    {
        nonTimerInterruptHandler(DISKINT, getPendingDevice(INTERRUPTLINE3));
    }
    /*Interrupt Line 4*/
    else if ((IP >> FLASHINT + GETINTLINE) & CLEAR31MSB == ON)
    {
        nonTimerInterruptHandler(FLASHINT, getPendingDevice(INTERRUPTLINE4));
    }
    /*Interrupt Line 5*/
    else if ((IP >> NETWINT + GETINTLINE) & CLEAR31MSB == ON)
    {
        nonTimerInterruptHandler(NETWINT, getPendingDevice(INTERRUPTLINE5));
    }
    /*Interrupt Line 6*/
    else if ((IP >> PRNTINT + GETINTLINE) & CLEAR31MSB == ON)
    {
        nonTimerInterruptHandler(PRNTINT, getPendingDevice(INTERRUPTLINE6)); 
    }
    /*Interrupt Line 7*/
    else if ((IP >> TERMINT + GETINTLINE) & CLEAR31MSB == ON)
    {
        nonTimerInterruptHandler(TERMINT, getPendingDevice(INTERRUPTLINE7));
    }
    /*No Pending Interrupt*/
    else 
    {
        return;
    }
    return; /*there's still pending interrupt(s)*/
}


int getPendingDevice(int* interrupt_line_address)
{
    if      ((*interrupt_line_address >> 0) & CLEAR31MSB == ON) return 0;
    else if ((*interrupt_line_address >> 1) & CLEAR31MSB == ON) return 1;
    else if ((*interrupt_line_address >> 2) & CLEAR31MSB == ON) return 2;
    else if ((*interrupt_line_address >> 3) & CLEAR31MSB == ON) return 3;
    else if ((*interrupt_line_address >> 4) & CLEAR31MSB == ON) return 4;
    else if ((*interrupt_line_address >> 5) & CLEAR31MSB == ON) return 5;
    else if ((*interrupt_line_address >> 6) & CLEAR31MSB == ON) return 6;
    else if ((*interrupt_line_address >> 7) & CLEAR31MSB == ON) return 7;
    else                                                        return -1; /*no device pending left for this interrupt line*/
}



void nonTimerInterruptHandler(int interrupt_line, int dev_no)
{
    

    /*calculate address for this device's dev register*/
    /*
    Given an interrupt line (IntLineNo) and a device number (DevNo) one can
    compute the starting address of the device’s device register:
    devAddrBase = 0x1000.0054 + ((IntlineNo - 3) * 0x80) + (DevNo * 0x10)
    */
    device_t* device_register= 0x10000054 + (interrupt_line - 3) * 0x80 + dev_no * 0x10;
    

    /*save off status code from device register*/
    unsigned int saved_status;
    /*debug(saved_status, 1006,1006,1006);*/
    int* device_semAdd;

    if(interrupt_line == TERMINT)
    {
        
        /*
        memaddr TRANSM_COMMAND = device_register->t_transm_command;
        memaddr TRANSM_STATUS = device_register->t_transm_status;
        memaddr RECV_COMMAND = device_register->t_recv_command;
        memaddr RECV_STATUS = device_register->t_recv_status;
        */
        /*transmit not ready -> transmitter interrupt
        transmit ready -> interrupt must be receiver*/

        /*if transmit not ready (there is transmit interrupt), acknowledge (complete) */
        if(device_register->t_transm_status & 0x000000FF != READY)
        {
            saved_status = device_register->t_transm_status;
            device_register->t_transm_command = (device_register->t_transm_command) & 0xFFFFFF00 | ACK; /*set transmit command to ACK, which sets TRANSM_STATUS to READY/CHAR TRANSMITTED*/
            
            device_semAdd = &device_sem[(interrupt_line - 3) * DEVPERINT + dev_no * 2 + 0];
        }
        /*else if transmit ready (there is receive interrupt), acknowledge (complete)*/
        else if (device_register->t_transm_status & 0x000000FF == READY)
        {
            saved_status = device_register->t_recv_status;

            device_register->t_recv_command = device_register->t_recv_command & 0xFFFFFF00 | ACK; /*set receive command to ACK*/
            device_semAdd = &device_sem[(interrupt_line - 3) * DEVPERINT + dev_no * 2 + 1];
        }
        
    }
    /*not terminal */
    else
    {
        saved_status = device_register->d_status;
        /*write ACK command code in device register, or write a new command*/
        device_register->d_command = ACK;
        
         /*perform V on Nucleus maintain semaphore of this device. */
        device_semAdd = &device_sem[(interrupt_line - 3) * DEVPERINT + dev_no];
    }
        
    (*device_semAdd)++;

    /*unblock pcb that initiated this I/O op and requested to wait for completion via SYS5*/
    pcb_PTR unblocked_pcb = removeBlocked(device_semAdd);
    
    cpu_t quantum_end_time;
    /*if V returns NULL, return to curr_proc (if there's no curr_proc, Scheduler calls WAIT())*/
    if (unblocked_pcb == NULL)
    {
        if(curr_proc != NULL)
        {
            
            /*STCK(quantum_end_time);
            curr_proc->p_time += (quantum_end_time - quantum_start_time);*/
            LDST((state_t*) BIOSDATAPAGE); 
        }
            
        else 
            scheduler();
    }

    softblock_cnt--;
        

    /*place saved off status code into the new unblocked pcb's v0 reg*/
    unblocked_pcb->p_s.s_v0 = saved_status;

    /*insert the new unblocked pcb to readyQueue*/
    insertProcQ(&ready_queue, unblocked_pcb);


    /*return to curr_proc (if there's no curr_proc, Scheduler calls WAIT()), 
    perform LDST on saved exception state on BIOS Data Page (processor 0 excp state)*/
    /*increasePC();
    increasePC();*/

    if(curr_proc != NULL) /*interrupt occurred before WAIT*/
    {
        STCK(quantum_end_time);
        curr_proc->p_time += (quantum_end_time - quantum_start_time);
        LDST((state_t*) BIOSDATAPAGE);  /*perform LDST manually here?*/
    }
        
    else
    {
        scheduler();
    }
        
}

void PLTInterruptHandler()
{
    /*acknowledge PLT interrupt by loading timer with new value: all ones with leading bit 0*/
    /*debug(46,46,46,46);*/
    /*debug(curr_proc,49,49,49);*/
    setTIMER(0x7FFFFFFF);
    /*debug(curr_proc,49,49,49);*/
    /*copy processor state (BIOS Data Page) into pcb's p_s */
    copyState(&(curr_proc->p_s), (state_t*) BIOSDATAPAGE);

    /*update accumulated CPU time for curr_proc*/
    cpu_t quantum_end_time;
    STCK(quantum_end_time);
    curr_proc->p_time += (quantum_end_time - quantum_start_time);

    /*Place the Current Process on the Ready Queue*/
    insertProcQ(&ready_queue, curr_proc);
    
    scheduler();    /*call scheduler*/
}

void IntervalTimerInterruptHandler()
{
    /*acknowledge interrupt by loading interval timer with 100 millisecs*/
    /*debug(48,48,48,48);*/
    LDIT(100000);

    /*unblock all pcbs blocked on pseudo-clock (49) semaphore*/
    pcb_PTR p;
    while ((p = removeBlocked(&device_sem[PSEUDOCLK])) != NULL)
    {
        insertProcQ(&ready_queue, p);
        softblock_cnt--;
    }
        

    /*reset pseudo-clock semaphore to 0*/
    device_sem[PSEUDOCLK] = 0;

    /*return control to curr_proc (switchContext func on saved exception on BIOS Data Page)*/
    LDST((state_t*) BIOSDATAPAGE);
} 

