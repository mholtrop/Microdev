/*
 * LEDBall.cpp
 *
 *  Created on: Nov 11, 2016
 *      Author: Maurik Holtrop
 */

#include "LEDBall.hpp"
#include "uart.hpp"

LEDBall::LEDBall(): NeoPixel() {
	

}

LEDBall::LEDBall(uint8_t p,volatile uint8_t *pt,neoPixelType t): NeoPixel(MAX_Z,p,pt,t){
  
}

LEDBall::~LEDBall() {
	
}
//
// NOTE: On the 8-bit AVR, a double and a float are both size 4 = 32 bit.
//
void LEDBall::Temp_to_RGB(uint16_t T, uint8_t *r, uint8_t *g, uint8_t *b){
  // Adapted from the UFRAW lib: https://github.com/sergiomb2/ufraw/blob/56b81e2fa2a8c1d8e15897ebd37467ba53201b49/ufraw_routines.c
  // The heavy use of floating point makes this a slow routing on an Arduino.
  // If you need it often, you may want to offload the task to the host CPU.
  //
  static const double XYZ_to_RGB[3][3] = {
    { 3.24071,	-0.969258,  0.0556352 },
    { -1.53726,	1.87599,    -0.203996 },
    { -0.498571,	0.0415557,  1.05707 }
  };
  
  int c;
  double xD, yD, X, Z, max;
  double RGB[3];
  
  // Fit for CIE Daylight illuminant
  if (T <= 4000) {
    //    xD = 0.27475e9 / (T * T * T) - 0.98598e6 / (T * T) + 1.17444e3 / T + 0.145986;
    xD = ((0.27475e9/T  - 0.98598e6) /T + 1.17444e3) /T + 0.145986;
  } else if (T <= 7000) {
    xD = ((-4.6070e9 / T  + 2.9678e6) / T + 0.09911e3) / T + 0.244063;
  } else {
    xD = ((-2.0064e9 /T + 1.9018e6) /T + 0.24748e3) / T + 0.237040;
  }
  yD = (-3 * xD  + 2.87) * xD - 0.275;
  
  // Fit for Blackbody using CIE standard observer function at 2 degrees
  //xD = -1.8596e9/(T*T*T) + 1.37686e6/(T*T) + 0.360496e3/T + 0.232632;
  //yD = -2.6046*xD*xD + 2.6106*xD - 0.239156;
  
  // Fit for Blackbody using CIE standard observer function at 10 degrees
  //xD = -1.98883e9/(T*T*T) + 1.45155e6/(T*T) + 0.364774e3/T + 0.231136;
  //yD = -2.35563*xD*xD + 2.39688*xD - 0.196035;
  
  X = xD / yD;
  Z = (1. - xD) / yD - 1.;
  max = 0;
  for (c = 0; c < 3; c++) {
    RGB[c] = X * XYZ_to_RGB[0][c] + XYZ_to_RGB[1][c] + Z * XYZ_to_RGB[2][c];
    if (RGB[c] > max) max = RGB[c];
  }
  max = max/255.;
  for (c = 0; c < 3; c++) RGB[c] = RGB[c] / max;
  
  printf("T=%u, xD = %lf, yD = %lf, X=%lf, Z=%lf \r\n",T,xD,yD,X,Z);
  flush();
  (*r) =(int) (RGB[0]>0?RGB[0]+0.5:0);
  (*g) =(int) (RGB[1]>0?RGB[1]+0.5:0);
  (*b) =(int) (RGB[2]>0?RGB[2]+0.5:0);
  printf("RGB = [%lf,%lf,%lf] = [%d,%d,%d]\r\n",RGB[0],RGB[1],RGB[2],*r,*g,*b);
  flush();
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

void LEDBall::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b,uint8_t *pix) {
  // Set the pixel at n for array pix.
  if(pix == NULL) pix=pixels;
  if(n < numLEDs) {
    uint8_t *p;
    p = &pix[n * 3];    // 3 bytes per pixel
    p[rOffset] = r;          // R,G,B always stored
    p[gOffset] = g;
    p[bOffset] = b;
  }
}

void LEDBall::multBaseColor(float r,float g,float b,uint8_t *pix){
  // Change all pixels by multiplication in r,g,b in color space.
  if(pix == NULL) pix=pixels;
  for(uint16_t i=0; i<MAX_Z; ++i){
    uint16_t n = 3*i;
    pix[n+rOffset] = (pix[n+rOffset]*r>255.?255:pix[n+rOffset]*r);
    pix[n+gOffset] = (pix[n+gOffset]*r>255.?255:pix[n+gOffset]*g);
    pix[n+bOffset] = (pix[n+bOffset]*r>255.?255:pix[n+bOffset]*b);
  }
};

void LEDBall::deltaBaseColor(int16_t r,int16_t g,int16_t b,uint8_t *pix){
  // Change all pixels by r,g,b in color space.
  if(pix == NULL) pix=pixels;
  for(uint16_t i=0; i<MAX_Z; ++i){
    uint16_t n = 3*i;
    pix[n+rOffset] += r;
    pix[n+gOffset] += g;
    pix[n+bOffset] += b;
  }
};

void LEDBall::multPixelColor(uint16_t i,float r,float g,float b,uint8_t *pix){
  // Change pixels i by multiplication in r,g,b in color space .
  if(pix == NULL) pix=pixels;
  uint16_t n=3*i;
  pix[n+rOffset] *= r;
  pix[n+gOffset] *= g;
  pix[n+bOffset] *= b;
};

void LEDBall::deltaPixelColor(uint16_t i,int16_t r,int16_t g,int16_t b,uint8_t *pix){
  // Change pixels i by r,g,b in color space.
  if(pix == NULL) pix=pixels;
  uint16_t n=3*i;
  pix[n+rOffset] += r;
  pix[n+gOffset] += g;
  pix[n+bOffset] += b;
 };

#ifdef EXTRA_STORE
int16_t LEDBall::alloc_store(){
  if(numBytes <=0) return -1;
  if(pixels_st) free(pixels_st);
  if((pixels_st = (uint8_t *)malloc(numBytes))) {
    memset(pixels_st, 0x12, numBytes);
    return(numBytes);
  }else{
    return -2;
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
