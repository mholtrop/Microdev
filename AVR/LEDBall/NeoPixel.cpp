/*-------------------------------------------------------------------------
  Arduino library to control a wide variety of WS2811- and WS2812-based RGB
  LED devices such as Adafruit FLORA RGB Smart Pixels and NeoPixel strips.
  Currently handles 400 and 800 KHz bitstreams on 8, 12 and 16 MHz ATmega
  MCUs, with LEDs wired for various color orders.  Handles most output pins
  (possible exception with upper PORT registers on the Arduino Mega).

  Written by Phil Burgess / Paint Your Dragon for Adafruit Industries,
  contributions by PJRC, Michael Miller and other members of the open
  source community.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing products
  from Adafruit!

  -------------------------------------------------------------------------
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
  -------------------------------------------------------------------------*/
/*--------------------------------------------------------------------
 This Library was quite seriously hacked from the original by Maurik Holtrop
 Reason: Get rid of all the Arduino overhead.
 Though I think the Arduino is absolutely a fantastic product, I am using the
 bare chip and need more direct use of the chip, without the Arduino libraries
 interfering.
 */

#include "NeoPixel.h"
#include <stdio.h>

// Constructor when length, pin and type are known at compile-time:
NeoPixel::NeoPixel(uint16_t n, uint8_t p,volatile uint8_t *pt, neoPixelType t) :
  begun(false), pixels(NULL), endTime(0)
{
  updateType(t);
  updateLength(n);
  setPinPort(p,pt);
}

// via Michael Vogt/neophob: empty constructor is used when strand length
// isn't known at compile-time; situations where program config might be
// read from internal flash memory or an SD card, or arrive via serial
// command.  If using this constructor, MUST follow up with updateType(),
// updateLength(), etc. to establish the strand type, length and pin number!
NeoPixel::NeoPixel() :
  begun(false), numLEDs(0), numBytes(0), pixels(NULL),rOffset(1), gOffset(0), bOffset(2),
  endTime(0), port(NULL), pinMask(0)
{
}

NeoPixel::~NeoPixel() {
  if(pixels)   free(pixels);
  if(pinMask > 0){
   //pinMode(pin, INPUT);
    (*port) &= ~pinMask;
  }
}

void NeoPixel::begin(void) {
  if(pinMask > 0) {
//    pinMode(pin, OUTPUT);
//    digitalWrite(pin, LOW);
    (*(port+1)) |= pinMask;
    (*port)    &= ~pinMask;
  }
  begun = true;
}

void NeoPixel::updateLength(uint16_t n) {
  if(pixels) free(pixels); // Free existing data (if any)

  // Allocate new data -- note: ALL PIXELS ARE CLEARED
  numBytes = n * 3;
  if((pixels = (uint8_t *)malloc(numBytes))) {
    memset(pixels, 0, numBytes);
    numLEDs = n;
  } else {
    numLEDs = numBytes = 0;
  }
}

void NeoPixel::updateType(neoPixelType t) {

  rOffset = (t >> 4) & 0b11; // regarding R/G/B/W offsets
  gOffset = (t >> 2) & 0b11;
  bOffset =  t       & 0b11;

}

void NeoPixel::setShowAll(uint8_t red, uint8_t gre, uint8_t blu) {
  // Set all the LEDS to r,g,b without putting the data in the pixel array first.
  if(!pixels) return;
  while(!canShow());
#if (F_CPU >= 15400000UL) && (F_CPU <= 20000000L)

  volatile uint8_t rgb[3];
  rgb[rOffset]=red;
  rgb[gOffset]=gre;
  rgb[bOffset]=blu;
  
  volatile uint16_t i   = numBytes/3; // Loop counter
  volatile uint8_t  *ptr = rgb;     // Pointer to next byte
  volatile uint8_t  byte = *ptr++;   // Current byte value
  volatile uint8_t  hi;             // PORT w/output bit set high
  volatile uint8_t  lo;             // PORT w/output bit set low
  
  volatile uint8_t next, bit;
  
  hi   = *port |  pinMask;
  lo   = *port & ~pinMask;
  next = lo;
  bit  = 8;
  //
  // 20 inst. clocks per bit: HHHHHxxxxxxxxLLLLLLL
  // ST instructions:         ^   ^        ^       (T=0,5,13)

  //
  // This is 3x longer than it needs to be, but hey, for now...
  //
  asm volatile(
               "mov   %[byte] , %[green]"   "\n\t"  //      byte = red
               "head30:"                  "\n\t" // Clk  Pseudocode    (T =  0)
               "st   %a[port],  %[hi]"    "\n\t" // 2    PORT = hi     (T =  2)
               "sbrc %[byte],  7"         "\n\t" // 1-2  if(b & 128)
               "mov  %[next], %[hi]"      "\n\t" // 0-1   next = hi    (T =  4)
               "dec  %[bit]"              "\n\t" // 1    bit--         (T =  5)
               "st   %a[port],  %[next]"  "\n\t" // 2    PORT = next   (T =  7)
               "mov  %[next] ,  %[lo]"    "\n\t" // 1    next = lo     (T =  8)
               "breq nextbyte30"          "\n\t" // 1-2  if(bit == 0) (from dec above)
               "rol  %[byte]"             "\n\t" // 1    b <<= 1       (T = 10)
               "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 12)
               "nop"                      "\n\t" // 1    nop           (T = 13)
               "st   %a[port],  %[lo]"    "\n\t" // 2    PORT = lo     (T = 15)
               "nop"                      "\n\t" // 1    nop           (T = 16)
               "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 18)
               "rjmp head30"              "\n\t" // 2    -> head20 (next bit out)

               "nextbyte30:"               "\n\t"//                    (T = 10)
               "ldi  %[bit]  ,  8"        "\n\t" // 1    bit = 8       (T = 11)
               "mov   %[byte] ,  %[red]" "\n\t"// 1    byte = green  (T = 12)
               "nop"                      "\n\t" // 1    nop           (T = 13)
               "st   %a[port], %[lo]"     "\n\t" // 2    PORT = lo     (T = 15)
               "nop"                      "\n\t" // 1    nop           (T = 16)
               "nop"                      "\n\t" // 1    nop           (T = 17)
               "nop"                      "\n\t" // 1    nop           (T = 18)
               "nop"                      "\n\t" // 1                  (T = 19)
               "nop"                      "\n\t" // 1                  (T = 20)

               "head31:"                  "\n\t"
               "st   %a[port],  %[hi]"    "\n\t" // 2    PORT = hi     (T =  2)
               "sbrc %[byte],  7"         "\n\t" // 1-2  if(b & 128)
               "mov  %[next], %[hi]"     "\n\t" // 0-1   next = hi    (T =  4)
               "dec  %[bit]"              "\n\t" // 1    bit--         (T =  5)
               "st   %a[port],  %[next]"  "\n\t" // 2    PORT = next   (T =  7)
               "mov  %[next] ,  %[lo]"    "\n\t" // 1    next = lo     (T =  8)
               "breq nextbyte31"          "\n\t" // 1-2  if(bit == 0) (from dec above)
               "rol  %[byte]"             "\n\t" // 1    b <<= 1       (T = 10)
               "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 12)
               "nop"                      "\n\t" // 1    nop           (T = 13)
               "st   %a[port],  %[lo]"    "\n\t" // 2    PORT = lo     (T = 15)
               "nop"                      "\n\t" // 1    nop           (T = 16)
               "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 18)
               "rjmp head31"              "\n\t" // 2    -> head20 (next bit out)
               
               "nextbyte31:"               "\n\t" //                    (T = 10)
               "ldi  %[bit]  ,  8"        "\n\t" // 1    bit = 8       (T = 11)
               "mov   %[byte] ,  %[blue]"  "\n\t" // 2    byte = blue   (T = 13)
               "nop"                      "\n\t" // 1    nop           (T = 14)
               "st   %a[port], %[lo]"     "\n\t" // 2    PORT = lo     (T = 15)
               "nop"                      "\n\t" // 1    nop           (T = 16)
               "nop"                      "\n\t" // 1                  (T = 17)
               "nop"                      "\n\t" // 1                  (T = 18)
               "nop"                      "\n\t" // 1                  (T = 19)
               "nop"                      "\n\t" // 1                  (T = 20)

               "head32:"                  "\n\t"
               "st   %a[port],  %[hi]"    "\n\t" // 2    PORT = hi     (T =  2)
               "sbrc %[byte],  7"         "\n\t" // 1-2  if(b & 128)
               "mov  %[next], %[hi]"     "\n\t" // 0-1   next = hi    (T =  4)
               "dec  %[bit]"              "\n\t" // 1    bit--         (T =  5)
               "st   %a[port],  %[next]"  "\n\t" // 2    PORT = next   (T =  7)
               "mov  %[next] ,  %[lo]"    "\n\t" // 1    next = lo     (T =  8)
               "breq nextbyte32"          "\n\t" // 1-2  if(bit == 0) (from dec above)
               "rol  %[byte]"             "\n\t" // 1    b <<= 1       (T = 10)
               "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 12)
               "nop"                      "\n\t" // 1    nop           (T = 13)
               "st   %a[port],  %[lo]"    "\n\t" // 2    PORT = lo     (T = 15)
               "nop"                      "\n\t" // 1    nop           (T = 16)
               "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 18)
               "rjmp head32"              "\n\t" // 2    -> head20 (next bit out)
               
               "nextbyte32:"               "\n\t" //                   (T = 10)
               "ldi  %[bit], 8"            "\n\t" // 1   reset bit cnt (T = 11)
               "mov   %[byte] ,  %[green]" "\n\t"   // 2    byte = red  (T = 13)
               "nop"                      "\n\t" // 1    nop           (T = 14)
               "st   %a[port], %[lo]"     "\n\t" // 2    PORT = lo     (T = 15)
//               "nop"                      "\n\t" // 1    nop           (T = 16)
               "sbiw %[count], 1"         "\n\t" // 2    i--           (T = 18)
               "breq  mfinish"            "\n\t"  // 2    if(i == 0) -> (finish)
               "jmp head30"               "\n\t"  // 2
               "mfinish:"                 "\n\t"
               "nop"                      "\n"
               : [port]  "+e" (port),
               [byte]  "+r" (byte),
               [bit]   "+r" (bit),
               [next]  "+r" (next),
               [count] "+w" (i)
               : [red]  "r" (red),
                 [green] "r" (gre),
                 [blue]  "r" (blu),
               [hi]     "r" (hi),
               [lo]     "r" (lo));
  
#else
#warning "CPU SPEED NOT SUPPORTED for setShowAll"
#endif // end F_CPU ifdefs on __AVR__
  
  
  sei(); // interrupts();
  endTime = TCNT1; // Save EOD time for latch on next call
}

void NeoPixel::show(void) {

  if(!pixels) return;

  // Data latch = 50+ microsecond pause in the output stream.  Rather than
  // put a delay at the end of the function, the ending time is noted and
  // the function will simply hold off (if needed) on issuing the
  // subsequent round of data until the latch time has elapsed.  This
  // allows the mainline code to start generating the next frame of data
  // rather than stalling for the latch.
  while(!canShow());
  // endTime is a private member (rather than global var) so that mutliple
  // instances on different pins can be quickly issued in succession (each
  // instance doesn't delay the next).

  // In order to make this code runtime-configurable to work with any pin,
  // SBI/CBI instructions are eschewed in favor of full PORT writes via the
  // OUT or ST instructions.  It relies on two facts: that peripheral
  // functions (such as PWM) take precedence on output pins, so our PORT-
  // wide writes won't interfere, and that interrupts are globally disabled
  // while data is being issued to the LEDs, so no other code will be
  // accessing the PORT.  The code takes an initial 'snapshot' of the PORT
  // state, computes 'pin high' and 'pin low' values, and writes these back
  // to the PORT register as needed.

  cli(); // noInterrupts(); // Need 100% focus on instruction timing

#ifdef __AVR__
// AVR MCUs -- ATmega & ATtiny (no XMEGA) ---------------------------------
  volatile uint16_t
    i   = numBytes; // Loop counter
  volatile uint8_t
   *ptr = pixels,   // Pointer to next byte
    b   = *ptr++,   // Current byte value
    hi,             // PORT w/output bit set high
    lo;             // PORT w/output bit set low

  // Hand-tuned assembly code issues data to the LED drivers at a specific
  // rate.  There's separate code for different CPU speeds (8, 12, 16 MHz)
  // for both the WS2811 (400 KHz) and WS2812 (800 KHz) drivers.  The
  // datastream timing for the LED drivers allows a little wiggle room each
  // way (listed in the datasheets), so the conditions for compiling each
  // case are set up for a range of frequencies rather than just the exact
  // 8, 12 or 16 MHz values, permitting use with some close-but-not-spot-on
  // devices (e.g. 16.5 MHz DigiSpark).  The ranges were arrived at based
  // on the datasheet figures and have not been extensively tested outside
  // the canonical 8/12/16 MHz speeds; there's no guarantee these will work
  // close to the extremes (or possibly they could be pushed further).
  // Keep in mind only one CPU speed case actually gets compiled; the
  // resulting program isn't as massive as it might look from source here.


  // 16 MHz(ish) AVR --------------------------------------------------------
#if (F_CPU >= 15400000UL) && (F_CPU <= 20000000L)
  
    // WS2811 and WS2812 have different hi/lo duty cycles; this is
    // similar but NOT an exact copy of the prior 400-on-8 code.

    // 20 inst. clocks per bit: HHHHHxxxxxxxxLLLLLLL
    // ST instructions:         ^   ^        ^       (T=0,5,13)

    volatile uint8_t next, bit;

    hi   = *port |  pinMask;
    lo   = *port & ~pinMask;
    next = lo;
    bit  = 8;

    asm volatile(
     "head20:"                   "\n\t" // Clk  Pseudocode    (T =  0)
      "st   %a[port],  %[hi]"    "\n\t" // 2    PORT = hi     (T =  2)
      "sbrc %[byte],  7"         "\n\t" // 1-2  if(b & 128)
       "mov  %[next], %[hi]"     "\n\t" // 0-1   next = hi    (T =  4)
      "dec  %[bit]"              "\n\t" // 1    bit--         (T =  5)
      "st   %a[port],  %[next]"  "\n\t" // 2    PORT = next   (T =  7)
      "mov  %[next] ,  %[lo]"    "\n\t" // 1    next = lo     (T =  8)
      "breq nextbyte20"          "\n\t" // 1-2  if(bit == 0) (from dec above)
      "rol  %[byte]"             "\n\t" // 1    b <<= 1       (T = 10)
      "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 12)
      "nop"                      "\n\t" // 1    nop           (T = 13)
      "st   %a[port],  %[lo]"    "\n\t" // 2    PORT = lo     (T = 15)
      "nop"                      "\n\t" // 1    nop           (T = 16)
      "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 18)
      "rjmp head20"              "\n\t" // 2    -> head20 (next bit out)
     "nextbyte20:"               "\n\t" //                    (T = 10)
      "ldi  %[bit]  ,  8"        "\n\t" // 1    bit = 8       (T = 11)
      "ld   %[byte] ,  %a[ptr]+" "\n\t" // 2    b = *ptr++    (T = 13)
      "st   %a[port], %[lo]"     "\n\t" // 2    PORT = lo     (T = 15)
      "nop"                      "\n\t" // 1    nop           (T = 16)
      "sbiw %[count], 1"         "\n\t" // 2    i--           (T = 18)
       "brne head20"             "\n"   // 2    if(i != 0) -> (next byte)
      : [port]  "+e" (port),
        [byte]  "+r" (b),
        [bit]   "+r" (bit),
        [next]  "+r" (next),
        [count] "+w" (i)
      : [ptr]    "e" (ptr),
        [hi]     "r" (hi),
        [lo]     "r" (lo));

#else
 #error "CPU SPEED NOT SUPPORTED"
#endif // end F_CPU ifdefs on __AVR__

// END AVR ----------------------------------------------------------------

#endif


// END ARCHITECTURE SELECT ------------------------------------------------

  sei(); // interrupts();
  endTime = TCNT1; // Save EOD time for latch on next call
}

// Set the output pin number and port.
void NeoPixel::setPinPort(uint8_t p,volatile uint8_t *pt){
  pinMask = _BV(p);
  port=pt;
  *(port-1) |= pinMask; // Set the DDR to output.
  *port &= ~pinMask;    // Set pin to zero.
}

// Set pixel color from separate R,G,B components:
void NeoPixel::setPixelColor(
 uint16_t n, uint8_t r, uint8_t g, uint8_t b) {

  if(n < numLEDs) {
    uint8_t *p;
    p = &pixels[n * 3];    // 3 bytes per pixel
    p[rOffset] = r;          // R,G,B always stored
    p[gOffset] = g;
    p[bOffset] = b;
  }
}

// Set pixel color from 'packed' 32-bit RGB color:
void NeoPixel::setPixelColor(uint16_t n, uint32_t c) {
  if(n < numLEDs) {
    uint8_t *p,
    r = (uint8_t)(c >> 16),
    g = (uint8_t)(c >>  8),
    b = (uint8_t)c;
    
    p = &pixels[n * 3];
    
    p[rOffset] = r;
    p[gOffset] = g;
    p[bOffset] = b;
  }
}

// Convert separate R,G,B into packed 32-bit RGB color.
// Packed format is always RGB, regardless of LED strand color order.
uint32_t NeoPixel::Color(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
}

// Query color from previously-set pixel (returns packed 32-bit RGB value)
uint32_t NeoPixel::getPixelColor(uint16_t n) const {
  if(n >= numLEDs) return 0; // Out of bounds, return no color.

  uint8_t *p;

  p = &pixels[n * 3];
    // No brightness adjustment has been made -- return 'raw' color
  return ((uint32_t)p[rOffset] << 16) |
          ((uint32_t)p[gOffset] <<  8) |
          (uint32_t)p[bOffset];
}

// Returns pointer to pixels[] array.  Pixel data is stored in device-
// native format and is not translated here.  Application will need to be
// aware of specific pixel data format and handle colors appropriately.
uint8_t *NeoPixel::getPixels(void) const {
  return pixels;
}

uint16_t NeoPixel::numPixels(void) const {
  return numLEDs;
}

void NeoPixel::clear() {
  memset(pixels, 0, numBytes);
}













