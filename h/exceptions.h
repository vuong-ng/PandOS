#ifndef EXCEPTIONS
#define EXCEPTIONS
#include "../h/initial.h"
#include "../h/interrupts.h"

extern void fooBar();
extern void uTLB_RefillHandler ();
extern void syscallHandler();
extern passUp(int passup_type);
extern state_t* processor_0_exception_state = BIOSDATAPAGE;


#endif