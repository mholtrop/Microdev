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

volatile uint32_t time_s;

ISR(TIMER1_COMPA_vect) {
  time_s++;
  TCNT1=0;
}


void timer1_init(void){
  
  PORTD ^= _BV(7); // Toggle the LED on Pin 13
  
  // Setup timer1 to keep track of microseconds.
  TCCR1A = 0; // Normal operation (no compare, no PWM)
 //  In Clear Timer on Compare or CTC mode (WGM02:0 = 2), the OCR0A Register is used to manipulate
//  the counter resolution. In CTC mode the counter is cleared to zero when the counter value (TCNT0) matches the OCR0A.
//  Page 98
//  TCCR1B |= _BV(WGM12); // This should saves us from resetting the timer in the interrupt routine. Doesn't work?
  
  OCR1A = 62500;        // Compare A once per second.

  TIMSK1 = _BV(OCIE1A);      // Generate interrupt on compare A match.
  
//  TCCR1A |= (1 << COM1A0); // Enable timer 1 Compare Output channel A in toggle mode (monitor)
  
  TCCR1C = 0; // Normal
  TCNT1 = 0;  // Start the timer at zero.
  
  TCCR1B = (_BV(CS12)) ; // Start timer: clk_io/256 - system clock = 16 MHz, so 16 us per count
  
  time_s = 0;
}

uint32_t gettime_ms(void){ // Return approximate ms since timer1_init
  return(time_s*1000+TCNT1*0.016);
}

int main(void) {

  unsigned long start,stop;
  uint32_t rot_delay=0;
  uint32_t rot_delay_time=0;
  uint32_t ch_delay=0;
  uint32_t ch_delay_time=0;
  
  char c=' ';

  DDRD |= _BV(7);
  DDRB |= _BV(1);
  
  sei();
  uart_init(0x3,BAUD);
  timer1_init();

#if UART_CTS_ENABLE == 1
  UART_CTS_PORT |= _BV(UART_CTS_PIN); // Send CTS pin high
#endif
  

  
  fputs_P(PSTR("\n\rLEDBall Code V0.4.0\r\n"),stdout);
  flush();

  LEDBall leds(L_PIN1, &L_PORT1, NEO_GRB + NEO_KHZ800);

  leds.clear();
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
  
  PORTD ^= _BV(7); // Toggle the LED on Pin 13

#if UART_CTS_ENABLE == 1
  UART_CTS_PORT &= ~_BV(UART_CTS_PIN); // Send CTS pin low
#endif

  
  while (1) {
    //    leds.copy_to_store();
    //    leds.copy_from_store();
    if (uart_receive_complete()) {
//      char strr[64];
//      fgets(strr,64,stdin);
//      c = strr[0];
      c= fgetc(stdin);
//      flush();
//      putc(c,stdout);
      switch( c ){
        uint8_t loc,x,y,r,g,b;
//        uint32_t tmp_c;
        float ff;
          // "Instant" settings do not change the pixel array.
          // A "show()" after these settings will restore the state of the LEDS.
        case 'B': // Instant set Blue
          leds.setShowAll(0,0,255);
          break;
        case 'G': // Instant set Green
          leds.setShowAll(0,255,0);
          break;
        case 'O': // Instant off
          leds.setShowAll(0,0,0);
	  leds.clear();
	  ch_delay=0;
	  rot_delay=0;
          break;
        case 'P': // Instant Purple
          leds.setShowAll(255,0,255);
          break;
        case 'R': // Instant Red
          leds.setShowAll(255,0,0);
          break;
        case 'S': // status
          printf_P(PSTR("q: %lu \r\n"),&ch_delay);
          printf_P(PSTR("r: %lu \r\n"),&rot_delay);
        case 'T':
          printf_P(PSTR("T: %6lu.%06lu s \r\n"),time_s,(16UL*TCNT1));
          break;
        case 'W': // Instant Warm White
          leds.setShowAll(255,110,30);
          break;
        case '+': // Increase brightness of base by 5% for each +
          ff=1.05;
          while(fgetc(stdin)=='+') ff *= 1.05;
          leds.multBaseColor(ff,ff,ff);
          break;
        case '-': // Decrease brightness of base by 5% for each -
          ff = 0.95;
          while(fgetc(stdin)=='-') ff *= 0.95;
          leds.multBaseColor(ff,ff,ff);
          break;
        case 'b': // Set base color to r,g,b
          scanf("%hhu %hhu %hhu",&r,&g,&b);
          leds.setBaseColor(r,g,b);
          break;
        case 'c':
          leds.clear();
          break;
        case 'd':
          printf("\r\n");
          for(x=0;x<MAX_X;++x) for(y=0;y<MAX_Y;++y){
            printf("(%3hhu,%3hhu) 0x%8lX \r\n",x,y,leds.getPixelColorXY(x,y));
            flush();
          }
          start= TCNT1;
          leds.rotate_z(1);
          stop = TCNT1;
          printf("Time= %lu\r\n",stop-start);
//          leds.linear_chase();
          leds.show();
          break;
        case 'q':
          scanf("%lu",&ch_delay);
          ch_delay_time = gettime_ms();
          break;
        case 'r':
          scanf("%lu",&rot_delay);
          rot_delay_time = gettime_ms();
          break;
        case 's':
          leds.show();
          break;
        case 't':
          uint16_t t;
          scanf("%u",&t);
          leds.Temp_to_RGB(t,&r,&g,&b);
          leds.setBaseColor(r,g,b);
          break;
        case 'w': // Set base color to warm white
          leds.setBaseColor(255,110,30);
          break;
        case 'i':  // Set indiviual pixel at loc in main array
          scanf("%hhu %hhu %hhu %hhu",&loc,&r,&g,&b);
          leds.setPixelColor(loc,r,g,b);
          break;
        case 'j':  // Set indiviual pixel at loc in alt array
          scanf("%hhu %hhu %hhu %hhu",&loc,&r,&g,&b);
          leds.setPixelColor(loc,r,g,b,leds.pixels_st);
          break;
        case 'x':  // Set indiviual pixel at x,y in main array
          scanf("%hhu %hhu %hhu %hhu %hhu",&x,&y,&r,&g,&b);
          // printf("x %hhu %hhu %hhu %hhu %hhu",x,y,r,g,b);
          leds.setPixelColorXY(x,y,r,g,b);
          break;
        case 'y':  // Set indiviual pixel at x,y in alt array
          scanf("%hhu %hhu %hhu %hhu %hhu",&x,&y,&r,&g,&b);
          leds.setPixelColorXY(x,y,r,g,b,leds.pixels_st);
          break;
        case 'z':
          leds.swap_store();
          break;
      }
    }
    
    if(rot_delay>0 && (gettime_ms() - rot_delay_time) > rot_delay ){ // Time to rotate the patern.
      rot_delay_time = gettime_ms();
      leds.rotate_z(1);
      leds.show();
    }
    
    if(ch_delay>0 && (gettime_ms() - ch_delay_time) > ch_delay ){ // Time to rotate the patern.
      ch_delay_time = gettime_ms();
      leds.linear_chase();
      leds.show();
    }
    
  }

  return 0; // never reached
}
