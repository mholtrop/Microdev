/*
 * LEDBall.cpp
 *
 *  Created on: Nov 11, 2016
 *      Author: Maurik Holtrop
 */

#include "LEDBall.hpp"

LEDBall::LEDBall(): Adafruit_NeoPixel() {
	

}

LEDBall::LEDBall(uint8_t p,volatile uint8_t *pt,neoPixelType t): Adafruit_NeoPixel(MAX_Z,p,pt,t){
  
}

LEDBall::~LEDBall() {
	
}

void LEDBall::setBaseColor(uint8_t r,uint8_t g,uint8_t b, uint8_t *pix){ // Set all pixels to r,g,b color.
  // Set all the pixel in array pix to the given r,g,b.
  // If pix is not set or NULL, use the default pixels array.
  if(pix == NULL) pix=pixels;

  //  uint24_t color = r << 16 | g << 8 | b;
  for(uint16_t i=0; i<MAX_Z; ++i){
    uint16_t n = 3*i;
    pix[n+rOffset] = r;
    pix[n+gOffset] = g;
    pix[n+bOffset] = b;
  }
};

void LEDBall::deltaBaseColor(int16_t r,int16_t g,int16_t b,uint8_t *pix){ // Change all pixels by r,g,b in color space.
  
  if(pix == NULL) pix=pixels;
  for(uint16_t i=0; i<MAX_Z; ++i){
    uint16_t n = 3*i;
    pix[n+rOffset] += r;
    pix[n+gOffset] += g;
    pix[n+bOffset] += b;
  }
};

void LEDBall::deltaPixelColor(uint16_t i,int16_t r,int16_t g,int16_t b,uint8_t *pix){ // Change all pixels by r,g,b in color space.
  if(pix == NULL) pix=pixels;
  uint16_t n=3*i;
  pix[n+rOffset] = r;
  pix[n+gOffset] = g;
  pix[n+bOffset] = b;
 };

#ifdef EXTRA_STORE
void LEDBall::alloc_store(){
  if(numBytes <=1) return;
  if(pixels_st) free(pixels_st);
  if((pixels_st = (uint8_t *)malloc(numBytes))) {
    memset(pixels_st, 0x12, numBytes);
  }
}

void LEDBall::swap_store(){
    // Trickier, swap the pixels and the pixels_st.
    
    // Memory intensive method. Not sure if malloc is fast enough for this.
    // This causes a memory overrun and a crash if MAX_Z is too large.
    //    uint8_t *pix_tmp = (uint8_t *)malloc(MAX_Z*3*sizeof(uint8_t));
    //    memcpy(pixels,pix_tmp,MAX_Z*3*sizeof(uint8_t));
    //    memcpy(pixels_st,pixels,MAX_Z*3*sizeof(uint8_t));
    //    memcpy(pix_tmp,pixels_st,MAX_Z*3*sizeof(uint8_t));
    //    free(pix_tmp);
  if(pixels_st == NULL)return;
  uint8_t top_pix=pixels[0];
  for(uint16_t i=0; i<MAX_Z*3; ++i){
    top_pix=pixels[i];
    pixels[i] = pixels_st[i];
    pixels_st[i] = top_pix;
  }
}
#endif
