/**
 *  UART Code for use with avr-lib printf
 *
 * This is code for the AVR UART (USART actually) on the ATmega328/168 type chips.
 * It will probably work on other chips as well, but you may need to change the registers used, i.e.
 * UCSR0A may need to be changed to UCSRA or UCSRnA where n is the number of the uart you want to use.
 * I made not effort to make this general, since it is an easy switch to another uart if you wish.
 * 
 * Some control of the code is done with #define statements in this header to keep the code small. It
 * uses less than 1.5k, quite a bit less than Arduino HardwareSerial, though that code has different functionality.
 * To be fair, if you add in the printf functionality, code size goes to ~ 3k.
 *
 * For really small code, turn off the buffering with the UART_USE_BUFFERS flag, but loose the buffers and 
 * interrupt routines
 *
 * If you keep the buffers, then bit0 if the mode flags disables (0) or enables (1) the transmit (TX) buffering.
 * and bit1 does so for the receive (RX). It can be useful when debugging code to turn off the buffers temporarily.
 *
 * Author: Maurik Holtrop
 * Date:   2016-12-26
 *
 * Yes, this code is 'pedagogically' commented. Deal with it.
**/

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
// Another detail is how fgets works.
// The AVR libc version of fgets, will read from the stream with getc until it reaches \n OR the string is full.
// It then ALWAYS adds a 0 at the end of the string, see below.
// This means that we need to leave the \n in the string, but NOT add a 0 to terminate the string.
//
//fgets(char *str, int size, FILE *stream)
//{
//  char *cp;
//  int c;
//
//  if ((stream->flags & __SRD) == 0 || size <= 0)
//    return NULL;
//
//  size--;
//  for (c = 0, cp = str; c != '\n' && size > 0; size--, cp++) {
//    if ((c = getc(stream)) == EOF)
//      return NULL;
//    *cp = (char)c;
//  }
//  *cp = '\0';
//
//  return str;
//}
//

#ifndef __UART_CPP__
#define __UART_CPP__

#include <stdio.h>

#ifndef F_CPU
#error This code will not work without properly defining F_CPU
#endif

#ifndef BAUD
#error This code will not work properly without defining BAUD
#endif

//
// Define varipus behaviors of the UART code.
//

#ifndef UART_ALLOW_BAUD
#define UART_ALLOW_BAUD 0          // Set to 0 fixes BAUD rate and saves space (320 bytes)
#define BAUD_TOL 2                 // Otherwise set in util/setbaud.h
#endif

#ifndef UART_ECHO
#define UART_ECHO 0                // Set to 1 if you want each keystroke echoed back immediately.
#endif

#ifndef UART_BLOCK_ON_OVERFLOW
#define UART_BLOCK_ON_OVERFLOW 0   
// Set to one if you want to block on overflow, so no chars go missing.
// Note that this is problematic if you depend on the interrupt routines to not take much time.
// If you set this to 0 then on overflow an @ is printed and characters are discarded from the buffer.
#endif

#ifndef UART_USE_BUFFERS
#define UART_USE_BUFFERS  1         // Set to 1 if you want buffered code; 0 no buffers, so this all gets smaller.
#endif

#ifndef UART_CTS_ENABLE
#define UART_CTS_ENABLE 0          // Set to 1 if CTS can be used to control the RECEIVING flow.
#endif

#define UART_CTS_DDR   DDRD
#define UART_CTS_PORT  PORTD
#define UART_CTS_PIN   2           // PD2 = pin 4 used for hardware handshake.

#define UART_EAT_CHAR  '\n'           // Character that should not be processed at all.
#define UART_EOL       '\r'        // End Of Line character. \r or BBB

#if UART_USE_BUFFERS == 1
//
// Buffer sizes. Note that these go into the RAM, which is very limited on most chips.
//
#ifndef TX_UART_BUF_SIZE
#define TX_UART_BUF_SIZE 64
#endif
#ifndef RX_UART_BUF_SIZE
#define RX_UART_BUF_SIZE 64
#endif

#if (TX_UART_BUF_SIZE>256)
typedef uint16_t __tx_uart_buf_index_t;
#else
typedef uint8_t __tx_uart_buf_index_t;
#endif

#if (RX_UART_BUF_SIZE>256)
typedef uint16_t __rx_uart_buf_index_t;
#else
typedef uint8_t __rx_uart_buf_index_t;
#endif

///
extern __tx_uart_buf_index_t __tx_uart_buf_head;
extern __tx_uart_buf_index_t __tx_uart_buf_tail;
extern __rx_uart_buf_index_t __rx_uart_buf_head;
extern __rx_uart_buf_index_t __rx_uart_buf_tail;

extern volatile uint16_t __rx_uart_receive_complete;

uint16_t uart_receive_complete(void);
void uart_empty_and_reset(void);

#else

inline uint16_t uart_receive_complete(void){
  return(1);
}

#endif  // UART_USE_BUFFERS
///

void uart_tx_buffer_flush();       //! Flush the buffers. This is a blocking operation for transmit.
#ifndef flush
#define flush uart_tx_buffer_flush
#endif
void uart_init(unsigned char mode=0,uint32_t baud=BAUD); //! Initialize the UART code.
uint16_t uart_receive_complete(void);  //! Return the receive complete flag. Indicates a return (RET) or newline '\n' on the RX.

int  uart_putchar_buffered(char c, FILE *stream); //! Internal routine, buffered put.
int  uart_putchar(char c, FILE *stream);          //! Internal routine, unbuffered put.
int  uart_getchar_buffered(FILE *stream);         //! Internal routing, buffered get.
int  uart_getchar(FILE *stream);                  //! Internal routine, unbuffered get.
void uart_print_rx_buffer(void);

#endif
