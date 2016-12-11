/*
 * LEDBall.h
 *
 *  Created on: Nov 11, 2016
 *      Author: Maurik Holtrop
 */

#ifndef LEDBALL_H_
#define LEDBALL_H_

#include <Adafruit_Neopixel.h>

// Configuration of the LED Ball.
#define MAX_X  8
#define MAX_Y 32
#define MAX_Z (MAX_X*MAX_Y)  // ODD strings are X/2 smaller!
#define STANDARD

// For the NEO_RGB pixels, the only kind I have, the value of NEO_RGN+NEO_KHZ800 = 6
// The offsets are then:
// roffset = 0
// goffset = 1
// boffset = 2
// So packing is in b,g,r order for each 24bit rgb pixel.
// Instead of using offsets, we could use:
// color = ( b << 16 | g << 8 | r )
// This is different from the Adafruit_NeoPixel::Color(r,g,b) which packs (r<<16 | g<<8 | b)
// and is thus a bit idiotic.
// I don't know what is faster.
class LEDBall : Adafruit_NeoPixel {

public:
	LEDBall(void);
  LEDBall(uint16_t n,uint8_t p=6,neoPixelType t=NEO_RGB + NEO_KHZ800);
  ~LEDBall();

  uint16_t getIndex(uint8_t x, uint8_t y){  // Compute the pixel index from the x,y location.
    if( x>= MAX_X) x=MAX_X-1;      // Check to not go over limit.
    if( y>= MAX_Y) y=MAX_Y-1;
#ifdef STANDARD
    return( x*MAX_Y + y);
#else   // ODD type, where every odd (1,3,5) strip has a pixel less. UGH
    if( x%2 == 1 && y = MAX_Y-1) y = MAX_Y -2
    return( x*MAX_Y + y - ( x/2 ) )
#endif
  };
  
  void setBaseColor(uint8_t r,uint8_t g,uint8_t b);
  void deltaBaseColor(int16_t r,int16_t g,int16_t b); // Change all pixels by r,g,b in color space.
  void deltaPixelColor(uint16_t i,int16_t r,int16_t g,int16_t b); // Change all pixels by r,g,b in color space.
  void setPixelColorXY(int x,int y,uint8_t r,uint8_t g,uint8_t b){
    uint16_t i=getIndex(x,y);
    setPixelColor(i,r,g,b);
  }
  
};

#endif /* LEDBALL_H_ */
