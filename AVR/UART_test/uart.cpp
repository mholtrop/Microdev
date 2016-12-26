#include <stdint.h>
#include <stdio.h>
#include <util/delay.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "uart.hpp"

// File handle for the uart
FILE __uart_io;

// Buffers for the buffered uart
char __tx_uart_buf[TX_UART_BUF_SIZE];
char __rx_uart_buf[RX_UART_BUF_SIZE];

// Buffer pointers, to make a circular buffer.
__tx_uart_buf_index_t __tx_uart_buf_head;
__tx_uart_buf_index_t __tx_uart_buf_tail;
__tx_uart_buf_index_t __rx_uart_buf_head;
__tx_uart_buf_index_t __rx_uart_buf_tail;


// Global flag, telling any code that a complete string was received.
bool __rx_uart_receive_complete;

void uart_init(unsigned char mode) {
// Setup the UART and the two FILE objects for input and output.
// This part uses the fixed BAUD set in the header. "setbaud.h" does the calculation.

#include <util/setbaud.h>

  UBRR0H = UBRRH_VALUE;
  UBRR0L = UBRRL_VALUE;
    
#if USE_2X
  UCSR0A |= _BV(U2X0);
#else
  UCSR0A &= ~(_BV(U2X0));
#endif

  uart_init2(mode); // Second stage init.
}

#if UART_ALLOW_BAUD == 1
void uart_init_baud(unsigned char mode,uint32_t baud){
  // Set baudrate explicitly from the program. Same calculation as setbaud.h
  uint16_t ubbr_value=(((F_CPU) + 8UL * (baud)) / (16UL * (baud)) -1UL);
  
  if(100UL*(F_CPU) >  (16 * ((UBRR_VALUE) + 1)) * (100 * (baud) + (baud) * (BAUD_TOL)) ||
     100 * (F_CPU) <  (16 * ((UBRR_VALUE) + 1)) * (100 * (baud) - (baud) * (BAUD_TOL)) ){
    // Outside of tolerance, use USE2X formula
    ubbr_value  = (((F_CPU) + 4UL * (baud)) / (8UL * (baud)) -1UL);
    UCSR0A |= _BV(U2X0);
  }else{
    UCSR0A &= ~(_BV(U2X0));
  }
  
  UBRR0H = (ubbr_value >> 8);
  UBRR0L = (ubbr_value & 0xFF);
 
  uart_init2(mode); // Second stage init.
}
#else
#warning Baudrate is fixed at BAUD.
void uart_init_baud(unsigned char mode,uint32_t baud){
  uart_init(mode);
}
#endif

void uart_init2(unsigned char mode){
// The second part of the init sets up the UART and links either the
// blocking (mode=0) or the buffered (mode=1) or a combo with the FILE
//
  UCSR0C  = _BV(UCSZ01) | _BV(UCSZ00); // 8-bit data, no parity, 1 stop bit = 8N1 = 0x06
  UCSR0B  = _BV(RXEN0) | _BV(TXEN0);   // Enable RX and TX

  if( (mode&0x01) == 0){ // unbuffered transmission, which is better for debugging.
    __uart_io.put = uart_putchar;
  }else{                       // Buffered output, better for 
    __uart_io.put = uart_putchar_buffered;
  }
    
  if( (mode&0x02) == 0){
    __uart_io.get = uart_getchar;
  }else{
    __uart_io.get = uart_getchar_buffered;
    UCSR0B |= _BV(RXCIE0);                // Receive interrupt enabled.
  }
  
  __uart_io.flags=  _FDEV_SETUP_RW;
  
  stdout = &__uart_io;
  stdin  = &__uart_io;
  
  __tx_uart_buf_head = 0;
  __tx_uart_buf_tail = 0;
  __rx_uart_buf_head = 0;
  __rx_uart_buf_tail = 0;
  __rx_uart_receive_complete=false;  // No, no <CR> received yet.
  
}


void uart_tx_buffer_flush(void){
  // Block until the buffer is empty.
  while(UCSR0B & _BV(UDRIE0));          // As long as the interrupt is enabled, we have stuff in the buffer.
  loop_until_bit_is_set(UCSR0A, UDRE0); // And wait for the shift register buffer to be empty.
  loop_until_bit_is_set(UCSR0A, TXC0 ); // And the last bits are shifted out.
}

int uart_putchar(char c, FILE *stream) {
  // This is a blocking version, it will wait until the UART is ready
  // and then output the character. This means it is synchronous.
  loop_until_bit_is_set(UCSR0A, UDRE0);
  UDR0 = c;
  return 1;
}

int uart_putchar_buffered(char c, FILE *stream){
  // Just put stuff in the circular buffer.
  // If it is full, well, too bad, overwrite and get garbage out :-)
  // This is asynchronous, we have the interrupt routine empty the buffer for us.
  // The program keeps going, but can be interrupted to full the UART again.
  if(__tx_uart_buf_head == __tx_uart_buf_tail && bit_is_set(UCSR0A,UDRE0) ){
    // The buffer was empty, AND the UART TX (UCSR0A _BV(UDRE0) is empty.)
    // UCSR0A = USART Control and status register A
    // UDRE0  = USART Data Register Empty.
    UDR0 = c;            // Send the char out right away.
    UCSR0A|=  _BV(TXC0); // Clear the transmit complete bit.
    return 1;            // Yup, it was send out.
  }

  // Here we check if adding this char will cause a buffer full.
  // This must be done *before* actually incrementing the head,
  // since we could be interrupted in the middle by the USART_UDRE_vect routine.
  __tx_uart_buf_index_t next_buffer_loc = (__tx_uart_buf_head + 1) % TX_UART_BUF_SIZE;

  
  if(next_buffer_loc == __tx_uart_buf_tail ){  // Only one last spot left in the buffer!
#if UART_BLOCK_ON_OVERFLOW == 1
    while(next_buffer_loc == __tx_uart_buf_tail){;} // Block until there is space cleared.
#else  
    
#warning Not blocking on overflow.
    // We are not allowed to block. We can either overwrite or we can discard.
    // Discarding seems safer. Mark the occurrence of the overflow by replacing the
    // last bit in the buffer by an @.
    if( __tx_uart_buf_head != 0){
      __tx_uart_buf[__tx_uart_buf_head-1] = '@'; // Last char is overwritten.
    }else{
      __tx_uart_buf[TX_UART_BUF_SIZE-1] = '@';
    }
    // HEAD is NOT increased (then buffer would look empty! So return.
    return 1;
#endif
  }

  __tx_uart_buf[__tx_uart_buf_head] = c;
  __tx_uart_buf_head = next_buffer_loc;

  
  UCSR0B |= _BV(UDRIE0); // Set USART Data Register Empty Interrupt Enable,
                         // so when the current transmit is complete, the interupt will send this one.
  return 1;
}

int uart_getchar(FILE *stream) {
// This is a blocking version. It will sit and wair until the UART has
// a character for us.
    loop_until_bit_is_set(UCSR0A, RXC0);
    return UDR0;
}

int uart_getchar_buffered(FILE *stream) {
// This is the non blocking version. It looks for data from the rx buffer.
// if the rx buffer is empty, return 0
  if(__rx_uart_buf_head > __rx_uart_buf_tail){
    char c=__rx_uart_buf[ __rx_uart_buf_tail ];
    __rx_uart_buf_tail = (__rx_uart_buf_tail+1)%RX_UART_BUF_SIZE;
    return c;
  }else{
    __rx_uart_receive_complete=false;
    return 0;
  }
  return -1;
}

int  uart_receive_str(char *str,int len){
  // Get the string from the buffer.
  
  return len;
}

bool uart_receive_complete(void){
  return (__rx_uart_receive_complete);
}

/////////////////////////////////////////////////
// INTERRUPT ROUTINES                         ///
/////////////////////////////////////////////////

ISR(USART_UDRE_vect){
  // This is called when the UART is EMPTY, so we can send the next char.
  UDR0 = __tx_uart_buf[__tx_uart_buf_tail]; // Send out the next char.
  __tx_uart_buf_tail = (__tx_uart_buf_tail + 1) % TX_UART_BUF_SIZE; // increment the tail
  // UCSR0A |= _BV(TXC0); // Clear the transmit complete bit.
  
  if(__tx_uart_buf_tail == __tx_uart_buf_head){ // We caught up, no next one to send.
    UCSR0B &= ~_BV(UDRIE0);
  }
}

ISR(USART_RX_vect){
  PORTC = PORTC ^ _BV(0);
  // Receive something interrupt. The UART has an input ready for us.
  char c = UDR0; // Read it right away.
  __rx_uart_buf_index_t rx_i = (__rx_uart_buf_head + 1) % RX_UART_BUF_SIZE;
  if(rx_i != __rx_uart_buf_tail){ // Buffer still has space.
    if( c == '\n' || c == 13 /* ^m */ || c == '\r'){
      __rx_uart_receive_complete=true; // Tell the code that we have something complete.
    }else{
      __rx_uart_buf[__rx_uart_buf_head] = c;
      __rx_uart_buf_head = rx_i;
    }
#if UART_ECHO == 1
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c; // Echo it back out right away.
#endif
  }else{ // Buffer is full.
    // Now what? - we *could* block, or we can throw the input away.
    // If we throw it away, do we throw the entire buffer?
    __rx_uart_receive_complete=true; // Pretend you got a return, so process the buffer.
  }
}

