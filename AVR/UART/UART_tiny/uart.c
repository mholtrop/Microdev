#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>

#include "uart.h"

FILE __uart_io;

void uart_init(void) {
#include <util/setbaud.h>
  
  UBRR0H = UBRRH_VALUE;
  UBRR0L = UBRRL_VALUE;
  
#if USE_2X
  UCSR0A |= _BV(U2X0);
#warning Set USE_2X
#else
  UCSR0A &= ~(_BV(U2X0));
#endif
  
  UCSR0C  = _BV(UCSZ01) | _BV(UCSZ00); // 8-bit data, no parity, 1 stop bit = 8N1 = 0x06
  UCSR0B  = _BV(RXEN0) | _BV(TXEN0);   // Enable RX and TX

  __uart_io.put = uart_putchar;
  __uart_io.get = uart_getchar;
  __uart_io.flags=  _FDEV_SETUP_RW;
  
  stdout = &__uart_io;
  stdin  = &__uart_io;
  
}


int uart_putchar(char c, FILE *stream) {
  // This is a blocking version, it will wait until the UART is ready
  // and then output the character. This means it is synchronous.
  loop_until_bit_is_set(UCSR0A, UDRE0);
  UDR0 = c;
  return 0;
}

  
int uart_getchar(FILE *stream) {
// This is a blocking version. It will sit and wait until the UART has
// a character for us.
  loop_until_bit_is_set(UCSR0A, RXC0);
  char c= UDR0;
#if UART_ECHO == 1
   loop_until_bit_is_set(UCSR0A, UDRE0);
  UDR0 = c; // Echo it back out right away.
#endif
  if( c == 13 /* ^m */ || c == '\r') c='\n'; // End of line
  return c;
}


