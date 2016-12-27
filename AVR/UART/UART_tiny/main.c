/*
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "uart.h"

int main(void) {

  uart_init(); 
  fputs("UART test code \r\n",stdout);
  char strr[64];

  while(1) {
    fgets(strr,64,stdin);
    fputs(" You wrote: [",stdout);
    fputs(strr,stdout);
    fputs("]\r\n",stdout);
  }
  
  return 0;
}
