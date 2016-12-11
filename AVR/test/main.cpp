/*
 * main.cpp
 *
 *  Created on: Mar 30, 2015
 *      Author: maurik
 */

#include "Arduino.h"

int main(void){

  init();

  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);   // turn the LED on (HIGH is the voltage level)
  
  while(1){
    digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(100);              // wait for a second
    digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
    delay(100);              // wait for a second
    digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(10);              // wait for a second
    digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
    delay(90);              // wait for a second
    digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(10);              // wait for a second
    digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
    delay(90);              // w
    digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(10);              // wait for a second
    digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
    delay(20);              // w
    
  }
}

