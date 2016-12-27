# AVR UART 

This bit of code will give you a UART that behaves correctly like a file, well sort of. Unlike the Ardiuno HardwareSerial, you can use buffered i/o or unbuffered, direct i/o. In addition, the code allows the use of "printf" and "fscanf" functions, as well as the more direct puts(), putc(), fgets() and fgetc() functions. 

In the header file you can set/unset a number of compile flags that allow you to shrink this code down to about 500 bytes, i.e. the difference between a main.cpp with and without the uart_init(), a puts() and a fgets(), is 500 bytes. The buffered version has a footprint of about 1k if you avoid the printf and scanf libraries.

### Options

```
#define UART_ALLOW_BAUD 1
```
This option, when set to 1, allow the BAUD definition during *uart_init()*, but adds ~320 bytes of code. If set to 0 the baudrate is set with #define BAUD <baudrate>.

```
#define UART_ECHO 1
```
If set to 1, then each input character is echoed immediately, if set to 0 there is no echo. Note that the echo does not use the output buffer, so it waits for the uart tx to finish and then places the character on the output. This causes a delay inside the interrupt routine, which is not recommended for applications with tight timing. It is useful for debugging.

```
#define UART_BLOCK_ON_OVERFLOW 1
```
If set to 1, the uart transmit routine will block, i.e. wait, until there is space in the output buffer, so effectively the buffering is turned off. If set to 0, then extra characters will be discarded and a '@' is printed indicating the overflow.

```
#define UART_USE_BUFFERS 1
```
This enables the buffered and interrupt driven code. If set to 0, the uart is un-buffered and quite simple, saving about 1kbyte in program space.
