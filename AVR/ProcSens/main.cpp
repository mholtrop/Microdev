/*
 *  Minimal.cpp
 *
 *  Minimal program for directly talking to an Atmega168
 *
 *  Created on: April 2, 2015
 *      Author: Maurik
 */

//#undef __AVR_ATmega16__
//#define __AVR_ATmega168__ 1
#include <avr/io.h>
#include "Arduino.h"
#include "HardwareSerial.h"
#include "CapSensor.h"

double GetTemp(void);

int main (void) {

  sei();  // Enable interrupts:  __asm__ __volatile__ ("sei" ::: "memory");
  Serial.begin(9600);
  Serial.println(" ");
  Serial.println("Hello this is the atmega168 starting up. 1MHz @ 9600 baud V-0.0.1a");
  double temp = 0; //GetTemp();
  Serial.print("Temperature is now: ");
  Serial.println(temp);

  MCUCR = (1<<PUD);  // Pull up Disable in input ports, regardless of PORTx setting.

//  volatile uint8_t *Sensor      = &PORTB;  // Sensor port
//  volatile uint8_t *Sensor_ctr  = &DDRB;   // Control register for sensor port
//  unsigned char Sensor_pulse = (1 << 1);    // Pulse on PB1 (pin 15), so OUTPUT
//  unsigned char Sensor_in    = (1 << 2);    // Sense on PB2 (pin 16)
//
//  *Sensor_ctr = Sensor_pulse & Sensor_in; // Both are output
//  *Sensor     = 0x00;                  // Bost are low.

  CapSensor *sens = new CapSensor(&PORTB,1,&PORTB,2);  // Send on PB1 Sense on PB2

  volatile uint8_t *LED = &PORTD;
  unsigned char LEDbit = (1<<7);    // PD7 is LED
  DDRD  = LEDbit; // 0b10000000; // PD7 is output = LED
  *LED = LEDbit; // ON

  Serial.print("Loop top..\n");
  int loopcount=0;
  while (1) {
      if( (loopcount++)%100 == 0){
        Serial.print("L=");
        Serial.println(loopcount);
      }

      long s1=sens->sensRaw(3);
      long s2=sens->sensRaw(20);
      Serial.print("Result:");
      Serial.print("\t");
      Serial.print(s1);
      Serial.print("\t");
      Serial.println(s2);


      for (unsigned char i = 0; i < 200; i++) {
        for (unsigned char i = 0; i < 255; i++) {
        asm volatile(
            "nop  \n\t"
            "nop  \n\t"
            "nop  \n\t"
            "nop  \n\t"
            "nop  \n\t"
            "nop  \n\t"
        );
      }
    }
    PORTD ^= 0x80;  // XOR = toggle bit 7.
  }
}

double GetTemp(void)
{
  unsigned int wADC;
  double t;

  // The internal temperature has to be used
  // with the internal reference of 1.1V.
  // Channel 8 can not be selected with
  // the analogRead function yet.

  // Set the internal reference and mux.
  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
  ADCSRA |= _BV(ADEN);  // enable the ADC

  delay(20);            // wait for voltages to become stable.

  ADCSRA |= _BV(ADSC);  // Start the ADC

  // Detect end-of-conversion
  unsigned int max_count=0;
  const unsigned int MAX_LOOP_COUNT =  255; //(1U<<15);
  while (bit_is_set(ADCSRA,ADSC) && max_count++< MAX_LOOP_COUNT){ asm volatile( "nop"); }

  // Reading register "ADCW" takes care of how to read ADCL and ADCH.
  wADC = ADCW;

  // The offset of 324.31 could be wrong. It is just an indication.
  t = (wADC - 324.31 ) / 1.22;

  // The returned temperature is in degrees Celcius.
  return (t);
}
