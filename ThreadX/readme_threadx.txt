                          Express Logic's ThreadX for ARC600

                             Using the MetaWare Tools

                               Educational Version


1.  Installation

The educational version of ThreadX for the ARC600 is delivered in a zip
file named tx52arc600edu.zip. This file can be extracted in any directory
and the demonstration my be built via executing the batch file
build_threadx_demo.bat.

Please note that this educational version of ThreadX for the ARC600 is 
strictly for educational use only and cannot be used for any product 
development or research. If you need ThreadX for such purposes, please 
contact your local distributor or Express Logic directly.


2.  Building the ThreadX run-time Library

The ThreadX educational version comes with a pre-built version of the 
ThreadX library that is limited to a maximum of 10 threads, 10 queues, 
10 semaphores, 10 mutexes, 10 event flag groups, 10 byte pools, and 
10 block pools. 


3.  Demonstration System

Building the demonstration is easy; simply execute the build_threadx_demo.bat 
batch file while inside your ThreadX directory on your hard-disk.

C:\threadx\arc600\metaware> build_threadx_demo

You should observe the compilation of demo_threadx.c (which is the demonstration 
application) and linking with tx.a. The resulting file demo_thread.out is a binary 
file that can be downloaded and executed on the ARC600 simulator. The following 
command line will load the ThreadX demo into the ARC600 simulator:

> scac -Xtimer demo.out

A SPECIAL NOTE:  The ThreadX demonstration needs the ARC600 auxiliary timer
interrupt in order to drive the time-dependent threads (threads 0, 3, 4, and
5).  Without a working timer interrupt, threads 1 and 2 are the only ones
that will run more than once.  The ThreadX demonstration should also run
on the actual ARC600 evaluation hardware providing it has the same auxiliary
timer registers and the same memory map.


4.  System Initialization

The system entry point using the MetaWare tools is at the label _start. 
This is defined within the crt1.s file supplied by MetaWare. In addition, 
this is where all static and global preset C variable initialization
processing is called from.

After the MetaWare startup function completes, ThreadX initialization is
called. The main initialization function is _tx_initialize_low_level and
is located in the file tx_initialize_low_level.s. This function is 
responsible for setting up various system data structures, and interrupt 
vectors.

By default free memory is assumed to start at the linker defined symbol 
_end. This is the address passed to the application definition 
function, tx_application_define.


5.  Assembler / Compiler Switches

The following are compiler switches used in building the demonstration 
system:

Compiler Switch                 Meaning

    -g                  Specifies debug information
    -c                  Specifies object code generation
    -Xmult32            Enables instructions for the 32x32 bit multiplier
                        extension. This will result in an optimized kernel 
                        build, and extended context handling for this multiplier.
    -Xmul_mac           Enables instructions for the 16x16 bit DSP multiplier
                        extension (ARC DK 2). This does not affect the kernel 
                        build, but produces extended context handling for this 
                        multiplier.
    -Xxmac_16           Enables instructions for the 16x16 bit XMAC extension (ARC
                        DK 3). This does not affect the kernel build, but
                        produces extended context handling for this multiplier.
    -Xxmac_d16          Enables instructions for the double 16x16 bit XMAC
                        extension (ARC DK 3). This does not affect the kernel 
                        build, but produces extended context handling for this 
                        multiplier.
    -Xxmac_24           Enables instructions for the 24x24 bit XMAC extension (ARC
                        DK 3). This does not affect the kernel build, but
                        produces extended context handling for this multiplier.
    Xmac_16x16_single   This assembler pre-processor definition should be
                        used when targeting the single 16x16 XMAC as this form 
                        of the XMAC unit uses one less register than the double 
                        16x16 or 24x24 versions. It suppresses the saving and 
                        restoring of the redundant register when context switching.  
                        It may be set by including "-Hasopt=-DXmac_16x16_single" 
                        (with the quotes) in the hcarc command line.


Linker Switch                   Meaning

    -o demo_threadx.out Specifies demo output file name
    -m                  Specifies a map file output


6.  Register Usage and Stack Frames

The ARC compiler assumes that registers r0-r12 are scratch registers for 
each function. All other registers used by a C function must be preserved 
by the function. ThreadX takes advantage of this in situations where a 
context switch happens as a result of making a ThreadX service call (which 
is itself a C function). In such cases, the saved context of a thread is 
only the non-scratch registers.

The following defines the saved context stack frames for context switches
that occur as a result of interrupt handling or from thread-level API calls.
All suspended threads have one of these two types of stack frames. The top
of the suspended thread's stack is pointed to by tx_thread_stack_ptr in the 
associated thread control block TX_THREAD.



    Offset        Interrupted Stack Frame        Non-Interrupt Stack Frame

     0x00                   1                           0
     0x04                   LP_START                    blink
     0x08                   LP_END                      fp
     0x0C                   LP_COUNT                    r26
     0x10                   blink                       r25
     0x14                   ilink1                      r24
     0x18                   fp                          r23
     0x1C                   r26                         r22
     0x20                   r25                         r21
     0x24                   r24                         r20
     0x28                   r23                         r19
     0x2C                   r22                         r18
     0x30                   r21                         r17
     0x34                   r20                         r16
     0x38                   r19                         r15
     0x3C                   r18                         r14
     0x40                   r17                         r13
     0x44                   r16                         STATUS32
     0x48                   r15
     0x4C                   r14
     0x50                   r13
     0x54                   r12
     0x58                   r11
     0x5C                   r10
     0x60                   r9
     0x64                   r8
     0x68                   r7
     0x6C                   r6
     0x70                   r5
     0x74                   r4
     0x78                   r3
     0x7C                   r2
     0x80                   r1
     0x84                   r0
     0x88                   mlo (Mult32 extension)
     0x8C                   mhi (Mult32 extension)
     0x90                   xtp_newval (Mul/Mac extension)
                                OR xmac0 (Xmac)
     0x94                   lsp_newval (Mul/Mac)
                                OR xmac1 (Xmac)
     0x98                   macmode (Mul/Mac)
                                OR aux_macmode (Xmac)
     0x9C                   aux_mac0 (Xmac)
     0xA0                   aux_mac1 (Xmac)
     0xA4                   aux_mac2 (Xmac)
     0xA8                   STATUS32_L1


7.  Improving Performance

The distribution version of ThreadX is built without any compiler 
optimizations. This makes it easy to debug because you can trace or set 
breakpoints inside of ThreadX itself. Of course, this costs some 
performance. To make it run faster, you can change the build_threadx.bat 
file to remove the -g option and enable all compiler optimizations. 

In addition, you can eliminate the ThreadX basic API error checking by 
compiling your application code with the symbol TX_DISABLE_ERROR_CHECKING 
defined. 


8.  Interrupt Handling

ThreadX provides complete and high-performance interrupt handling for the
ARC600 processor. The file tx_initialize_low_level.s contains assembly language shells for
each of the Level 1 ARC interrupts. The Level 2 ARC interrupts are left
completely along, i.e. ThreadX does not disable Level 2 interrupts and 
does not allow ThreadX API services to be called from Level 2 ISRs.

The ARC Level 1 ISR template is defined as follows:

    .global _tx_vector_irq_level1
_tx_vector_irq_level1:
    sub     sp, sp, 172                     ; Allocate an interrupt stack frame
...

    st      blink, [sp, 16]                 ; Save blink (blink must be saved before _tx_thread_context_save)
    bl      _tx_thread_context_save         ; Save interrupt context
;
;   /* Application ISR processing goes here!  Your ISR can be written in 
;      assembly language or in C. If it is written in C, you must allocate
;      16 bytes of stack space before it is called. This must also be 
;      recovered once your C ISR return. An example of this is shown below. 
;
;      If the ISR is written in assembly language, only the compiler scratch
;      registers are available for use without saving/restoring (r0-r12). 
;      If use of additional registers are required they must be saved and
;      restored.  */
;
    bl      your_ISR_written_in_C           ; Call an ISR written in C
;
    b       _tx_thread_context_restore      ; Restore interrupt context


9.  ThreadX Timer Interrupt

ThreadX requires a periodic interrupt source to manage all time-slicing, 
thread sleeps, timeouts, and application timers. Without such a timer 
interrupt source, these services are not functional but the remainder of 
ThreadX will still run.

By default, the ThreadX timer interrupt is mapped to the ARC600 auxiliary
timer 0, which generates Level 1 interrupts on interrupt vector 3.
It is easy to change the timer interrupt source by changing the setup 
code in tx_initialize_low_level.s and also changing the Level 1 interrupt vector and timer
reset processing if necessary.


10.  Revision History

For generic code revision information, please refer to the readme_threadx_generic.txt
file, which is included in your distribution. The following details the revision
information associated with this specific port of ThreadX:

12/12/2008  Educational version of ThreadX G5.2.5.0 for ARC600 using MetaWare tools.


Copyright(c) 1996-2008 Express Logic, Inc.


Express Logic, Inc.
11423 West Bernardo Court
San Diego, CA  92127

www.expresslogic.com

