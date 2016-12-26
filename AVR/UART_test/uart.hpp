#ifndef __UART_CPP__
#define __UART_CPP__

#include <stdio.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

// Define varipus behaviors of the UART code.

#ifndef BAUD
#define BAUD 115200UL
#define BAUD_TOL 4
#endif

#ifndef UART_ECHO
#define UART_ECHO 0
#endif

#ifndef TX_UART_BUF_SIZE
#define TX_UART_BUF_SIZE 64
#endif

#ifndef RX_UART_BUF_SIZE
#define RX_UART_BUF_SIZE 64
#endif

#ifndef UART_ALLOW_BAUD
#define UART_ALLOW_BAUD 1          // Undefine to save space (320 bytes)
#endif

#ifndef UART_BLOCK_ON_OVERFLOW
#define UART_BLOCK_ON_OVERFLOW 0
#endif

//#define UART_BLOCK_ON_OVERFLOW   // If the output buffers are full, then block instead of discard.

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

///

void uart_tx_buffer_flush();
int  uart_putchar_buffered(char c, FILE *stream);
int  uart_putchar(char c, FILE *stream);
int  uart_getchar_buffered(FILE *stream);
int  uart_getchar(FILE *stream);
int  uart_receive_str(char *str,int len);
void uart_init(unsigned char mode=0);
void uart_init_baud(unsigned char mode=0,uint32_t baud=115200UL);
void uart_init2(unsigned char mode=0);
bool uart_receive_complete(void);

#endif
