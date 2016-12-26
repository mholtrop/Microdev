/*
 *
 */
 
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/delay.h>

//
// By including the uart.cpp (and not uart.h) we can control the behavior of the
// UART code with the #define statements. In that case, do not include the uart.cpp
// in the compile and link stage.
//
// Ofcourse, you can also edit the uart.h to get the behavior you want, and then
// do a normal compile of uart.cpp and link with main.cpp
//
#define UART_USE_BUFFERS 0
#include "uart.cpp"

unsigned int __timer0_count;

int main(void) {

  sei();                    // Enable all interrupts.
  DDRC = 0x3f;
  
  uart_init(3,115200); // Fully buffered,
  puts("UART test code \r");
  uart_tx_buffer_flush();
  puts("This is a really long sentence that will still fit in buffer.\r");
  uart_tx_buffer_flush(); // Flush, i.e. block, else we will overflow on "Hello World."
  
  while(1) {
    _delay_ms(500);
//    fputc('.',stdout);
    if(uart_receive_complete()){
//      printf("rc = %d,%d\r\n",__rx_uart_buf_tail,__rx_uart_buf_head);
      char strr[64];
      fgets(strr,64,stdin);
      strr[strlen(strr)-1]=0; // Chomp the newline.

// NOTE: fscanf works, but a little different from expected. It will ignore spaces
// and then get the string and return on the next space. IF you use it with a specifier,
// "x %s", or something, it may hang on never getting the input and never getting an EOF.
//
//      fscanf(stdin,"%s",strr);

// Note: printf works as expected, BUT note that it takes a bunch more code space!
//      printf("You wrote: l=%d\r\n [%s] \r\n",stdin->len,strr );
      fputs("You wrote: [",stdout);
      fputs(strr,stdout);
      fputs("]\r\n",stdout);
    }
  }
  
  return 0;
}
