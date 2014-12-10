Nucleo-F411RE Retargeting Example
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
