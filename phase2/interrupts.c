#include "../h/interrupts.h"

void interruptHandler(unsigned int* cause)
{
    /*step 1: determine highest priority pending interrupt*/
    int IP = (*cause);
    int* interrupt_line_address;
    int dev_no;

    /*Interrupt Line 0*/
    if ((IP >> 15) & CLEAR31MSB == 1)
    {

    }
    /*Interrupt Line 1*/
    else if ((IP >> 14) & CLEAR31MSB == 1)
    {

    }
    /*Interrupt Line 2*/
    else if ((IP >> 13) & CLEAR31MSB == 1)
    {

    }
    /*Interrupt Line 3*/
    else if ((IP >> 12) & CLEAR31MSB  == 1)
    {
        interrupt_line_address = INTERRUPTLINE3;
        dev_no = getPendingDevice(interrupt_line_address);
        if (dev_no == -1)
            (*cause) & 0b1111111111111111111011111111111;  /*set interrupt line bit to 0*/
        else
        {
            nonTimerInterruptHandler(3, dev_no);
        } 

    }
    /*Interrupt Line 4*/
    else if ((IP >> 11) & CLEAR31MSB == 1)
    {
        interrupt_line_address = INTERRUPTLINE4;
        dev_no = getPendingDevice(interrupt_line_address);
        if (dev_no == -1) 
            (*cause) & 0b1111111111111111111101111111111; /*set interrupt line bit to 0*/
        else
        {
            nonTimerInterruptHandler(4, dev_no); 
        } 
    }
    /*Interrupt Line 5*/
    else if ((IP >> 10) & CLEAR31MSB == 1)
    {
        interrupt_line_address = INTERRUPTLINE5;
        dev_no = getPendingDevice(interrupt_line_address);
        if (dev_no == -1)
            (*cause) & 0b1111111111111111111110111111111; /*set interrupt line bit to 0*/
        else
        {
            nonTimerInterruptHandler(5, dev_no);
        }  
    }
    /*Interrupt Line 6*/
    else if ((IP >> 9) & CLEAR31MSB == 1)
    {
        interrupt_line_address = INTERRUPTLINE6;
        dev_no = getPendingDevice(interrupt_line_address);
        if (dev_no == -1)
            (*cause) & 0b1111111111111111111111011111111; /*set interrupt line bit to 0*/
        else
        {
            nonTimerInterruptHandler(6, dev_no); 
        }
    }
    /*Interrupt Line 7*/
    else if ((IP >> 8) & CLEAR31MSB == 1)
    {
        interrupt_line_address = INTERRUPTLINE7;

        dev_no = getPendingDevice(interrupt_line_address);
        if (dev_no == -1)
            (*cause) & 0b1111111111111111111111101111111; /*set interrupt line bit to 0*/
        else 
        {
            nonTimerInterruptHandler(7, dev_no);
        } 
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
    if      ((*interrupt_line_address >> 7) & CLEAR31MSB == 1) return 0;
    else if ((*interrupt_line_address >> 6) & CLEAR31MSB == 1) return 1;
    else if ((*interrupt_line_address >> 5) & CLEAR31MSB == 1) return 2;
    else if ((*interrupt_line_address >> 4) & CLEAR31MSB == 1) return 3;
    else if ((*interrupt_line_address >> 3) & CLEAR31MSB == 1) return 4;
    else if ((*interrupt_line_address >> 2) & CLEAR31MSB == 1) return 5;
    else if ((*interrupt_line_address >> 1) & CLEAR31MSB == 1) return 6;
    else if ((*interrupt_line_address     ) & CLEAR31MSB == 1) return 7;
    else                                                      return -1; /*no device pending left for this interrupt line*/
}


void nonTimerInterruptHandler(int interrupt_line, int dev_no)
{
    /*set handled device to 0 in here*/

    /*calculate address for this device's dev register*/
    int devAddrBase = 0x10000054 + (interrupt_line - 3) * 0x80 + dev_no * 0x10;

    /*save off status code from device register*/
    device_t* device_register = devAddrBase;
    unsigned int saved_status = device_register->d_status;

    /*write ACK command code in device register, or write a new command*/
    device_register->d_command = ACK;

    /*perform V on Nucleus maintain semaphore of this device. */
    int* device_semAdd;
    /*not terminal (HANDLED LATER)*/
    if(interrupt_line != 7)
        device_semAdd = device_sem[(interrupt_line - 3) * 8 + dev_no];
    else
        device_semAdd = device_sem[(interrupt_line - 3) * 8 + dev_no * 2];
    *(device_semAdd) += 1;

    /*unblock pcb that initiated this I/O op and request to wait for completion via SYS5*/
    pcb_PTR unblocked_pcb = removeBlocked(device_semAdd);

    /*if V returns NULL, return to curr_proc (if there's no curr_proc, Scheduler calls WAIT())*/
    if (unblocked_pcb == NULL)
        return;

    /*place saved off status code into the new unblocked pcb's v0 reg*/
    unblocked_pcb->p_s.s_v0 = saved_status;

    /*insert the new unblocked pcb to readyQueue*/
    insertProcQ(&ready_queue, unblocked_pcb);


    /*return to curr_proc (if there's no curr_proc, Scheduler calls WAIT()), 
    perform LDST on saved exception state on BIOS Data Page (processor 0 excp state)*/
    state_t* processor_0_exception_state = BIOSDATAPAGE; 
    LDST(processor_0_exception_state);  /*perform LDST manually here??????*/
}

void PLTInterrupt()
{
    /*acknowledge PLT interrupt by loading timer with new value??*/
    setTIMER(0);

    /*copy processor state (BIOS Data Page) into pcb's p_s */
    state_t* processor_0_exception_state = BIOSDATAPAGE;
    curr_proc->p_s.s_entryHI = processor_0_exception_state->s_entryHI;
    curr_proc->p_s.s_cause = processor_0_exception_state->s_cause;
    curr_proc->p_s.s_status = processor_0_exception_state->s_status;
    curr_proc->p_s.s_pc = processor_0_exception_state->s_pc;
    int i;
    for (i = 0; i < STATEREGNUM; i++)
        curr_proc->p_s.s_reg[i] = processor_0_exception_state->s_reg[i];

    /*update accumulated CPU time for curr_proc*/

    unsigned int quantum_end_time;
    STCK(quantum_end_time);
    curr_proc->p_time += (quantum_end_time - quantum_start_time) + curr_proc->p_time;

    /*Place the Current Process on the Ready Queue*/
    insertProcQ(&ready_queue, curr_proc);
    scheduler();/*call scheduler*/
}

void IntervalTimerInterrupt()
{
    
} 