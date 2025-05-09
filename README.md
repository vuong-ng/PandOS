# CS372_Project 
An implementation of the Pandos Operating System:
1. Phase 1: A simple queue implementation for pcbs and semaphores queues.
2. Phase 2: Implements the kernel, which supports 8 syscalls and interrupts handling.
3. Phase 3: Implements virtual memory, and additional user-level syscalls.
4. Phase 4: Implements DMA-based devices: disk and flash.
5. Phase 5: Implements a Daemon process, along with a delay facility for user processes.

To run and test the whole 5 phases, do the following
> cd testers
> make
> cd ../phase5
> make
> umps3
and set the configurations inside the simulator as desired.

Written by Long Pham & Vuong Nguyen.