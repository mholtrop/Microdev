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
#include "MySerial.hpp"          // This must go before Arduino.h, otherwise it will include the HardwareSerial.h
#define HardwareSerial_h
#include "Arduino.h"
#include "Adafruit_NeoPixel.h"
#include "LEDBall.hpp"

#define L_PIN1 6
#define N_PIN1 256

MySerial Serial(&UBRR0H, &UBRR0L, &UCSR0A, &UCSR0B, &UCSR0C, &UDR0);    // To be able to hook the functions into these vectors, and to be able to put "print" anywhere in the code, this must be global.

ISR(USART_RX_vect){
  Serial._rx_complete_irq();
}

ISR(USART_UDRE_vect){
  Serial._tx_udr_empty_irq();
}

//extern "C" void __vector_18 (void) __attribute__ ((signal,used, externally_visible)) ; void __vector_18 (void){
//  Serial._rx_complete_irq();
//}
//
//extern "C" void USART_UDRE_vect (void) __attribute__ ((signal,used, externally_visible)) ; void USART_UDRE_vect (void){
//  Serial._tx_udr_empty_irq();
//}


int main(void) {

  init();               // SETUP THE ARDUINO environment.
  
  Serial.begin(115200);
  Serial.print("\n\rHello. This is LEDBall Code V0.2.\n\r");
  Serial.flush();
  Serial.print("\n\rI kind of really hope the serial stuff works.\n\r");
  
  pinMode(L_PIN1, OUTPUT);  // This sets the output pin to OUTPUT. It also makes sure the arduinolib is loaded properly.
  
  LEDBall leds;
  
  while (1) {
    leds.copy_to_store();
    leds.copy_from_store();
    if (Serial.available()) {
      int s_in = Serial.read();
      Serial.print(s_in);
    }
	}
  
	return 0; // never reached
}
