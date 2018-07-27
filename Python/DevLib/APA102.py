#!/usr/bin/env python
#
# MCP320x
#
# Author: Maurik Holtrop
#
# This module is a pure Python version to write to the APA102 LEDS.
#
# The code borrowed from Martin Erzberger's version: https://github.com/tinue/APA102_Pi/blob/master/apa102.py
# But is a complete re-implementation.
#
# APA102 Datasheet:
# AdaFruit info: https://learn.adafruit.com/adafruit-dotstar-leds
#
# A note on these LEDS:
# Despite what the AdaFruit folks say, you *can* actually control this type of LED with 3.3V logic.,
# just not very well. This works best if you run the LEDS at 3.3V, though they will not be quite as bright, they
# will work OK and can be tested in that mode.
# However, if you want this to run glitch free at high data rates and full brightness, 5V power and logic are recommended.
#
try:
    import RPi.GPIO as GPIO
except:
    pass
try:
    import Adafruit_BBIO as GPIO
except:
    pass

import spidev
import BBSpiDev

import numpy as np
from math import ceil

class APA102:
    """
    Driver for APA102 LEDS
    """

    def __init__(self,num_leds=1,CS=None,CLK=None,MOSI=None):
        """Inialize the APA102 class.
        Parameters:
        num_leds     - integer: the number of leds in the string
                     - (x,y)  : the number of leds in matrix of x,y dimension
                       If you set the num_leds as (x,y) you can set pixels as (x,y) as well.
                       This uses np.matrix to set the LED array, leds.
        CS           - Dummy - not used by APA102 LEDS, so set to None.
                       If set to 0 or 1, the HW CS will be used. You can use an NAND gate
                       and an INVERTER to effectively add a CS line to the APA102 string.
        CLK          - For HW SPI this is the clock speed.
                       For Software SPI, this it the PIN with the clock signal.
        MOSI         - GPIO pin number for data out. Use None for hardware SPI.
        """

        if type(num_leds) is int:
            self.leds = [0]*num_leds
        elif type(num_leds) is tuple or type(num_leds) is list:
            self.leds = np.zeros(num_leds,dtype=long)

        self.DATA = MOSI
        self.CS_bar = 0
        self.CLK = CLK
        self._dev = None
        self.global_brightness = 10

        if self.DATA is None:
            if self.CLK < 1:
                self.CLK=1000000
            # Initialize the SPI hardware device
            self._dev = spidev.SpiDev(0,self.CS_bar)
            self._dev.max_speed_hz = self.CLK
        else:
            self._dev = BBSpiDev.BBSpiDev(self.CS_bar,self.CLK,self.DATA,None)

    def zero(self):
        """Set all the LEDS to zero, i.e. dark, but do not call show() """
        if type(self.leds) is list:
            for led in range(len(self.leds)):
                self.set_pixel(led, 0, 0)
        elif type(self.leds) is np.ndarray or type(self.leds) is np.matrix:
            self.leds.fill(0b11100000<<24)

    def clear(self):
        """ Turns off the strip and shows the result right away."""
        self.zero()
        self.show()

    def set_pixel(self, loc, col, bright=1.):
        """Sets the color of one pixel in the LED stripe.
        params:
        loc    :  The location of the led to set. Can be an int
                  or an (x,y) location, depending on initialization.
        col    :  if col is an integer -> an rgb HEX color value.
               :  if col is tuple      -> (r,g,b) HEX triplet.
        bright :  The overall brightness fraction of global_brightness.
                  1. = MAX brightness.

        This sets the pixel in the leds array. Pixels will be shown with
        the show() function.
        """
        if type(col) is list or type(col) is tuple:
            rgb_color = self.rgb_hex(col)
        elif type(col) is int:
            rgb_color = col
        else:
            raise ValueError("set_pixel requires an integer or (r,g,b) tuple")

        # Calculate pixel brightness as a percentage of the
        # defined global_brightness. Round up to nearest integer
        # as we expect some brightness unless set to 0
        brightness = int(ceil(bright*self.global_brightness/100.0))
        # LED startframe is three "1" bits, followed by 5 brightness bits
        ledstart = (brightness & 0b00011111) | 0b11100000

        if type(self.leds) is list:
            self.leds[loc] = (ledstart<<24) + rgb_color
        elif type(self.leds) is np.ndarray or type(self.leds) is np.matrix:
            self.leds.itemset(loc,(ledstart<<24) + rgb_color)

    def roll(self, positions=1,axis=None):
        """Roll the LEDs by the specified number of positions.
        For (x,y) arrays, if axis=0 roll in the x shape, and
        if axis=1 roll in the y shape. If axis is None, roll
        as if linear.
        """
        if type(self.leds) is list:
            cutoff = (-positions % len(self.leds))
            self.leds = self.leds[cutoff:] + self.leds[:cutoff]
        elif type(self.leds) is np.ndarray or type(self.leds) is np.matrix:
            self.leds = np.roll(self.leds,positions,axis=axis)

    def show(self):
        """Sends the content of the pixel buffer to the strip.
        Todo: More than 1024 LEDs requires more than one xfer operation.
        """
        self._dev.writebytes([0]*4) # Write 32 zeros for the start frame.

        # We need to unpack the array of 32bit ints into 8bit bytes, because
        # spidev can only handle 8bit byte arrays. You can can do this the slow
        # way as:
        # for x in self.leds:
        #     for i in [(x>>24)&0xFF,(x>>16)&0xFF,(x>>8)&0xFF,x&0xFF]:
        #       vals.append(i)
        # It is faster/simpler to use list comprehension, in this case
        # a nested list comprehension.
        # In the process we create a copy, which for spidev is good thing.
        # If you run the clock at 8MHz, it takes 6 to 7 ms for show() with 256 LEDs on an RPi3.
        # SPI takes up to 4096 Integers. So we are fine for up to 1024 LEDs.
        if type(self.leds) is list:
            self._dev.writebytes([ int(i) for x in self.leds for i in [(x>>24)&0xFF,(x>>16)&0xFF,(x>>8)&0xFF,x&0xFF]])
        elif type(self.leds) is np.ndarray or type(self.leds) is np.matrix:
            self._dev.writebytes([ int(i) for x in self.leds.flatten() for i in [(x>>24)&0xFF,(x>>16)&0xFF,(x>>8)&0xFF,x&0xFF]])
        # To ensure that all the data gets to all the LEDS, we need to
        # continue clocking. The easiest way to do so is by sending zeros.
        # Round up num_led/2 bits (or num_led/16 bytes)
        self._dev.writebytes([0]*((len(self.leds) + 15) // 16))


    def cleanup(self):
        """Release the SPI device; Call this method at the end"""
        self._dev.close()  # Close SPI port

    @staticmethod
    def rgb_hex(t):
        """Make one 3*8 byte color value from an (r,g,b) tuple
        of HEX r,g,b values, i.e. (r,g,b) are 8-bit integers [0,255]"""
        return(((t[2]&0xFF)<<16)+((t[1]&0xFF)<<8)+(t[0]&0xFF))

    @staticmethod
    def rgb(t):
        """Make one 3*8 byte color value from an (r,g,b) tuple
        of float r,g,b values, i.e. (r,g,b) are [0.,1.]"""
        return(self.rgb_hex([ int(i*256) for i in t]))

    @staticmethod
    def hls(t):
        """Make one 3*8 byte color value from an (h,l,s) tuple.
        Note that (h,l,s) are [0.,1.0], floats between 0 and 1."""
        return(self.rgb(colorsys.hls_to_rgb(t[0],t[1],t[2])))
