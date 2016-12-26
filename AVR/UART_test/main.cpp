/*
 *
 */
 
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "uart.hpp"


unsigned int __timer0_count;

int main(void) {

  sei();                    // Enable all interrupts.
  DDRC = 0x3f;
  
  uart_init_baud(3,115200); // Fully buffered,
  puts("UART test code \r");
  uart_tx_buffer_flush();
  puts("This is a really long sentence that will still fit in buffer.\r");
  uart_tx_buffer_flush(); // Flush, i.e. block, else we will overflow on "Hello World."
  
  while(1) {
    _delay_ms(500);
//    fputc('.',stdout);
    if(uart_receive_complete()){
      printf("receive complete = %d,%d\r\n",__rx_uart_buf_tail,__rx_uart_buf_head);
      char strr[64];
      fgets(strr,64,stdin);
      printf("\r\n\r\n You wrote:\r\n [%s] \r\n", strr );
    }
  }
  
  return 0;
}
