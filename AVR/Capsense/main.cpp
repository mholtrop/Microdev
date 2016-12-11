/*
 *  Minimal.cpp
 *
 *  Minimal program for directly talking to an Atmega168
 *
 *  Created on: April 2, 2015
 *      Author: Maurik
 */

#include <avr/io.h>
#include "Arduino.h"
#include "HardwareSerial.h"

#define MAX_SENSE_TIME 1000

// send pin's ports and bitmask and receive pin's ports and bitmask
int SenseTouch(volatile uint8_t *sPort, uint8_t sBit,volatile uint8_t *rPort, uint8_t rBit )
{
  int total = 0;
  int low_tot=0;
  int hi_tot=0;
  
  *(sPort-1) |= (sBit);   // Set bit   = Set send pin to OUTPUT
  *(rPort-1) &= ~(rBit) ; // Clear bit = Set receive pin to INPUT
  *rPort &= ~rBit;        // pullup is off
  
  cli(); // Clear interrupt flag, disabling interrups.

  *sPort &= ~sBit;        // sendPin Low
  *(rPort-1) |= rBit;     // Output
  *rPort &= ~rBit;        // receive pin LOW = Discharge Cap
  *(rPort-1) &= ~rBit;    // receive pin back to Input
  *rPort &= ~rBit;        // pullup is off
  *sPort |= sBit;         // sendPin to High
  
  sei(); // Allow interrupts again.
  
 while ( ( (*(rPort-2) & rBit)==0 ) && (low_tot <= MAX_SENSE_TIME) ) {  // while receive pin is LOW AND total is positive value
    low_tot++;
  }
  
  if (low_tot >= MAX_SENSE_TIME) {
    return -1;
  }
  
  // set receive pin HIGH briefly to charge up fully - because the while loop above will exit when pin is ~ 2.5V
  cli();
  *(rPort-1) |= rBit; // receivePin to Output (high)
  *rPort |= rBit;     // set high
  *(rPort-1) &= ~rBit; // receivePin to Input
  *rPort &= ~rBit;        // pullup is off
  *sPort &= ~sBit;  // sendPin to LOW
  sei();
  
  while (  ( (*(rPort-2) & rBit)) && (hi_tot <= MAX_SENSE_TIME) ) {  // while receive pin is HIGH  AND total is less than timeout
    hi_tot++;
  }
//  Serial.print("Sense(2): ");
//  Serial.println(hi_tot);

  total = hi_tot + low_tot;
  
  if (hi_tot >= MAX_SENSE_TIME) {
    return -2;
  }

  return total;
}


int main (void) {

  sei();  // Enable interrupts:  __asm__ __volatile__ ("sei" ::: "memory");
  Serial.begin(9600);
  Serial.print("\n\r --- Hello this is the atmega168 starting up. V-0.0.4 ---\n\r");

//  DDRB = 0x02;  // Scope to PB1
//  PORTB = 0x02; // ON
//  DDRD = 0x80;  // 0b10000000; // PD7 is output.
//  PORTD = 0x80; // ON

//  CapSensor *sens1 = new CapSensor(&PORTC,4,&PORTC,5); // Send on PC4 Sense on PC5
//  CapSensor *sens2 = new CapSensor(&PORTC,4,&PORTC,3); // Send on PC4 Sense on PC3

  
  Serial.print("Loop top..\n\r");
  unsigned int loop_count=0;
  
  while (1) {
    loop_count++;
  
    int s1 = SenseTouch(&PORTC,_BV(4),&PORTC,_BV(5));
    int s2 = SenseTouch(&PORTC,_BV(4),&PORTC,_BV(5));
    int s3 = SenseTouch(&PORTC,_BV(4),&PORTC,_BV(3));
    int s4 = SenseTouch(&PORTC,_BV(4),&PORTC,_BV(3));
    Serial.print(loop_count);
    Serial.print(" - Result:");
    Serial.print("\t");
    Serial.print(s1);
    Serial.print("\t");
    Serial.print(s2);
    Serial.print("\t");
    Serial.print(s3);
    Serial.print("\t");
    Serial.println(s4);
    for(unsigned int ii =0;ii<5000; ++ii){
      for(unsigned int jj=0;jj<20  ; ++jj){
      _NOP();
      }
    }
  }
}

