#ifndef __UART_CPP__
#define __UART_CPP__

#include <stdio.h>

#ifndef F_CPU
#error This code will not work without properly defining F_CPU
#endif

#ifndef BAUD
#error This code will not work properly without defining BAUD
#endif

void uart_init(void); //! Initialize the UART code.
int  uart_putchar(char c, FILE *stream);          //! Internal routine, unbuffered put.
int  uart_getchar(FILE *stream);                  //! Internal routine, unbuffered get.


#endif
