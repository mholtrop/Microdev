/*
 * main.cpp
 *
 *  Created on: Mar 30, 2015
 *      Author: maurik
 */
#include "Arduino.h"
#include "Adafruit_NeoPixel.h"
#include "HardwareSerial.h"

#define L_PIN1 6
#define N_PIN1 4

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)

Adafruit_NeoPixel *strip1 = new Adafruit_NeoPixel(N_PIN1, L_PIN1, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

//void rainbow(Adafruit_NeoPixel *strip,uint8_t wait);
//void rainbowCycle(Adafruit_NeoPixel *strip,uint8_t wait);
//void theaterChase(Adafruit_NeoPixel *strip,uint32_t c, uint8_t wait);
//void theaterChaseRainbow(Adafruit_NeoPixel *strip,uint8_t wait);
//uint32_t Wheel(Adafruit_NeoPixel *strip,byte WheelPos);
//void colorWipe(Adafruit_NeoPixel *strip,uint32_t c, uint8_t wait);

int main(void){
  init();               // SETUP THE ARDUINO!

  Serial.begin(115200);
  Serial.print("NeoPixel V0.2 \n\r");
  pinMode(L_PIN1, OUTPUT);  // This sets the output pin to OUTPUT. It also makes sure the arduinolib is loaded properly.

  strip1->begin();
  strip1->clear();
  strip1->show();

  for(int i=0;i<N_PIN1;i++){
    strip1->setPixelColor(i, strip1->Color(255,255,255));
  }
  strip1->show();
  delay(2000);

  Serial.print("R");
  for(unsigned int i=0;i<N_PIN1;i++){
    strip1->setPixelColor(i, strip1->Color(255,0,0));
  }
  strip1->show();
  delay(2000);
  Serial.print("G");
  for(unsigned int i=0;i<N_PIN1;i++){
    strip1->setPixelColor(i, strip1->Color(0,255,0));
  }
  strip1->show();
  delay(2000);
  Serial.println("B");
  for(unsigned int i=0;i<N_PIN1;i++){
    strip1->setPixelColor(i, strip1->Color(0,0,255));
  }
  strip1->show();
  delay(2000);

//  Serial.print("RAINB. \n\r");
//  rainbow(strip1,2);
//
  Serial.print("\n\r");
  char command_buf[8];
  int cmd_i=0;
  int loc;
  int red;
  int green;
  int blue;

  while(1){
    if (Serial.available()) {
      int s_in = Serial.read();
      Serial.println(s_in);
      if( s_in != ' ' && s_in != '\r' && s_in != '\n'){
	command_buf[cmd_i]=s_in;
	if( ++cmd_i >= 8 ){
	  Serial.println("xxx:");
	  Serial.println(command_buf);
	  command_buf[0]=0;
	  cmd_i=0;
	}
      }else{
	switch( command_buf[0] ){
	case 'O': // Off
	  strip1->clear();
	  strip1->show();
	  Serial.println("O");
	  break;
	case 'F': // Full
	  for(int i=0;i<N_PIN1;i++){
	    strip1->setPixelColor(i, 255,255,255);
	  }
	  strip1->show();
	  Serial.println("F");
	  break;
	case 'W': // Warm White
	  for(int i=0;i<N_PIN1;i++){
	    strip1->setPixelColor(i, 255,110,30);
	  }
	  strip1->show();
	  Serial.println("W");
	  break;	
	case 'R': // All Red
	  for(int i=0;i<N_PIN1;i++){
	    strip1->setPixelColor(i, 255,0,0);
	  }
	  strip1->show();
	  Serial.println("R");
	  break;	
	case 'G': // All Green
	  for(int i=0;i<N_PIN1;i++){
	    strip1->setPixelColor(i, 0,255,0);
	  }
	  strip1->show();
	  Serial.println("G");
	  break;	
	case 'B': // All Blue
	  for(int i=0;i<N_PIN1;i++){
	    strip1->setPixelColor(i, 0,0,255);
	  }
	  strip1->show();
	  Serial.println("B");
	  break;		
//	case 'r': // expect rgb codes
//	  red = Serial.parseInt();
//	  green = Serial.parseInt();
//	  blue = Serial.parseInt();
//	  for(int i=0;i<N_PIN1;i++){
//	    strip1->setPixelColor(i, (red<256?red:0),(green<256?green:0),(blue<256?blue:0));
//	  }
//	  strip1->show();
//	  Serial.print("r: ");
//	  Serial.print(red);
//	  Serial.print(green);
//	  Serial.println(blue);
//	  break;		
	case 'x': // expect location and  rgb codes
	  loc  = Serial.parseInt();
	  red  = Serial.parseInt();
	  green= Serial.parseInt();
	  blue = Serial.parseInt();
	  strip1->setPixelColor((loc<256?loc:0), (red<256?red:0),(green<256?green:0),(blue<256?blue:0));
	  strip1->show();
	  Serial.print("x: ");
	  Serial.print(red);
	  Serial.print(green);
	  Serial.println(blue);
	  break;
	}
	command_buf[0]=0;
	cmd_i=0;
      }
    }
  }
}

//
//// Some example procedures showing how to display to the pixels:
////  colorWipe(strip.Color(255, 0, 0), 50); // Red
////  colorWipe(strip.Color(0, 255, 0), 50); // Green
////  colorWipe(strip.Color(0, 0, 255), 50); // Blue
////  // Send a theater pixel chase in...
////  theaterChase(strip.Color(127, 127, 127), 50); // White
////  theaterChase(strip.Color(127,   0,   0), 50); // Red
////  theaterChase(strip.Color(  0,   0, 127), 50); // Blue
////
////  rainbow(20);
////  rainbowCycle(20);
////  theaterChaseRainbow(50);
//
//// Fill the dots one after the other with a color
//void colorWipe(Adafruit_NeoPixel *strip,uint32_t c, uint8_t wait) {
//  for(uint16_t i=0; i<strip->numPixels(); i++) {
//    strip->setPixelColor(i, c);
//    strip->show();
//    delay(wait);
//  }
//}
//
//void rainbow(Adafruit_NeoPixel *strip,uint8_t wait) {
//  uint16_t i, j;
//  
//  for(j=0; j<256; j++) {
//    for(i=0; i<strip->numPixels(); i++) {
//      strip->setPixelColor(i, Wheel(strip,(i+j) & 255));
//    }
//    strip->show();
//    delay(wait);
//  }
//}
//
//// Slightly different, this makes the rainbow equally distributed throughout
//void rainbowCycle(Adafruit_NeoPixel *strip,uint8_t wait) {
//  uint16_t i, j;
//  
//  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
//    for(i=0; i< strip->numPixels(); i++) {
//      strip->setPixelColor(i, Wheel(strip,((i * 256 / strip->numPixels()) + j) & 255));
//    }
//    strip->show();
//    delay(wait);
//  }
//}
//
////Theatre-style crawling lights.
//// void theaterChase(uint32_t c, uint8_t wait) {
////   for (int j=0; j<10; j++) {  //do 10 cycles of chasing
////     for (int q=0; q < 3; q++) {
////       for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
////         strip.setPixelColor(i+q, c);    //turn every third pixel on
////       }
////       strip.show();
//
////       delay(wait);
//
////       for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
////         strip.setPixelColor(i+q, 0);        //turn every third pixel off
////       }
////     }
////   }
//// }
//
//// //Theatre-style crawling lights with rainbow effect
//void theaterChaseRainbow(Adafruit_NeoPixel *strip,uint8_t wait) {
//  for (unsigned int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
//    for (unsigned int q=0; q < 3; q++) {
//      for (uint16_t i=0; i < strip->numPixels(); i=i+3) {
//	strip->setPixelColor(i+q, Wheel(strip, (i+j) % 255));    //turn every third pixel on
//      }
//      strip->show();
//      
//      delay(wait);
//      
//      for (uint16_t i=0; i < strip->numPixels(); i=i+3) {
//	strip->setPixelColor(i+q, 0);        //turn every third pixel off
//      }
//    }
//  }
//}
//
//// Input a value 0 to 255 to get a color value.
//// The colours are a transition r - g - b - back to r.
//uint32_t Wheel(Adafruit_NeoPixel *strip,byte WheelPos) {
//  WheelPos = 255 - WheelPos;
//  if(WheelPos < 85) {
//    return strip->Color(255 - WheelPos * 3, 0, WheelPos * 3);
//  } else if(WheelPos < 170) {
//    WheelPos -= 85;
//    return strip->Color(0, WheelPos * 3, 255 - WheelPos * 3);
//  } else {
//    WheelPos -= 170;
//    return strip->Color(WheelPos * 3, 255 - WheelPos * 3, 0);
//  }
//}
//



