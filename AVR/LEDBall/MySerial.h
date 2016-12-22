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

#ifndef F_CPU
#define F_CPU 16000000
#endif

#ifndef BAUD
#define BAUD 9600
#endif
#include <util/setbaud.h>

#include <stdio.h>
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
//#include "Arduino.h"
//#include "Stream.h"

#define SERIAL_TX_BUFFER_SIZE 64
#define SERIAL_RX_BUFFER_SIZE 64

void init_serial(void);
void uart_putchar(char c, FILE *stream);
char uart_getchar(FILE *stream);

FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);

#endif /* MySerial_hpp */
