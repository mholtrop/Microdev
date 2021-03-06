/*
 * LEDBall.h
 *
 *  Created on: Nov 11, 2016
 *      Author: Maurik Holtrop
 */

#ifndef LEDBALL_H_
#define LEDBALL_H_

//#define HardwareSerial_h
#include "NeoPixel.h"

// Configuration of the LED Ball.
#ifndef MAX_X
#define MAX_X  8
#endif
#ifndef MAX_Y
#define MAX_Y  32
#endif

#define MAX_Z (MAX_X*MAX_Y)  // ODD strings are X/2 smaller!

#ifndef STANDARD
#define STANDARD 0
#endif

#define EXTRA_STORE

// For the NEO_RGB pixels, the only kind I have, the value of NEO_RGN+NEO_KHZ800 = 6
// The offsets are then:
// roffset = 0
// goffset = 1
// boffset = 2
// So packing is in b,g,r order for each 24bit rgb pixel.
// Instead of using offsets, we could use:
// color = ( b << 16 | g << 8 | r )
// This is different from the My_NeoPixel::Color(r,g,b) which packs (r<<16 | g<<8 | b)
// and is thus a bit idiotic.
// I don't know what is faster.
class LEDBall : public NeoPixel {

public:
	LEDBall(void);
  LEDBall(uint8_t p=4,volatile uint8_t *pt=&PORTD,neoPixelType t=NEO_RGB + NEO_KHZ800);
  ~LEDBall();

  uint16_t getIndex(uint8_t x, uint8_t y){  // Compute the pixel index from the x,y location.
    if( x>= MAX_X) x=MAX_X-1;      // Check to not go over limit.
    if( y>= MAX_Y) y=MAX_Y-1;
#if STANDARD == 0
  #warning Standard 0 code.
    return( x*MAX_Y + y);
#elif STANDARD == 1   // ODD type, where every odd (1,3,5) strip has a pixel less. UGH
  #warning Standard 1 code.
    if( x%2 == 1 && y == (MAX_Y-1)) y = MAX_Y -2;
    return( x*MAX_Y + y - ( x/2 ) );
#elif STANDARD == 2  // Reversing type
  #warning Standard 2 code.
    if( x < MAX_X/2) return( (2*x*MAX_Y) + y);  // Y is normal, x is two times further
    return( 2*(x+1-MAX_X/2)*MAX_Y - y - 1);
#else 
#error STANDARD has a value that I do not know about.
#endif
  };
  
  void setBaseColor(uint8_t r,uint8_t g,uint8_t b,uint8_t *pix=NULL);
  void multBaseColor(float r,float g,float b,uint8_t *pix=NULL); // Change all pixels by r,g,b in color space.
  void deltaBaseColor(int16_t r,int16_t g,int16_t b,uint8_t *pix=NULL); // Change all pixels by r,g,b in color space.
  void multPixelColor(uint16_t i,float r,float g,float b,uint8_t *pix=NULL); // Change all pixels by r,g,b in color space.
   void deltaPixelColor(uint16_t i,int16_t r,int16_t g,int16_t b,uint8_t *pix=NULL); // Change all pixels by r,g,b in color space.
  void setPixelColor(uint16_t loc,uint8_t r,uint8_t g,uint8_t b,uint8_t *pix=NULL);
  uint32_t getPixelColorXY(uint16_t x,uint16_t y,uint8_t *pix=NULL);
  void setPixelColorXY(uint16_t x,uint16_t y,uint32_t c,uint8_t *pix=NULL);
  void setPixelColorXY(uint16_t x,uint16_t y,uint8_t r,uint8_t g,uint8_t b,uint8_t *pix=NULL);
  void Temp_to_RGB(uint16_t T, uint8_t *r, uint8_t *g, uint8_t *b);
  
#ifdef EXTRA_STORE
  int16_t alloc_store(void);
  
  void copy_to_store(){
    if(pixels_st) memcpy(pixels_st,pixels,MAX_Z*3);
  }
  
  void copy_store(uint8_t *pix1,uint8_t *pix2){
    // Copy the pixels from pix2 into pix1 using memcpy.
    if(pix1 && pix2)memcpy(pix1,pix2,MAX_Z*3);
  }

  void copy_from_store(){
    if(pixels_st)memcpy(pixels,pixels_st,MAX_Z*3);
  }
  
  void swap_store();
  
  void linear_chase(uint8_t *pix=NULL);
  void rotate_z(uint8_t n,uint8_t *pix=NULL);
  
public:
  
  uint8_t *pixels_st;
#endif
  
};

#endif /* LEDBALL_H_ */
