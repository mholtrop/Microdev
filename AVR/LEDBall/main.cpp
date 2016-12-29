/*
 * LED Ball
 *
 * main.cpp
 *
 *  Created on: Nov 11, 2016
 *      Author: Maurik Holtrop
 */
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "uart.hpp"        // This must go before Arduino.h, otherwise it will include the HardwareSerial.h
#define HardwareSerial_h   // Don't led Arduino grab the serial.
#include "LEDBall.hpp"

#define L_PIN1  4
#define L_PORT1 PORTD      // Output is on PD4, pin 6

void timer1_init(void){
  // Setup timer1 to keep track of microseconds.
  TCCR1A = 0; // Normal operation (no compare, no PWM)
  TCCR1B = (_BV(CS11)) ; // clk_io/8 - system clock = 16 MHz, so 2 MHz clock.
  TCCR1C = 0; // Normal
  TCNT1 = 0;  // Start the timer at zero.
}

int main(void) {
  unsigned long start,stop;

  uart_init(0x0,BAUD);
  timer1_init();
  
  printf("\n\rHello. This is LEDBall Code V0.3.1\r\n");
  flush();
  DDRD |= _BV(4) | _BV(3);  // Pin 6 of the chip is on port PD4
  PORTD = 0;
  PORTD |= _BV(3);
  start = TCNT1;
  printf("Starting with micros = %ld\r\n",start);
//  Serial.println(start);
  
  PORTD &= ~_BV(3);
  LEDBall leds(L_PIN1, &L_PORT1, NEO_GRB + NEO_KHZ800);

  start = TCNT1;
  printf("Now we have micros   = %ld\r\n",start);

  printf("Red...\r\n");
  flush();
  start = TCNT1;
  leds.setBaseColor(255,0,0,leds.pixels);
  stop  = TCNT1;
//
  printf("1: %ld us\r\n",(stop - start)/2);
  flush();

  TCNT1 = 0;
  start = TCNT1;
  leds.show();
  stop  = TCNT1;
  printf("2: %ld us \r\n",(stop-start)/2);

  leds.setBaseColor(0,0,0,leds.pixels);
  _delay_ms(500);
  leds.show();
  printf("off\r\n");
  
////  _delay_ms(1000);
////  Serial.println("All Green...");
////  leds.setBaseColor(0,255,0);
////  leds.show();
////  _delay_ms(500);
////  leds.setBaseColor(0,0,0);
////  leds.show();
////  _delay_ms(1000);
////  Serial.println("All Blue...");
////  leds.setBaseColor(0,0,255);
////  leds.show();
////  _delay_ms(500);
////  leds.setBaseColor(0,0,0);
////  leds.show();
////  _delay_ms(1000);
////  Serial.println("All White...");
////  leds.setBaseColor(255,255,255);
////  leds.show();
////  _delay_ms(500);
////  leds.setBaseColor(0,0,0);
////  leds.show();
////  Serial.println("Off, ready for input.");
  
  while (1) {
//    leds.copy_to_store();
//    leds.copy_from_store();
//    if (Serial.available()) {
//      char s_in = Serial.read();
//     Serial.print(s_in);
//    }
	}

  return 0; // never reached
}
