/*
 CapacitiveSense.h v.04 - Capacitive Sensing Library for 'duino / Wiring
 https://github.com/PaulStoffregen/CapacitiveSensor
 http://www.pjrc.com/teensy/td_libs_CapacitiveSensor.html
 http://playground.arduino.cc/Main/CapacitiveSensor
 Copyright (c) 2009 Paul Bagder  All right reserved.
 Version 05 by Paul Stoffregen - Support non-AVR board: Teensy 3.x, Arduino Due
 Version 04 by Paul Stoffregen - Arduino 1.0 compatibility, issue 146 fix
 vim: set ts=4:
 */
#include "Arduino.h"
#include "CapSensor.h"
#include "HardwareSerial.h"

// Constructor /////////////////////////////////////////////////////////////////
// Function that handles the creation and setup of instances

CapSensor::CapSensor(volatile uint8_t *sendPort,uint8_t sendPin, volatile uint8_t *receivePort,uint8_t receivePin){
  // initialize this instance's variables
  // Serial.begin(9600);		// for debugging

  sPort= sendPort;
  sBit = 1<<sendPin;
  rPort= receivePort;
  rBit = 1<<receivePin;


  error = 1;
  loopTimingFactor = 310;		// determined empirically -  a hack

  CS_Timeout_Millis = (2000 * (float)loopTimingFactor * (float)F_CPU) / 16000000;
  CS_AutocaL_Millis = 20000;
  total = 0;

  Serial.print("timwOut =  ");
  Serial.println(CS_Timeout_Millis);

  // get pin mapping and port for send Pin - from PinMode function in core

  *(sPort-1) |= (sBit);         // Set bit   = Set send pin to OUTPUT
  *(rPort-1) &= ~(rBit) ; // Clear bit = Set receive pin to INPUT

  *sPort &= ~(sBit);  // Clearbit = Set Sendpin to LOW = 0

  leastTotal = 0x0FFFFFFFL;   // input large value for autocalibrate begin
  lastCal = millis();         // set millis for start
}

// Public Methods //////////////////////////////////////////////////////////////
// Functions available in Wiring sketches, this library, and other libraries

long CapSensor::sens(uint8_t samples)
{
  total = 0;
  if (samples == 0) return 0;
  if (error < 0) return -1;            // bad pin


  for (uint8_t i = 0; i < samples; i++) {    // loop for samples parameter - simple lowpass filter
    if (senseOne() < 0)  return -2;   // variable over timeout
  }

  // only calibrate if time is greater than CS_AutocaL_Millis and total is less than 10% of baseline
  // this is an attempt to keep from calibrating when the sensor is seeing a "touched" signal

  if ( (millis() - lastCal > CS_AutocaL_Millis) && abs(total  - leastTotal) < (int)(.10 * (float)leastTotal) ) {

    Serial.println();               // debugging
    Serial.println("auto-calibrate");
    Serial.println();
    delay(2000);

    leastTotal = 0x0FFFFFFFL;          // reset for "autocalibrate"
    lastCal = millis();
  }
  else{                                // debugging
    Serial.print("  total =  ");
    Serial.print(total);

    Serial.print("   leastTotal  =  ");
    Serial.println(leastTotal);

    Serial.print("total - leastTotal =  ");
    unsigned long x = total - leastTotal ;
    Serial.print(x);
    Serial.print("     .1 * leastTotal = ");
    x = (int)(.1 * (float)leastTotal);
    Serial.println(x);
  }

  // routine to subtract baseline (non-sensed capacitance) from sensor return
  if (total < leastTotal) leastTotal = total;                 // set floor value to subtract from sensed value
  return(total - leastTotal);

}

long CapSensor::sensRaw(uint8_t samples)
{
  total = 0;
  if (samples == 0) return 0;
  if (error < 0) return -1;                  // bad pin - this appears not to work

  for (uint8_t i = 0; i < samples; i++) {    // loop for samples parameter - simple lowpass filter
    if (senseOne() < 0)  return -2;   // variable over timeout
  }

  return total;
}


void CapSensor::reset_CS_AutoCal(void){
  leastTotal = 0x0FFFFFFFL;
}

void CapSensor::set_CS_AutocaL_Millis(unsigned long autoCal_millis){
  CS_AutocaL_Millis = autoCal_millis;
}

void CapSensor::set_CS_Timeout_Millis(unsigned long timeout_millis){
  CS_Timeout_Millis = (timeout_millis * (float)loopTimingFactor * (float)F_CPU) / 16000000;  // floats to deal with large numbers
}

// Private Methods /////////////////////////////////////////////////////////////
// Functions only available to other functions in this library

int CapSensor::senseOne(void)
{
  noInterrupts();

  *sPort &= ~sBit;        // sendPin Low
  *(rPort-1) |= rBit;     // Output
  *rPort &= ~rBit;        // receive pin LOW = Discharge Cap
//  delayMicroseconds(2);
  *(rPort-1) &= ~rBit;    // receive pin back to Input
  *sPort |= sBit;         // sendPin to High
  interrupts();

  while ( ( (*(rPort-2) & rBit)==0 ) && (total <= CS_Timeout_Millis) ) {  // while receive pin is LOW AND total is positive value
    total++;
  }
//  Serial.print("sensOne(1): ");
//  Serial.println(total);

  if (total >= CS_Timeout_Millis) {
//    Serial.println("sensOne(1) overflow");
    return -2;         //  total variable over timeout
  }

  // set receive pin HIGH briefly to charge up fully - because the while loop above will exit when pin is ~ 2.5V
  noInterrupts();
  *(rPort-1) |= rBit; // receivePin to Output (high)
  *rPort |= rBit;     // set high
  *(rPort-1) &= ~rBit; // receivePin to Input
  *rPort &= ~rBit;   // pullup is off
  *sPort &= ~sBit;  // sendPin to LOW
  interrupts();

  while (  ( (*(rPort-2) & rBit)? 1:0 ) && (total < CS_Timeout_Millis) ) {  // while receive pin is HIGH  AND total is less than timeout
    total++;
  }
//  Serial.print("SenseOne(2): ");
//  Serial.println(total);

  if (total >= CS_Timeout_Millis) {
//    Serial.print("sensOne(2) overflow");
    return -2;     // total variable over timeout
  } else {
    return 1;
  }
}
