#ifndef VMSUPPORT
#define VMSUPPORT

#define UPROCMAX 8

#include "initial.h"
#include "const.h"
#include "initProc.h"


extern swap_frame_t swap_pool_table [UPROCMAX * 2]; /*each entry per Swap Pool frame, recording information about the logical page occupying it*/
extern int swap_pool_sem;

extern void initSwapStructs();
extern void pager();



#endif