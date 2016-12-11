/*
 *  Minimal.cpp
 *
 *  Minimal program for directly talking to an Atmega168
 *
 *  Created on: April 2, 2015
 *      Author: Maurik
 */

#include "avr/io.h"
#include <avr/power.h>

void delay(unsigned int t){
  for (unsigned int i = 0; i < t; ++i) {
    for(unsigned char j=0;j<10;++j){
      asm volatile(
        "nop  \n\t"
        "nop  \n\t"
        "nop  \n\t"
        "nop  \n\t"
        "nop  \n\t"
        "nop  \n\t"
        "nop  \n\t"
        "nop  \n\t"
      );
    }
  }
}

void short_wait(unsigned char t){
  for(unsigned char i =0; i < t; ++i){
    asm volatile(
                 "nop  \n\t"
                 "nop  \n\t"
                 );
  }
}


void dim_on(unsigned char led_id,unsigned int on_time,unsigned bright){
  
  for(unsigned int i=0; i < on_time;++i){
    for(unsigned char j=0;j<2;++j){
      PORTB = 0;
      short_wait(255-bright);
      PORTB = led_id;
      short_wait(bright);
    }
  }
  PORTB = 0;
}


int main (void) {

  __asm__ __volatile__ ("sei" ::: "memory"); // sei();  // Enable interrupts:  __asm__ __volatile__ ("sei" ::: "memory");
  // Set interrupts enabled.

  power_adc_disable(); // Disable the ADC to save power.
  
  // CLKPR = 0b00000101 ; // Divide the CPU clock by 32, slow everything down, and save power
  
  //  DDRB = 0x02;                   // Scope to PB1
//  PORTB = 0x02;   // ON

#define LED1 0x08
#define LED2 0x10
  
  DDRB  = 0x18; // 0b00011000; // PB3 & PB4 are output.
  PORTB = LED1; // ON
  PORTB = 0;
  dim_on(LED2,100,10);
  PORTB = 0;
  dim_on(LED1,100,20);
  PORTB = 0;
  dim_on(LED2,100,40);
  PORTB = 0;
  dim_on(LED1,100,60);
  PORTB = 0;
  dim_on(LED2,100,80);
  PORTB = 0;
  dim_on(LED1,100,120);
  PORTB = 0;
  dim_on(LED2,100,128);
  PORTB = 0;
  dim_on(LED1 + LED2,100,180);
  PORTB = 0;
  delay(500U);
  
  while (1) {
    PORTB = 0; // OFF
    delay(20000U);
    for(unsigned int i=0;i<200; ++i){
      dim_on(LED1 + LED2,1,i); // PB3 and PB4 fade in
    }
    for(unsigned int i=200;i>0; --i){
      dim_on(LED1 + LED2,1,i); // PB3 and PB4 fade out
    }
    
    PORTB = 0; // ALL OFF
  }
}

