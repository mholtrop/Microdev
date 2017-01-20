#include <stdint.h>
#include <stdio.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "uart.hpp"

// File handle for the uart
FILE __uart_io;

#if UART_USE_BUFFERS == 1

// Buffers for the buffered uart
char __tx_uart_buf[TX_UART_BUF_SIZE];
char __rx_uart_buf[RX_UART_BUF_SIZE];

// Buffer pointers, to make a circular buffer.
__tx_uart_buf_index_t __tx_uart_buf_head;
__tx_uart_buf_index_t __tx_uart_buf_tail;
__rx_uart_buf_index_t __rx_uart_buf_head;
__rx_uart_buf_index_t __rx_uart_buf_tail;

volatile uint16_t __rx_uart_receive_complete; // Global flag, count the number of <LF> in the buffer, i.e. how many complete strings.

uint16_t uart_receive_complete(void){
  return (__rx_uart_receive_complete);
}

void uart_empty_and_reset(void){
  __rx_uart_buf_tail = __rx_uart_buf_head;    // Skip any remaining stuff in buffer.
  __rx_uart_receive_complete = 0; // Ready for new.
}

__rx_uart_buf_index_t uart_get_rx_buf_space_left(void){
  // Return the number of remaining bytes before the buffer is full.
  // Note that in a ring buffer one location is always "empty", which serves
  // to indicate the end of buffer. The only way around that is to keep a separate count
  // in a variable which holds the value of how much space is left in the buffer.
  // That is both slower and takes up at least as much space.
  if(__rx_uart_buf_head == __rx_uart_buf_tail) return(RX_UART_BUF_SIZE -1);
  if( __rx_uart_buf_head > __rx_uart_buf_tail ){
    return(RX_UART_BUF_SIZE -1 + __rx_uart_buf_tail - __rx_uart_buf_head);
  }else{
    return(__rx_uart_buf_tail - __rx_uart_buf_head);
  }
}

void debug_echo_char(char c){
// Useful for debugging. Print printable characters and translate common non printable ones.
  if( c >=32 && c <=126){
    uart_putchar(c,NULL);
  }else{
    if(c==0)          uart_putchar('0',NULL);
    else if(c=='\n')  uart_putchar('$',NULL);
    else if(c=='\r')  uart_putchar('<',NULL);
    else if(c== 13)   uart_putchar('*',NULL);
    else              uart_putchar('@',NULL);
  }
}

void uart_print_rx_buffer(void){
  // For debugging, direct print the content of the entire buffer.
  // This is done through a non buffered output on the UART.

  uart_putchar('\r',NULL);
  uart_putchar('\n',NULL);

  char *buf= __rx_uart_buf;
  uint16_t max = RX_UART_BUF_SIZE;
  
  for(uint16_t i=0; i< max; ++i ){
    char c = buf[i];
    debug_echo_char(c);
  }
  uart_putchar('\r',NULL);
  uart_putchar('\n',NULL);
  for(uint16_t i=0; i< max; ++i ){
    if(i== __rx_uart_buf_tail){
      uart_putchar('+',NULL);
    }else if(i== __rx_uart_buf_head){
      uart_putchar('|',NULL);
    }else{
      uart_putchar('-',NULL);
    }
  }
  uart_putchar('\r',NULL);
  uart_putchar('\n',NULL);

}

#endif

#if UART_ALLOW_BAUD == 1
void uart_init(unsigned char mode,uint32_t baud){
  // Setup the UART and the two FILE objects for input and output.
  // Set baudrate explicitly from the program. Same calculation as setbaud.h. Extra 320 bytes if code.
  uint16_t ubbr_value=(((F_CPU) + 8UL * (baud)) / (16UL * (baud)) -1UL);
  
  if(100UL*(F_CPU) >  (16 * ((ubbr_value) + 1)) * (100 * (baud) + (baud) * (BAUD_TOL)) ||
     100 * (F_CPU) <  (16 * ((ubbr_value) + 1)) * (100 * (baud) - (baud) * (BAUD_TOL)) ){
    // Outside of tolerance, use USE2X formula
    ubbr_value  = (((F_CPU) + 4UL * (baud)) / (8UL * (baud)) -1UL);
    UCSR0A |= _BV(U2X0);
  }else{
    UCSR0A &= ~(_BV(U2X0));
  }
  
  UBRR0H = (ubbr_value >> 8);
  UBRR0L = (ubbr_value & 0xFF);
 
#else // UART_ALLOW_BAUD

#warning Baudrate is fixed at BAUD.
void uart_init(unsigned char mode, uint32_t /* baud is discarded */) {
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
  
#endif // UART_ALLOW_BAUD

// The second part of the init sets up the UART and links either the
// blocking (mode=0) or the buffered (mode=1) or a combo with the FILE handle.
//
  UCSR0C  = _BV(UCSZ01) | _BV(UCSZ00); // 8-bit data, no parity, 1 stop bit = 8N1 = 0x06
  UCSR0B  = _BV(RXEN0) | _BV(TXEN0);   // Enable RX and TX

#if UART_USE_BUFFERS == 1
  if( (mode&0x01) == 0){ // unbuffered transmission, which is better for debugging.
    __uart_io.put = uart_putchar;
  }else{                       // Buffered output, better for 
    __uart_io.put = uart_putchar_buffered;
  }
    
  if( (mode&0x02) == 0){
    __uart_io.get = uart_getchar;
    __rx_uart_receive_complete=1;  // Not buffering, so always complete.
  }else{
    __uart_io.get = uart_getchar_buffered;
    UCSR0B |= _BV(RXCIE0);                // Receive interrupt enabled.
    __rx_uart_receive_complete=0;  // No, no <CR> received yet.
#if UART_CTS_ENABLE == 1
    UART_CTS_DDR  |= _BV(UART_CTS_PIN);
    UART_CTS_PORT &= ~_BV(UART_CTS_PIN); // Send CTS pin low, ready for input.
#endif
  }
  __tx_uart_buf_head = 0;
  __tx_uart_buf_tail = 0;
  __rx_uart_buf_head = 0;
  __rx_uart_buf_tail = 0;

#else
  __uart_io.put = uart_putchar;
  __uart_io.get = uart_getchar;
#endif

  __uart_io.flags=  _FDEV_SETUP_RW;
  
  stdout = &__uart_io;
  stdin  = &__uart_io;
  
}


void uart_tx_buffer_flush(void){
  // Block until the buffer is empty.
  // Does nothing for unbuffered output.
#if UART_USE_BUFFERS == 1
  while(UCSR0B & _BV(UDRIE0));          // As long as the interrupt is enabled, we have stuff in the buffer.
  loop_until_bit_is_set(UCSR0A, UDRE0); // And wait for the shift register buffer to be empty.
  loop_until_bit_is_set(UCSR0A, TXC0 ); // And the last bits are shifted out.
#endif
}

int uart_putchar(char c, FILE *stream) {
  // This is a blocking version, it will wait until the UART is ready
  // and then output the character. This means it is synchronous.
  loop_until_bit_is_set(UCSR0A, UDRE0);
  UDR0 = c;
  return 0;
}

#if UART_USE_BUFFERS == 1
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
    return 0;            // Yup, it was send out. 0 = no error in fwrite.
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
    return 0;
#endif
  }

  __tx_uart_buf[__tx_uart_buf_head] = c;
  __tx_uart_buf_head = next_buffer_loc;

  
  UCSR0B |= _BV(UDRIE0); // Set USART Data Register Empty Interrupt Enable,
                         // so when the current transmit is complete, the interupt will send this one.
  return 0;
}

#endif // UART_USE_BUFFERS
  
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

#if UART_USE_BUFFERS == 1
int uart_getchar_buffered(FILE *stream) {
// This is the non blocking version. It looks for data from the rx buffer.
// if the rx buffer is empty, unset __rx_uart_receive_complete and return '\n'
// Note: We return '\n' and not 0 because fgets expects an \n at the end of the line.
//
  if(__rx_uart_buf_tail != __rx_uart_buf_head){ // We did not yet catch up with the head.
    char c=__rx_uart_buf[ __rx_uart_buf_tail ];
    __rx_uart_buf_tail = (__rx_uart_buf_tail + 1) % RX_UART_BUF_SIZE; //next_buffer_loc;
#if UART_CTS_ENABLE == 1
    if( uart_get_rx_buf_space_left() > 6){
      UART_CTS_PORT &= ~_BV(UART_CTS_PIN);  // Send CTS pin low - ready for more input.
    }
#endif
    if(c=='\n' || __rx_uart_buf_tail == __rx_uart_buf_head){ // Now we did catch up with the head.
        if(__rx_uart_receive_complete>0) __rx_uart_receive_complete--;
    }

    return c;
  }else{ // We caught up with the tail. The buffer is empty, so be ready for next receive.
    __rx_uart_receive_complete = 0; // If it is empty, then we cannot have any more <LF>. This is here in case of a miscount.
    return 0; // EOF
  }
  return -1;
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
    // Receive something interrupt. The UART has an input ready for us.
  char c = UDR0; // Read it right away.
//  debug_echo_char(c);

#if UART_ECHO == 1
  loop_until_bit_is_set(UCSR0A, UDRE0);
  UDR0 = c; // Echo it back out right away.
#endif
  
  __rx_uart_buf_index_t rx_i = (__rx_uart_buf_head + 1) % RX_UART_BUF_SIZE; // Next buffer location
    
#if UART_CTS_ENABLE == 1
  if( uart_get_rx_buf_space_left() > 5){              // Need at least 3 spaces.
#else
  if(rx_i != __rx_uart_buf_tail){                     // Buffer still has space.
#endif
    if( c == '\n' ){  // End of line
      __rx_uart_receive_complete++; // Tell the code that we have something complete.
      __rx_uart_buf[__rx_uart_buf_head] = '\n'; // Keep the return character. This indicates to fgets that string is complete.
      __rx_uart_buf_head = rx_i;
    }else if(c == '\r'){ // Do nothing, 'eating' the \r character. If not, it messes up processing.
    }else{
      __rx_uart_buf[__rx_uart_buf_head] = c;
      __rx_uart_buf_head = rx_i;
    }
    
  }else{ // Buffer is full
#if UART_CTS_ENABLE == 1
    UART_CTS_PORT |= _BV(UART_CTS_PIN);  // Send CTS pin high - no more input please
#endif
        // We need to know if there is code looking to read some from the buffer.
    if(__rx_uart_receive_complete>0){          // There is, so let it process some input
#if UART_CTS_ENABLE == 1
      if(rx_i != __rx_uart_buf_tail){                     // Buffer still has space.
        __rx_uart_buf[__rx_uart_buf_head] = c;   // Now buffer is almost full. We count on CTS to prevent this being called again.
        __rx_uart_buf_head = rx_i;
      }else{
        __rx_uart_receive_complete++;           // Pretend you got a return, so start processing the buffer.
        __rx_uart_buf[__rx_uart_buf_head] = 0;  // You loose the last character. LAST COMMAND MAY BE BAD.
      }
#else
      __rx_uart_receive_complete++;           // Pretend you got a return, so start processing the buffer.
      __rx_uart_buf[__rx_uart_buf_head] = 0;  // You loose the last character. LAST COMMAND MAY BE BAD.
#endif
    }else{   // There isn't!! So this is a really long line without a \n = GARBAGE
      __rx_uart_buf_head = __rx_uart_buf_tail; // RESET head, so buf is empty.
      __rx_uart_receive_complete = 0;          // We wiped all the <LF>s in the process.
#if UART_CTS_ENABLE == 1
      UART_CTS_PORT &= ~_BV(UART_CTS_PIN);  // Send CTS pin low - ready for more input.
#endif
    }
  }
}
    
#endif // UART_USE_BUFFERS

