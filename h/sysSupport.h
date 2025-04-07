#ifndef SYSSUPPORT
#define SYSSUPPORT

#include "initProc.h"

extern void sptGeneralExceptionHandler();
extern void sptTrapHandler(support_t* sPtr, int* swap_pool_sem);
#endif