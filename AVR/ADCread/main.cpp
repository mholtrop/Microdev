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


uint16_t ReadADC(uint8_t ch)
{
  // ADC channel: 0-7 = pins, 8 = temp, 14 = 1.1 V, 15 = GND
  ch=ch&0x0f;
  if( ch > 8 && ch < 14) ch = 15;
  ADMUX = (ADMUX & 0xf0) | ch;
  
  //Start Single conversion
  ADCSRA|=(1<<ADSC);
  
  //Wait for conversion to complete
  unsigned int convert_time = 0;
  while( !(ADCSRA & (1<<ADIF)) && convert_time < 0xfffe ) convert_time++;
  
  ADCSRA|=(1<<ADIF);
  
  Serial.print("ADC converson on ");
  Serial.print(ch);
  Serial.print(" finished in: ");
  Serial.print(convert_time);
  Serial.print(" value: ");
  Serial.println(ADC);
  
  return(ADC);
}

float ReadInternalTemp(void){
  // Setup the ADC to read the internal temperature of the chip.
  // This requires the 1.1V internal reference, so NOTHING CONNECTED TO VREF (exept a cap to GND).
  uint8_t ADMUX_store = ADMUX;
  ADMUX = (1<<REFS1) | (1<<REFS0) | 0x08; // Select 1.1 V ref and input 8 = Temp.
  ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); // Enable ADC, set prescale to 128.
  
  ADCSRA|=(1<<ADSC); // Start conversion.
  while( !(ADCSRA & (1<<ADIF)) ); // Wait.
  ADCSRA |= (1<<ADIF); // Reset ADIF bit by writing 1 to it.

  int temp1 = ADC;
  
  ADCSRA|=(1<<ADSC); // Start conversion.
  while( !(ADCSRA & (1<<ADIF)) ); // Wait.
  ADCSRA |= (1<<ADIF); // Reset ADIF bit by writing 1 to it.

  int temp2 = ADC;

  Serial.print("Temp: ");
  Serial.print(temp1);
  Serial.print(" ");
  Serial.print(temp2);
  Serial.print(" = ");
  
  float temp = (temp1 + temp2 - 2*269)*0.506240; // Calibration from reference is WAY OFF!
  
  Serial.println(temp);
  
  ADMUX = ADMUX_store;  // Restore the ADC settings.
  return(temp);
  
}

int main (void) {

  sei();  // Enable interrupts:  __asm__ __volatile__ ("sei" ::: "memory");
  Serial.begin(9600);
  Serial.print("\n\r --- Hello this is the atmega168 starting up. V-0.0.1x ---\n\r");

  // Setup the ADC for reading.
  ADMUX = (1<<REFS0); // 0x40; // Set bit 6, select AVcc as voltage ref. (1<< 6), no result shift, select ADC0
//  ADMUX = (1<<REFS0)|(1<<REFS1) ; // Internal 1.1V reference.
  
  ADCSRA = (1<<ADEN)|(0<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); // Enable ADC, set prescale to 128.
  
  Serial.print("Loop top..\n\r");
//  unsigned int loop_count=0;
  
  while(1){
//    float temp = ReadInternalTemp();
    for(char jj=0;jj<=5;++jj) ReadADC(jj);
    for(unsigned int ii =0;ii<10000; ++ii){
      for(unsigned int jj=0;jj<100  ; ++jj){
        _NOP();
      }
    }
    for(unsigned int ii =0;ii<10000; ++ii){
      for(unsigned int jj=0;jj<100  ; ++jj){
      _NOP();
      }
    }
  }
}

