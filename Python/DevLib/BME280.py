#!/usr/bin/env python
#
# BME280/BMP280 Sensor module
#
# Author: Maurik Holtrop
#
# This module interfaces over the I2C bus with an BMP280 or BME280 sensor.
# The only difference between these sensors is that the BME also includes a
# humidity sensor, while the BMP version has only temperature and pressure.
#
# I2C bus:
# You can check for the presense of this detector on the I2C bus with the command:
# "i2cdetect 1", which returns a table of addresses. This sensor is at 76 (HEX)
#
# Datasheet: https://www.bosch-sensortec.com/bst/products/all_products/bme280
# Official C reference driver: https://www.bosch-sensortec.com/bst/products/all_products/bme280
#
# Hookup of breakout board:
# Most breakout boards bring out the VCC, GND, SCL, SDA, CSB and SDO
# The VCC and GND are obviously for the power (3.3V) and ground.
# The SCL is the CLOCK, connect this to the SCK.
# The SDA on the board is SDI on the chip. It is data in/out for I2C
# (It is also data in for SPI, but this driver does not support SPI at the moment.)
# The CSB is chip select bar, it also selects between I2C and SPI. For I2C it needs to be pulled high, to VCC
# during power up. If not, the device will be locked in SPI mode.
# The SDO pin is used to select the address. If low, the address is 1110110 (0x76), if high 1110111 (0x77).
# In SPI mode, the SDO pin is used as the SPI data output.
#
#

import smbus
from ctypes import c_short
from ctypes import c_byte
from ctypes import c_ubyte

class BME280:

    # Table of useful address locations.
    _DEVICE_ID  = 0xD0
    _CONFIG_REG = 0xF5


    def __init__(self,address=0x76):
        '''Initialize the class. The only argument is the I2C address, which is either 0x76 (SDO pin low)
        or 0x77 (SDO pin high)'''

        self._dev = smbus.SMBus(1)
        self._dev_address = address
        # Get the device ID from the device.
        self._dev_id      =self._dev.read_i2c_block_data(self._dev_address,self._DEVICE_ID,1)[0]

    def Read_Calibrations(self):
        '''Read and store the calibration data from the device.
        This is needed for the corrections to the raw values from the device.'''

        # Read the two data blocks.
        Calbuf1 = self._dev.read_i2c_block_data(self._dev_address,0x88,26)
        Calbuf2 = self._dev.read_i2c_block_data(self._dev_address,0xE1,8)

        # Parse the pairs into signed short integers (int16_t).
        # The first 3 are for temperature, the next 9 are for pressure.
        self._Cal_TP = [ c_short((Calbuf1[i*2+1]<<8) + Calbuf1[i*2]).value for i in range(len(Calbuf1)//2)]
        # Fixup the unsigned shorts (uint16_t)
        self._Cal_TP[0]=(Calbuf1[1]<<8) + Calbuf1[0]
        self._Cal_TP[3]=(Calbuf1[7]<<8) + Calbuf1[6]

        # The humidity numbers are in the second block, except the first number.
        # These numbers are packed more complicated.
        self._Cal_H=[]
        self._Cal_H.append(Calbuf1[25])
        self._Cal_H.append(c_short((Calbuf2[1]<<8) + Calbuf2[0]).value)
        self._Cal_H.append(Calbuf2[2])
        self._Cal_H.append(c_short( (Calbuf2[3]<<4) | (Calbuf2[4] & 0x0F) ).value)
        self._Cal_H.append(c_short( (Calbuf2[5]<<4) | (Calbuf2[4] >> 4  ) ).value)
        self._Cal_H.append(c_byte(  Calbuf2[6]).value)

    def Read_Data(self):
        '''Read the Temperature, Pressure and Humidity (if BME device) from the device,
        and return the calibrated values.
        The numbers will depend on the oversampling settings for the device, which
        normally are 1,1,1.'''
        
