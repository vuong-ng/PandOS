/******************************* INTERRUPTS.h *********************************
*
* Module: Interrupt handling declarations for Pandos OS
*
* External Functions:
* - interruptHandler(): Routes interrupts by priority (Lines 1-7)
* - getPendingDevice(): Gets pending device number from devices bitmap
* - nonTimerInterruptHandler(): Handles non-timer (device) interrupts
* - PLTInterruptHandler(): Handles processor local timer interrupt
* - IntervalTimerInterruptHandler(): Handles interval timer interrupt
*
* Interrupt Types:
* - Timer: PLT (Line 1), Interval (Line 2)
* - Devices: Disk (3), Flash (4), Network (5), Printer (6), Terminal (7)
*
* Dependencies:
* - types.h: System type definitions 
* - const.h: Device and interrupt constants
*****************************************************************************/

#ifndef INTERRUPTS
#define INTERRUPTS

extern void interruptHandler(unsigned int cause);           /*Main interrupt handler - routes interrupts by priority*/
extern int getPendingDevice(memaddr* int_line_bitmap);      /*Gets device number with pending interrupt from bitmap*/
extern void nonTimerInterruptHandler(int interrupt_line, int dev_no);      /*Handles device (non-timer) interrupts */
extern void PLTInterruptHandler();                           /*Handles processor local timer interrupts*/
extern void IntervalTimerInterruptHandler();                /*Handles interval timer interrupts*/

#endif