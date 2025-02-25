#ifndef EXCEPTIONS
#define EXCEPTIONS
#include "../h/initial.h"
#include "../h/interrupts.h"

extern void fooBar();
extern void uTLB_RefillHandler ();
extern void syscallHandler();
extern void trapHandler();
extern void passUp(int passup_type);
extern state_t* CP0_exception_s = BIOSDATAPAGE;

extern void copyState(state_t* dest, state_t* src);


#endif