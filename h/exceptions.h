#ifndef EXCEPTIONS
#define EXCEPTIONS
#include "../h/types.h"

extern void fooBar();
extern void uTLB_RefillHandler();
extern void syscallHandler();
extern void trapHandler();
extern void passUp(int passup_type);

extern void copyState(state_t* dest, state_t* src);
extern void terminateProc(pcb_PTR proc);


#endif