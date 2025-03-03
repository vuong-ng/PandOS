#ifndef INTERRUPTS
#define INTERRUPTS

extern void interruptHandler(unsigned int cause);
extern int getPendingDevice(memaddr* int_line_bitmap);
extern void nonTimerInterruptHandler(int interrupt_line, int dev_no);
extern void PLTInterruptHandler();
extern void IntervalTimerInterruptHandler();

#endif