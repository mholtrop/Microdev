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
#include "LEDBall.hpp"
#include <avr/pgmspace.h>   // To use PSTR()

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
//  unsigned long start,stop;
  char c=' ';

  sei();
  uart_init(0x3,BAUD);
  timer1_init();
  
  fputs_P(PSTR("\n\rLEDBall Code V0.3.3\r\n"),stdout);
  flush();

  LEDBall leds(L_PIN1, &L_PORT1, NEO_GRB + NEO_KHZ800);

  leds.clear();
  fputs_P(PSTR("alloc\r\n"),stdout);
  flush();
  leds.alloc_store();
//  TCNT1 = 0;
//  start = TCNT1;
//  leds.copy_from_store(); //  ~44 us
//  leds.swap_store();        // ~193 us
  leds.setShowAll(255,0,0);
//  stop  = TCNT1;
//  printf("1: %ld us \r\n",(stop-start)/2);
  
//  TCNT1 = 0;
//  start = TCNT1;
//  leds.show();  // ~ 16510 us
//  stop  = TCNT1;
//  printf("3: %ld us \r\n",(stop-start)/2);
  _delay_ms(150);
  
//  start = TCNT1;
  leds.setShowAll(0,0,0);
//  stop  = TCNT1;
//  printf("2: %ld us \r\n",(stop-start)/2);
  _delay_ms(300);
  
//  start = TCNT1;
//  leds.show();
//  stop  = TCNT1;
//  printf("3: %ld us \r\n",(stop-start)/2);
//  _delay_ms(100);
//
  leds.setShowAll(0,255,0);
  _delay_ms(150);
  leds.setShowAll(0,0,0);
  _delay_ms(300);
  leds.setShowAll(0,0,255);
  _delay_ms(150);
  leds.setShowAll(0,0,0);
  _delay_ms(300);
  leds.clear();
  leds.show();
  
  fputs_P(PSTR("off\r\n"),stdout);
  while (1) {
    //    leds.copy_to_store();
    //    leds.copy_from_store();
    if (uart_receive_complete()) {
//      char strr[64];
//      fgets(strr,64,stdin);
//      c = strr[0];
      c= fgetc(stdin);
      putc(c,stdout);
      switch( c ){
        uint8_t loc,r,g,b;
        case 'B':
          leds.setShowAll(0,0,255);
          break;
        case 'G':
          leds.setShowAll(0,255,0);
          break;
        case 'O': // Off
          leds.clear();
          leds.show();
          break;
        case 'P':
          leds.setShowAll(255,0,255);
          break;
        case 'R':
          leds.setShowAll(255,0,0);
          break;
        case 'W': // Warm White
          leds.setBaseColor(255,110,30);
          leds.show();
          break;
        case 's':
          leds.show();
          break;
        case 'x':  // Set indiviual pixel at x in main array
          scanf("%hhu %hhu %hhu %hhu",&loc,&r,&g,&b);
          leds.setPixelColor(loc,r,g,b);
          break;
        case 'y':  // Set indiviual pixel at x in alt array
          scanf("%hhu %hhu %hhu %hhu",&loc,&r,&g,&b);
          leds.setPixelColor(loc,r,g,b);
          break;
        case 'z':
          leds.swap_store();
          break;
      }
    }
	}

  return 0; // never reached
}
