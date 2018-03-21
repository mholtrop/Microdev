/*
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>   // To use PSTR()

//
// By including the uart.cpp (and not uart.h) we can control the behavior of the
// UART code with the #define statements. In that case, do not include the uart.cpp
// in the compile and link stage.
//
// Ofcourse, you can also edit the uart.h to get the behavior you want, and then
// do a normal compile of uart.cpp and link with main.cpp
//

#include "uart.hpp"
#include <util/delay.h>

int main(void) {

  sei();                    // Enable all interrupts.
  uart_init(0x3,BAUD);
  
//  DDRB  |= _BV(1);
//  PORTB |= _BV(1);
//
//  DDRB  |= _BV(2);
//  PORTB |= _BV(2);
//  _delay_ms(500);
//  PORTB &= ~_BV(1);
//  PORTB &= ~_BV(2);
  
  
  UART_CTS_PORT |= _BV(UART_CTS_PIN);
  fputs_P(PSTR("\r\nUART test code\r\n"),stdout);
  flush();
  _delay_ms(4000);
  fputs_P(PSTR("\r\nstart\r\n"),stdout);
  UART_CTS_PORT &= ~_BV(UART_CTS_PIN);
  uart_tx_buffer_flush();
//  fputs("This is a really long sentence that will still fit in buffer\r\n",stdout);
//  uart_tx_buffer_flush(); // Flush, i.e. block, else we will overflow on "Hello World."

  char strr[64];
//  char tmpbuf[10];
//  uint32_t i;
  
  int count=0;
  
  while(1) {

    if(uart_receive_complete()){
 
      count++;

      fgets(strr,64,stdin);
//      uart_print_rx_buffer();
//      int jj;
//      for(jj=0;jj<64;jj++){
//        char c = fgetc(stdin);
//        strr[jj]=c;
//        if( c== 0) break;
//      }
//      if(jj>=63) fwrite("Input overflow\r\n",1,16,stdout);
//      strr[strlen(strr)-1]=0; // Chomp the newline.
//      fputs("\r\n len =",stdout);
//      fputs(itoa(stdin->len,tmpbuf,10),stdout);
// NOTE: fscanf works, but a little different from expected. It will ignore spaces
// and then get the string and return on the next space. IF you use it with a specifier,
// "x %s", or something, it may hang on never getting the input and never getting an EOF.
//
//        fscanf(stdin,"%s",strr);

// Note: printf works as expected, BUT note that it takes a bunch more code space!
      
      printf("i:%3d urc=%1u [%s]\r\n",count,__rx_uart_receive_complete,strr );


      
      //        fwrite("[",1,1,stdout);
//        fwrite(strr,1,strlen(strr),stdout);
//        fwrite("]\r\n",1,1,stdout);
//        printf("(0x%hhx)",strr[0]);
//        fwrite("<",1,1,stdout);
//      fwrite(&strr[1],1,1,stdout);
//      fwrite(">",1,1,stdout);
//      printf(" c=%d,%d\n",count,strlen(strr));
//      fwrite("]\r\n",1,3,stdout);
//        printf("l:%d %c %c",strlen(strr),strr[0],strr[strlen(strr)-1]);
    }
    
    _delay_ms(500.);
  }
  
  return 0;
}
