Embedded Systems @ JCT 2013
===========================

This course teaches practices in low-level development, for instance memory mapping, controllers, compilers/linkers, drivers, among many other things. The development is mainly driven around the ARC processor, for which an Ubuntu VM provides the necessary tools.

The objective of the exercises is to program a feature phone device with text message functionality and classical graphical interface. Fun fact: the phone's layout is that of the Sony Ericsson® C510.

Exercise 0
----------
We trained with the development environment, some basic tools and concepts: compiler flags, linking, sections headers, disassembly and understanding ARC assembly code.

Exercise 1
----------
We wrote drivers, making use of memory mapping, by communicating with the user through a remote terminal using the UART protocol and command specifications, by using the CPU clock and by displaying numbers using a seven segments display (SSD).

Exercise 2
----------
We extended the previous exercise by using writing a driver to read from and write to flash memory. This will enable persistence capabilities for exercise 4.

Exercise 3
----------
We implemented SMS-like text messages and developed drivers for the network controller (which sends and receives messages, input panel (with keys 0-9, *, # and OK) and display (18x12 characters).  We also used a real time operating system (RTOS) named ThreadX in order to enable multithreading, which improved performance. Finally, the "slave mode" uses the UART extension from exercise 1 to enable the control of the device through a classic interface. 

Exercise 4
----------
We introduced persistence with the flash memory extension and driver from exercise 2. This allows the user to keep the text messages in memory instead of having them removed every time the device is switched off. We also made some final optimizations (ROM, RAM and flash size and power consumption).

Running
-------
Use the makefile in each exercise folder. Just make sure you have the Metaware C compiler (mcc) for compiling and the Metware debugger (mdb/runac) for debugging and running the simulation.
The code is fully usable, and can work on actual ARC hardware, provided the proper extensions (flash memory, memory-mapped I/O, network transceiver).