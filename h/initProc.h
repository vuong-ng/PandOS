#ifndef INITPROC
#define INITPROC

#include "const.h"
#include "types.h"

#include "initial.h"
#include "vmSupport.h"
#include "sysSupport.h"

extern void test();
extern void disableInterrupts();
extern void enableInterrupts();

extern int mutex_device_sem[DEVSEMNO];
extern int masterSemaphore;

#endif