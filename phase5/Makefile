#$Id: Makefile,v 1.2 2004/05/01 14:53:48 morsiani Exp morsiani $
# Makefile for mips-linux
#
# Edited for uMPS v3 by MikeyG on 2020/04/14

# Simplistic search for the umps3 installation prefix.
# If you have umps3 installed on some weird location, set UMPS3_DIR_PREFIX by hand.
ifneq ($(wildcard /usr/bin/umps3),)
	UMPS3_DIR_PREFIX = /usr
	LIBDIR = $(UMPS3_DIR_PREFIX)/lib/x86_64-linux-gnu/umps3
	
else
	UMPS3_DIR_PREFIX = /usr/local
	LIBDIR = $(UMPS3_DIR_PREFIX)/lib/umps3
endif

INCDIR = $(UMPS3_DIR_PREFIX)/include/umps3/umps
SUPDIR = $(UMPS3_DIR_PREFIX)/share/umps3
#LIBDIR = $(UMPS3_DIR_PREFIX)/lib/umps3

DEFS = ../h/const.h ../h/types.h ../h/pcb.h ../h/asl.h \
	../h/initial.h ../h/interrupts.h ../h/scheduler.h ../h/exceptions.h \
	../h/initProc.h ../h/vmSupport.h ../h/sysSupport.h ../h/delayDaemon.h ../h/deviceSupportDMA.h \
	$(INCDIR)/libumps.h Makefile

OBJS = ../phase1/asl.o ../phase1/pcb.o \
       ../phase2/initial.o ../phase2/interrupts.o ../phase2/scheduler.o ../phase2/exceptions.o \
       ../phase3/initProc.o ../phase3/vmSupport.o ../phase3/sysSupport.o \
	   ../phase4/deviceSupportDMA.o \
	   delayDaemon.o

../phase1/%.o: ../phase1/%.c $(DEFS)
	$(CC) $(CFLAGS) -o $@ $<

../phase2/%.o: ../phase2/%.c $(DEFS)
	$(CC) $(CFLAGS) -o $@ $<

../phase3/%.o: ../phase3/%.c $(DEFS)
	$(CC) $(CFLAGS) -o $@ $<

../phase4/%.o: ../phase4/%.c $(DEFS)
	$(CC) $(CFLAGS) -o $@ $<


CFLAGS = -ffreestanding -ansi -Wall -c -mips1 -mabi=32 -mfp32 -mno-gpopt -G 0 -fno-pic -mno-abicalls

LDAOUTFLAGS = -G 0 -nostdlib -T $(SUPDIR)/umpsaout.ldscript
LDCOREFLAGS =  -G 0 -nostdlib -T $(SUPDIR)/umpscore.ldscript


CC = mipsel-linux-gnu-gcc
LD = mipsel-linux-gnu-ld
AS = mipsel-linux-gnu-as -KPIC

EF = umps3-elf2umps
UDEV = umps3-mkdev

#main target
all: kernel.core.umps devices disk0.umps

kernel.core.umps: kernel
	$(EF) -k kernel

kernel: $(OBJS)
	$(LD) $(LDCOREFLAGS) $(LIBDIR)/crtso.o $(OBJS) $(LIBDIR)/libumps.o -o kernel

%.o: %.c $(DEFS)
	$(CC) $(CFLAGS) $<


clean:
	rm -f *.o *.umps kernel 
	rm -f ../phase1/*.o
	rm -f ../phase2/*.o
	rm -f ../phase3/*.o
	rm -f ../phase4/*.o


distclean: clean
	-rm kernel.*.umps tape0.umps

PRINTERS = print0.umps print1.umps print2.umps print3.umps print4.umps print5.umps print6.umps print7.umps
TERMINALS = term0.umps term1.umps term2.umps term3.umps term4.umps term5.umps term6.umps term7.umps
devices: $(PRINTERS) $(TERMINALS)

disk0.umps:
	$(UDEV) -d disk0.umps

%.umps:
	touch $@ 