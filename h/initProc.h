#ifndef INITPROC
#define INITPROC

#include "const.h"
#include "types.h"

#include "initial.h"
#include "vmSupport.h"

#define PFN  12
#define ASID 6
#define D       0x400
#define V       0x200
#define G       0x100

#define GET20LSB  0xFFFFF

#define VPNSHIFT 0x80000
#define STACKPGVPN 0xBFFFF
#define READBLK 0x2
#define WRITEBLK 0x3
#define PRINTCHR 0x2
#define TRANSMITCHAR 0x2
#define RECEIVECHAR 0x2

#define CHARTRANSMITTED 0x5
#define CHARRECEIVED 0x5

#define PAGETABLESIZE 32

extern void test();


#endif