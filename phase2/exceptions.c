#include "../h/exceptions.h"

void uTLB_RefillHandler () 
{
    setENTRYHI(0x80000000);
    setENTRYLO(0x00000000);
    TLBWR();
    LDST ((state_PTR) 0x0FFFF000);
}

void fooBar()
{
    STST(&curr_proc->p_s); /*store curr proc's state in memory (in 0x0FFFF000 ?)*/
    setCAUSE(curr_proc->p_s.s_cause); /*set Cause register*/
    unsigned int cause = getCAUSE();

    cause >>= 2;
    cause &= 0b00000000000000000000000000001111;
    switch (cause)
    {
    case 0:
    {
        break;
    }
        
    case 1:
        break;
    case 2:
        break;
    case 3:
        break;
    case 4:
        break;
    case 5:
        break;
    case 6:
        break;
    case 7:
        break;
    case 8:
        break;
    case 9:
        break;
    case 10:
        break;
    case 11:
        break;
    case 12:
        break;
    default:
        break;
    }
}
