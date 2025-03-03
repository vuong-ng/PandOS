#ifndef EXCEPTIONS
#define EXCEPTIONS
#include "../h/types.h"

extern void generalExceptionHandler();
extern void uTLB_RefillHandler();
extern void syscallHandler();
extern void trapHandler();

extern void passUpOrDie(int passup_type);
extern void killProcess(pcb_PTR proc);
extern void killDescendants(pcb_PTR first_child);
extern void Passeren(int* semAdd);
extern pcb_PTR Verhogen(int* semAdd);


extern void increasePC();
extern void updateTime(pcb_PTR proc);
extern void copyState(state_t* dest, state_t* src);
extern int isDeviceSem(int* semAdd);


#endif