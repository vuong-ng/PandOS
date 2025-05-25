# Pandos Operating System
## About the project
Pandos is a major educational project that we built in our Operating System course. The Pandos operating system was originally inspired by the T.H.E. system outlined by Dijkstra back in 1968. Each layer i was an ADT or abstract machine to layer i + 1; successively building up the capabilities of the system for each new layer to build upon. A key design goal of Pandos is to be representatively complete. Many of the standard parts of an operating system are present, though in a rather unsophisticated form. Pandos can be run on μMPS3. In this project we built 7 Levels of Pandos:

Level 0: The base hardware of μMPS3.

Level 1: The additional services provided in BIOS. This includes the services pro- vided by the BIOS-Excpt handler, the BIOS-TLB-Refill handler, and the additional BIOS services/instructions (i.e. LDST, LDCXT, PANIC, and HALT).

Level 2: The Queues Manager (Phase 1 – described in Chapter 2). Based on the key operating systems concept that active entities at one layer are just data structures at lower layers, this layer supports the management of queues of structures: pcb’s.

Level 3: The Kernel (Phase 2 – described in Chapter 3). This level implements eight new kernel-mode process management and synchronization primitives in addition to multiprogramming, a process scheduler, device interrupt han- dlers, and deadlock detection.

Level 4: The Support Level - The Basics (Phase 3 – described in Chapter 4). Level 3 is extended to support a system that can support multiple user-level pro- cesses that each run in their own virtual address space. Furthermore, support is provided to read/write to character-oriented devices.

Level 5: DMA Device Support (Phase 4 – described in Chapter 5). An extension of Level 4 providing I/O support for DMA devices: disk drives and flash de- vices. Furthermore, this level optionally implements a more realistic back- ing store implementation.

Level 6: The Delay Facility (Phase 5 – described in Chapter 6). This level provides the Support Level with a sleep/delay facility.

Level 7: Cooperating User Processes (Phase 6 – described in Chapter 7). This level introduces a shared memory space and user-level synchronization primitives to facilitate cooperating processes.

## Run project
To run and test 5 phases, do the following:

1. cd into `testers` and implement `make`
> cd testers

> make

2. cd back into phase 5 and `make` phase 5
> cd ../phase5

> make

3. Run in phase 5 calling `umps3`
> umps3

There are sample testers processes provided in `testers` but you are welcome to add more processes.

Written by Long Pham & Vuong Nguyen.