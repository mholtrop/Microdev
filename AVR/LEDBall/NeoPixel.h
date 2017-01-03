/*--------------------------------------------------------------------
  This file is part of the Adafruit NeoPixel library.

  NeoPixel is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of
  the License, or (at your option) any later version.

  NeoPixel is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with NeoPixel.  If not, see
  <http://www.gnu.org/licenses/>.
  --------------------------------------------------------------------*/
/*--------------------------------------------------------------------
 This Library was quite seriously hacked from the original by Maurik Holtrop
 Reason: Get rid of all the Arduino overhead, make it smaller, 16 MHz AVR only.
 Though I think the Arduino is absolutely a fantastic product, I am using the
 bare chip and need more direct use of the chip, without the Arduino libraries 
 interfering.
 */

#ifndef NEOPIXEL_H
#define NEOPIXEL_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h> // Defs for memset
#include <avr/io.h>
#include <avr/interrupt.h>

// The order of primary colors in the NeoPixel data stream can vary
// among device types, manufacturers and even different revisions of
// the same item.  The third parameter to the NeoPixel
// constructor encodes the per-pixel byte offsets of the red, green
// and blue primaries (plus white, if present) in the data stream --
// the following #defines provide an easier-to-use named version for
// each permutation.  e.g. NEO_GRB indicates a NeoPixel-compatible
// device expecting three bytes per pixel, with the first byte
// containing the green value, second containing red and third
// containing blue.  The in-memory representation of a chain of
// NeoPixels is the same as the data-stream order; no re-ordering of
// bytes is required when issuing data to the chain.

// Bits 5,4 of this value are the offset (0-3) from the first byte of
// a pixel to the location of the red color byte.  Bits 3,2 are the
// green offset and 1,0 are the blue offset.  If it is an RGBW-type
// device (supporting a white primary in addition to R,G,B), bits 7,6
// are the offset to the white byte...otherwise, bits 7,6 are set to
// the same value as 5,4 (red) to indicate an RGB (not RGBW) device.
// i.e. binary representation:
// 0bWWRRGGBB for RGBW devices
// 0bRRRRGGBB for RGB

// RGB NeoPixel permutations; white and red offsets are always same
// Offset:         W          R          G          B
#define NEO_RGB  ((0 << 6) | (0 << 4) | (1 << 2) | (2))
#define NEO_RBG  ((0 << 6) | (0 << 4) | (2 << 2) | (1))
#define NEO_GRB  ((1 << 6) | (1 << 4) | (0 << 2) | (2))
#define NEO_GBR  ((2 << 6) | (2 << 4) | (0 << 2) | (1))
#define NEO_BRG  ((1 << 6) | (1 << 4) | (2 << 2) | (0))
#define NEO_BGR  ((2 << 6) | (2 << 4) | (1 << 2) | (0))

#define NEO_KHZ800 0x0000 // 800 KHz datastream

typedef uint8_t  neoPixelType;

class NeoPixel {

 public:

  // Constructor: number of LEDs, PORTD bit number, LED type
  NeoPixel(uint16_t n, uint8_t p=4, volatile uint8_t *PORT= &PORTC,neoPixelType t=NEO_GRB + NEO_KHZ800);
  NeoPixel(void);
  ~NeoPixel();

  void begin(void);
  void show(void);
  void setShowAll(uint8_t r, uint8_t g, uint8_t b);
  void setPinPort(uint8_t p, volatile uint8_t *pt);
  void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
  void setPixelColor(uint16_t n, uint32_t c);
  void clear();
  void updateLength(uint16_t n);
  void updateType(neoPixelType t);
  uint8_t *getPixels(void) const;
  int8_t  getPin(void) { return ffs(pinMask)-1; }; // ffs, gcc builtin: Returns one plus the index of the least significant 1-bit of x, or if x is zero, returns zero.
  volatile uint8_t *getPort(void){ return port;};
  uint16_t numPixels(void) const;
  static uint32_t Color(uint8_t r, uint8_t g, uint8_t b);
  uint32_t getPixelColor(uint16_t n) const;
  inline bool canShow(void) { return ((TCNT1-endTime)>100); } // Assumes the Timer 1 runs at 2MHz, this is 50us.

// private:

  bool
    begun;         // true if begin() previously called
  uint16_t
    numLEDs,       // Number of RGB LEDs in strip
    numBytes;      // Size of 'pixels' buffer below (3 or 4 bytes/pixel)
  uint8_t
   *pixels,        // Holds LED color values (3 or 4 bytes each)
    rOffset,       // Index of red byte within each 3- or 4-byte pixel
    gOffset,       // Index of green byte
    bOffset;       // Index of blue byte
  uint16_t
    endTime;       // Latch timing reference
#ifdef __AVR__
  volatile uint8_t
    *port;         // Output PORT register
  uint8_t
    pinMask;       // Output PORT bitmask
#endif

};

#endif // NEOPIXEL_H
