/*
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>

//
// By including the uart.cpp (and not uart.h) we can control the behavior of the
// UART code with the #define statements. In that case, do not include the uart.cpp
// in the compile and link stage.
//
// Ofcourse, you can also edit the uart.h to get the behavior you want, and then
// do a normal compile of uart.cpp and link with main.cpp
//
// Additional notes:
// It took a long investigation of what was happening with fputs(). I discovered that
// fputs() is replaced by fwrite() by the compiler when outputting a string. The fwrite() code
// requires that stream->put() returns a zero, otherwise it is an error and it stops.
//
// So, for the fastest and most compact code, you want to use fwrite exclusively. Using fputs with -Os
// (optimize for size) also works and won't replace some fputs with fwrite. Otherwise, if you use fputs()
// you will include both fputs() code and fwrite() code.
// You can also use printf(), which is very versatile but adds a lot of extra program space.
//
#include "uart.hpp"

extern void __builtin_avr_delay_cycles(unsigned long);
int my_fputs(const char *str, FILE *stream);

int main(void) {

  sei();                    // Enable all interrupts.
  uart_init(0x3,BAUD);
  
  fwrite("\r\nUART test code\r\n",1,18,stdout);
  uart_tx_buffer_flush();
//  fputs("This is a really long sentence that will still fit in buffer\r\n",stdout);
//  uart_tx_buffer_flush(); // Flush, i.e. block, else we will overflow on "Hello World."

  char strr[64];
//  char tmpbuf[10];
//  uint32_t i;
  
  while(1) {
    //    __builtin_avr_delay_cycles(0);

    if(uart_receive_complete()){
      fgets(strr,64,stdin);
      strr[strlen(strr)-1]=0; // Chomp the newline.
//      fputs("\r\n len =",stdout);
//      fputs(itoa(stdin->len,tmpbuf,10),stdout);
// NOTE: fscanf works, but a little different from expected. It will ignore spaces
// and then get the string and return on the next space. IF you use it with a specifier,
// "x %s", or something, it may hang on never getting the input and never getting an EOF.
//
//      fscanf(stdin,"%s",strr);

// Note: printf works as expected, BUT note that it takes a bunch more code space!
//      printf("You wrote: l=%d\r\n [%s] \r\n",stdin->len,strr );
      fwrite("You wrote: [",1,12,stdout);
      fwrite(strr,1,strlen(strr),stdout);
      fwrite("]\r\n",1,3,stdout);
    }
  }
  
  return 0;
}
