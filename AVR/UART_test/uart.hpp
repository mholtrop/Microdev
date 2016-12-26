/**
 *  UART Code for use with avr-lib printf
 *
 * This is code for the AVR UART (USART actually) on the ATmega328/168 type chips.
 * It will probably work on other chips as well, but you may need to change the registers used, i.e.
 * UCSR0A may need to be changed to UCSRA or UCSRnA where n is the number of the uart you want to use.
 * I made not effort to make this general, since it is an easy switch to another uart if you wish.
 * 
 * Some control of the code is done with #define statements in this header to keep the code small. It
 * uses less than 3k, quite a bit less than Arduino HardwareSerial, though that code has different functionality.
 *
 * For really small code, turn off the buffering with the UART_USE_BUFFERS flag, but loose the buffers and 
 * interrupt routines
 *
 * If you keep the buffers, then bit0 if the mode flags disables (0) or enables (1) the transmit (TX) buffering.
 * and bit1 does so for the receive (RX). It can be useful when debugging code to turn off the buffers temporarily.
 *
 * Author: Maurik Holtrop
 * Date:   2016-12-26
**/
#ifndef __UART_CPP__
#define __UART_CPP__

#include <stdio.h>

#ifndef F_CPU
#define F_CPU 16000000UL   // For the BAUD calculation, we need accurate F_CPU. Define with -DF_CPU=16000000 on compiler command line.
#endif

//
// Define varipus behaviors of the UART code.
//

#ifndef BAUD
#define BAUD 115200UL   // Default BAUD rate.
#define BAUD_TOL 4      // Up the tolerance from 2 to avoid warning from utils/setbaud.h
#endif

#ifndef UART_ALLOW_BAUD
#define UART_ALLOW_BAUD 1          // Set to 0 fixes BAUD rate and saves space (320 bytes)
#endif

#ifndef UART_ECHO
#define UART_ECHO 0                // Set to 1 if you want each keystroke echoed back immediately.
#endif

#ifndef UART_BLOCK_ON_OVERFLOW
#define UART_BLOCK_ON_OVERFLOW 0   // Set to one if you want to block on overflow, so no chars go missing.
// Note that this is problematic if you depend on the interrupt routines to not take much time.
// If you set this to 0 then on overflow an @ is printed and characters are discarded from the buffer.
#endif

#ifndef UART_USE_BUFFERS
#define UART_USE_BUFFERS  1          // Set to 1 if you want buffered code; 0 no buffers, so this all gets smaller.
#endif

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

extern bool __rx_uart_receive_complete;
///
extern __tx_uart_buf_index_t __tx_uart_buf_head;
extern __tx_uart_buf_index_t __tx_uart_buf_tail;
extern __tx_uart_buf_index_t __rx_uart_buf_head;
extern __tx_uart_buf_index_t __rx_uart_buf_tail;

#endif  // UART_USE_BUFFERS
///

void uart_tx_buffer_flush();       //! Flush the buffers. This is a blocking operation for transmit.
void uart_init(unsigned char mode=0,uint32_t baud=115200UL); //! Initialize the UART code.
bool uart_receive_complete(void);  //! Return the receive complete flag. Indicates a return (RET) or newline '\n' on the RX.

int  uart_putchar_buffered(char c, FILE *stream); //! Internal routine, buffered put.
int  uart_putchar(char c, FILE *stream);          //! Internal routine, unbuffered put.
int  uart_getchar_buffered(FILE *stream);         //! Internal routing, buffered get.
int  uart_getchar(FILE *stream);                  //! Internal routine, unbuffered get.


#endif
