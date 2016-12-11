/*
 * LEDBall.cpp
 *
 *  Created on: Nov 11, 2016
 *      Author: Maurik Holtrop
 */

#include "LEDBall.hpp"

LEDBall::LEDBall(): Adafruit_NeoPixel() {
	

}

LEDBall::LEDBall(uint16_t n,uint8_t p,neoPixelType t): Adafruit_NeoPixel(n,p,t){
  
}

LEDBall::~LEDBall() {
	
}

void LEDBall::setBaseColor(uint8_t r,uint8_t g,uint8_t b){ // Set all pixels to r,g,b color.
  //  uint8_t *pixels = getPixels();
  
  //  uint24_t color = r << 16 | g << 8 | b;
  for(uint16_t i=0; i<MAX_Z; ++i){
    uint16_t n = 3*i;
    pixels[n+rOffset] = r;
    pixels[n+gOffset] = g;
    pixels[n+bOffset] = b;
  }
};

void LEDBall::deltaBaseColor(int16_t r,int16_t g,int16_t b){ // Change all pixels by r,g,b in color space.
  //  uint32_t *pixels = getPixels();

  //  uint24_t color = r << 16 | g << 8 | b;
  for(uint16_t i=0; i<MAX_Z; ++i){
    uint16_t n = 3*i;
    pixels[n+rOffset] += r;
    pixels[n+gOffset] += g;
    pixels[n+bOffset] += b;
  }
};

void LEDBall::deltaPixelColor(uint16_t i,int16_t r,int16_t g,int16_t b){ // Change all pixels by r,g,b in color space.
  //  uint32_t *pixels = getPixels();
  uint16_t n=3*i;
  pixels[n+rOffset] = r;
  pixels[n+gOffset] = g;
  pixels[n+bOffset] = b;
 };
