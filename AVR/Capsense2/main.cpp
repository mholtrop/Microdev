/*
 *  Minimal.cpp
 *
 *  Minimal program for directly talking to an Atmega168
 *
 *  Created on: April 2, 2015
 *      Author: Maurik
 */

#include <avr/io.h>
#include <util/delay.h>

#include "Arduino.h"
#include "HardwareSerial.h"

#define ADC_SETTLE_TIME     10000
#define CHARGE_WAIT_TIME    1000
#define DISCHARGE_WAIT_TIME 1000

int SenseTouch2(uint8_t ch){
  // Read a capacitive sense.
  // This is on the ADC which is also PORTC on the ATMega168/328 chips
  
  ch=ch&0x0f; // Only allow channels 0-7

  // Discharge the ADC S/H capacitor by measuring GND
  // I found it takes 2 measurements to consistently read 0.

  ADMUX =(1<<REFS0);  // Select AVcc as voltage ref. (1<< 6), no result shift, select ADC0
  ADMUX = (ADMUX & 0xf0) | 15;   // Channel 15 = Ground.

  _delay_us(ADC_SETTLE_TIME);

  ADCSRA |= (1<<ADSC);           // Start conversion, thus discharging cap.
  while( !(ADCSRA & (1<<ADIF))); // Wait.
  ADCSRA|=(1<<ADIF);             // Reset conversion flag.
  ADMUX = (ADMUX & 0xf0) | 15;   // Channel 15 = Ground.
  ADCSRA |= (1<<ADSC);           // Start conversion, thus discharging cap.
  while( !(ADCSRA & (1<<ADIF))); // Wait.
  ADCSRA|=(1<<ADIF);             // Reset conversion flag.
  
  uint16_t ref_adc0 = ADC;
  
// Charge the outside capacitance though the pullup resistor.
// Note that a "direct charge", by setting the pin to output and 1 does not
// seem to work. It did not result in any sensitivity.
//
  DDRC  &= ~(1<<ch);   // Clear the bit for ch on DDRC - set port to INPUT
  PORTC |=  (1<<ch);   // Pull up resistor is ON - so we charge the external capacitor.
  _delay_us(CHARGE_WAIT_TIME); // Wait for a little, to allow cap to charge.
  PORTC &= ~(1<<ch);   // Pull resistor OFF.

// Connect the charged up pin to the S/H capacitor of the ADC and read the resulting value.
  
  ADMUX = (ADMUX & 0xf0) | ch;   // Channel select.
  ADCSRA |= (1<<ADSC);           // Start conversion. This causes charge share between caps.
  while( !(ADCSRA & (1<<ADIF))); // Wait.
  ADCSRA|=(1<<ADIF);             // Reset conversion flag.
  uint16_t touch_up = ADC;       // Store the ADC value.

#ifdef Serial_Debug_Sense_ON
  Serial.println("-----------------------");
  Serial.print("Touch ref  = ");
  Serial.println(ref_adc0);
  Serial.print("Touch up   = ");
  Serial.println(touch_up);
#endif

  // We now charge the S/H capacitor and then charge share with the external capacitance.
// Again, do two charge up measurements, which seems to give a more stable value.
//
  ADMUX = (1<<REFS0)|(1<<REFS1) ; // Internal 1.1V reference.
  ADMUX = (ADMUX & 0xf0) | 14;    // Channel select to internal +1.1V reference.

  _delay_us(ADC_SETTLE_TIME);

  ADCSRA |= (1<<ADSC);           // Start conversion, thus charging the S/H cap.
  while( !(ADCSRA & (1<<ADIF))); // Wait.
  ADCSRA|=(1<<ADIF);             // Reset conversion flag.
  ADMUX = (ADMUX & 0xf0) | 14;   // Channel select to internal 1.1 V reference.
  ADCSRA |= (1<<ADSC);           // Start conversion, thus charging the S/H cap.
  while( !(ADCSRA & (1<<ADIF))); // Wait.
  ADCSRA|=(1<<ADIF);             // Reset conversion flag.

  uint16_t ref_adc1 = ADC;        // Store the ADC value for reference
  
  // Discharge the outside capacitance fully by connecting pin to ground.
  DDRC  |= (1<<ch);              // Set pin to OUTPUT
  PORTC &= ~(1<<ch);             // Set pin to 0
  _delay_us(DISCHARGE_WAIT_TIME);// Wait. Probably can be really short.
  DDRC  &= ~(1<<ch);   // Clear the bit for ch on DDRC - set port to INPUT

  // Connect the discharged pin to the charged S/H capacitor of the ADC and read the resulting value.

  ADMUX = (ADMUX & 0xf0) | ch;   // Channel select.
  ADCSRA |= (1<<ADSC);           // Start conversion. This causes charge share between caps.
  while( !(ADCSRA & (1<<ADIF))); // Wait.
  ADCSRA|=(1<<ADIF);             // Reset conversion flag.

  uint16_t touch_down = ADC;       // Store the ADC value.
  
#ifdef Serial_Debug_Sense_ON
  Serial.print("Touch ref  = ");
  Serial.println(ref_adc1);
  Serial.print("Touch down = ");
  Serial.println(touch_down);
#endif
  
  ADMUX =(1<<REFS0);  // Select AVcc as voltage ref. (1<< 6), no result shift, select ADC0
  
  return(touch_up - ref_adc0 + ref_adc1 - touch_down);
}


int main (void) {

  sei();  // Enable interrupts:  __asm__ __volatile__ ("sei" ::: "memory");
  Serial.begin(9600);

  // Init the ADC circuitry.
  // Setup the ADC for reading.

  // Prescale number will be calculated and optimized by compiler.
#define ADC_PRESCALE ((int)(ceil((log((F_CPU/125000))/log(2)))))
  
  ADCSRA = (1<<ADEN)| ADC_PRESCALE; // Enable ADC, set prescale to 2^3 = 8.
  ADMUX =(1<<REFS0);  // Select AVcc as voltage ref. (1<< 6), no result shift, select ADC0
  ADMUX = (1<<REFS0)|(1<<REFS1) ; // Internal 1.1V reference.
  
  MCUCR &= ~(1<< PUD); // Set the PUD (pull up disable) bit on MCUCR to zero, so we allow pull-ups.

  Serial.print("\n\r --- Hello this is Touch2 V-0.9 ---\n\r");
  Serial.println("--- Calibrating --- ");
  Serial.print("F_CPU ");
  Serial.println(F_CPU);
  Serial.print("ADC prescale: ");
  Serial.println(ADC_PRESCALE);
  
#define N_CALIB_SAMPLE 20

  SenseTouch2(5);
  SenseTouch2(3);

  unsigned int s=0;
  for(int ii=0;ii<N_CALIB_SAMPLE;++ii){
    unsigned int m = SenseTouch2(5);
    s += m;
  }
  int base_line5 = s/N_CALIB_SAMPLE - 10;
  
  Serial.print("base_line5: ");
  Serial.println(base_line5);

  s=0;
  for(int ii=0;ii<N_CALIB_SAMPLE;++ii){
    unsigned int m = SenseTouch2(3);
    s += m;
  }
  int base_line3 = s/N_CALIB_SAMPLE - 10;
  
  Serial.print("base_line3: ");
  Serial.println(base_line3);

  Serial.println("------------------- ");
  

  Serial.print("Loop top..\n\r");
  unsigned int loop_count=0;
  
  while (1) {
    loop_count++;
  
    int s1 = SenseTouch2(5) - base_line5;
    int s2 = SenseTouch2(5) - base_line5;
    int s3 = SenseTouch2(3) - base_line3;
    int s4 = SenseTouch2(3) - base_line3;
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
    _delay_ms(500);
  }
}

