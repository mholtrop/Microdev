//
//  MySerial.hpp
//  LEDBall
//
//  Created by Maurik Holtrop on 11/25/16.
//  Copyright © 2016 Holtrop Coding. All rights reserved.
//
//  This class implements a dedicated interrupt driven serial interface using the UART0
//  on the atmega168/328 chip.
//  It borrows a lot from the Arduino HardwareSerial. It differs in that it handles the
//  incoming strings up to a newline, then hands off the command in that string to appropriate handler.
//
#ifndef MySerial_H_
#define MySerial_H_
#define HardwareSerial_h  // This makes sure that the HardwareSerial from Arduino.h is NOT also included.

#include <stdio.h>
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "Arduino.h"
#include "Stream.h"

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif


#define SERIAL_TX_BUFFER_SIZE 64
#define SERIAL_RX_BUFFER_SIZE 64

typedef uint8_t tx_buffer_index_t;
typedef uint8_t rx_buffer_index_t;
#define SERIAL_8N1 0x06

class MySerial : public Stream {
  
public:
  inline MySerial(volatile uint8_t *ubrrh, volatile uint8_t *ubrrl,
                  volatile uint8_t *ucsra, volatile uint8_t *ucsrb,
                  volatile uint8_t *ucsrc, volatile uint8_t *udr);
  void begin(unsigned long baud) { begin(baud, SERIAL_8N1); }
  void begin(unsigned long, uint8_t);
  void end();
  virtual int available(void);
  virtual int peek(void);
  virtual int read(void);
  int availableForWrite(void);
  virtual void flush(void);
  virtual size_t write(uint8_t);
  inline size_t write(unsigned long n) { return write((uint8_t)n); }
  inline size_t write(long n) { return write((uint8_t)n); }
  inline size_t write(unsigned int n) { return write((uint8_t)n); }
  inline size_t write(int n) { return write((uint8_t)n); }
  using Print::write; // pull in write(str) and write(buf, size) from Print
  operator bool() { return true; }
  
  // Interrupt handlers - Not intended to be called externally
  inline void _rx_complete_irq(void);
  void _tx_udr_empty_irq(void);
  
public:
//  uint8_t status;

protected:
  volatile uint8_t * const _ubrrh;
  volatile uint8_t * const _ubrrl;
  volatile uint8_t * const _ucsra;
  volatile uint8_t * const _ucsrb;
  volatile uint8_t * const _ucsrc;
  volatile uint8_t * const _udr;
  // Has any byte been written to the UART since begin()
  bool _written;
  
  volatile rx_buffer_index_t _rx_buffer_head;
  volatile rx_buffer_index_t _rx_buffer_tail;
  volatile tx_buffer_index_t _tx_buffer_head;
  volatile tx_buffer_index_t _tx_buffer_tail;
  
  // Don't put any members after these buffers, since only the first
  // 32 bytes of this struct can be accessed quickly using the ldd
  // instruction.
  unsigned char _rx_buffer[SERIAL_RX_BUFFER_SIZE];
  unsigned char _tx_buffer[SERIAL_TX_BUFFER_SIZE];
  
  
};

// Initialization with the correct registers for the chip.
MySerial::MySerial(
 volatile uint8_t *ubrrh, volatile uint8_t *ubrrl,
 volatile uint8_t *ucsra, volatile uint8_t *ucsrb,
 volatile uint8_t *ucsrc, volatile uint8_t *udr) :
_ubrrh(ubrrh), _ubrrl(ubrrl),
_ucsra(ucsra), _ucsrb(ucsrb), _ucsrc(ucsrc),
_udr(udr),
_rx_buffer_head(0), _rx_buffer_tail(0),
_tx_buffer_head(0), _tx_buffer_tail(0){
  
}

void MySerial::_rx_complete_irq(void)
{
  if (bit_is_clear(*_ucsra, UPE0)) {
    // No Parity error, read byte and store it in the buffer if there is
    // room
    unsigned char c = *_udr;
    rx_buffer_index_t i = (unsigned int)(_rx_buffer_head + 1) % SERIAL_RX_BUFFER_SIZE;
    
    // if we should be storing the received character into the location
    // just before the tail (meaning that the head would advance to the
    // current location of the tail), we're about to overflow the buffer
    // and so we don't write the character or advance the head.
    if (i != _rx_buffer_tail) {
      _rx_buffer[_rx_buffer_head] = c;
      _rx_buffer_head = i;
    }
  } else {
    // Parity error, read byte but discard it
    *_udr;
  };
}

#endif /* MySerial_hpp */
