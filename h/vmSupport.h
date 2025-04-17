/******************************** VMSUPPORT.H *********************************
* Written by Long Pham & Vuong Nguyen
* Module: Virtual Memory Support Header for Pandos OS Phase 3
* 
* Dependencies:
* - initial.h: initial
* - const.h: constants
* - initProc.h: U-proc initialization
*
* Exposed Functions:
* - initSwapStructs(): Initializes swap pool data structures
* - pager(): Handles page fault
*
* Data Structures:
* - swap_frame_t: Entry for each swap pool frame
* - swap_pool_table: Array of swap pool frame entries
***************************************************************************/

#ifndef VMSUPPORT
#define VMSUPPORT


#include "initial.h"
#include "const.h"
#include "initProc.h"

/* initialize swap pool data structures */
extern void initSwapStructs();

/* handle page faults */
extern void pager();

#endif