/*
 * Aniela's Spider
 * main.cpp
 *
 * Crated on: January 18, 2020
 * Author:     Aniela
 * Consultant: Maurik
 */

// #include adds other bits of code, so you have pre-defined functions.
#include "avr/io.h"      // This is always needed, it defines the input/output pins to use.
#include <avr/power.h>   // Defines power_adc_disable() = Disables part of the chip to save energy.

// Define a function that delays processing for a little while, depending on "t"
void delay(unsigned int t){
  for (unsigned int i = 0; i < t; ++i) { // Do the next loop "t" times.
    for(unsigned char j=0;j<10;++j){     // Do the next 10 times. Total of 80 nops.
      asm volatile(
                   // 8 times, do nothing, but it does take 1 clock cycle.
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

// A shorter delay, of 2*t nops.
void short_wait(unsigned char t){
  for(unsigned char i =0; i < t; ++i){
    asm volatile(
                 "nop  \n\t"
                 "nop  \n\t"
                 );
  }
}

//
void dim_on(unsigned char led_id,unsigned int on_time,unsigned bright){
  
  for(unsigned int i=0; i < on_time;++i){
    for(unsigned char j=0;j<2;++j){
      PORTB = 0;                    // All LEDS OFF
      short_wait(255-bright);       // Wait a little (255 - bright) amount
      PORTB = led_id;               // Turn LED ON
      short_wait(bright);           // Wait a little (bright) amount
    }
  }
  PORTB = 0;                        // When done, turn LEDS OFF.
}

// main is where the program actually starts when the power is turned on.
int main (void) {

  power_adc_disable(); // Disable the ADC to save power.
  
  // CLKPR = 0b00000101 ; // Divide the CPU clock by 32, slow everything down, and save power
  
  //  DDRB = 0x02;                   // Scope to PB1
//  PORTB = 0x02;   // ON

#define LED1 (1<<PB3)  // PortB3 = pin2 of the ATtiny85
#define LED2 (1<<PB4)  // PortB4 = pin3 of the ATtiny85
  
  DDRB  = LED1 + LED2 ; // 0b00011000; // LED1 & LED2 are output.
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
  
  while (true) {       // Always true, so loop forever.
    PORTB = 0; // OFF
    delay(20000U);
    for(unsigned int i=0;i<200; ++i){
      dim_on(LED1 + LED2,1,i); // PB3 and PB4 fade in
    }
    for(unsigned int i=200;i>=0; --i){
      dim_on(LED1 + LED2,1,i); // PB3 and PB4 fade out
    }
    
    PORTB = 0; // ALL OFF
  }
}

