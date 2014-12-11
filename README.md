Nucleo Retargeting Example
-----------------------------

As the Nucleo boards provide access to one of the STM32's USARTS 
(usually USART2) on their debug connector, this example shows you
how to 'hook it' into the C library provided in the [gcc-arm-embedded][gcc]
toolchain so that you can use printf and friends to talk to your
board from your C code.

[gcc]: http://launchpad.net/gcc-arm-embedded/

Add the object retarget.o to the OBJ files and it will add a constructor
function (`SystemInit()`) which will run before main is started. That
function initializes the system clock, the UART that is connected to
the Debug portion of the Nucleo board, and a 1kHz system "tick" interrupt
(which is useful for timing etc). 

The main.c code simply prints out some numbers and creates a simple
interactive dialog over the UART "console".

There are two board files now, one for the NUCLEO-F030R8 and one for
the NUCLEO-F411RE which differ by processor of course. The retarget file
is also updated to use `scb_reset_system` which more reliably resets the
board than hacking the stack does.
