#!/usr/bin/env python
#
# SI7021 Temperature and Humidity Sensor module
#
# Author: Maurik Holtrop
#
# This module interfaces over the I2C bus with an SI7021 sensor.
#
# I2C bus:
# You can check for the presense of this detector on the I2C bus with the command:
# "i2cdetect 1", which returns a table of addresses. This sensor is at 40 (HEX)
#
# Datasheet: https://www.silabs.com/documents/public/data-sheets/Si7021-A20.pdf
#
# The AdaFruit breakout for this sensor: https://learn.adafruit.com/adafruit-si7021-temperature-plus-humidity-sensor
#
# This breakout includes an LDO voltage regulator to translate 5V down to 3.3. It also includes voltage
# regulators for the SDA (data) and SCL (clock) pins, which uses BSS138 MOSFET transistors to do the translation.
# This setup allows the chip to communicate with a 5V Arduino as well as RPi's at 3.3V.
#
# Hookup of breakout board:
# Most breakout boards bring out the Vin, 3.3V, SDA, SLC, GND.
# The Vin and GND are obviously for the power and ground.
# The 3.3V pin is the 3.3V output of the LDO.
# The SCL is the CLOCK, connect this to the SCK.
# The SDA on the board is SDI on the chip. It is data in/out for I2C
#
# There are different SMBus implementation for Python. The "standard" one (import smbus)
# has some limitations. Since the SI7021 behaves different from many other sensors, in
# it does not have simply register write/read operations, the standard smbus does not work well.
# This implementation with smbus2 does. If it is not installed, install it with "pip install smbus2"
# Documentation is at: https://github.com/kplindegaard/smbus2
#
from __future__ import print_function

try:
    import smbus2 as smb
    import time
except:
    print("Please install smbus2 with 'pip install smbus2' ")
    raise ModuleNotFoundError("smbus2 not found.")

class SI7021:

    # Table of useful address locations.
    _ADDRESS=0x40
    _RESET = 0xFE
    _READ_HUM= 0xF5          # Read humidity "no hold master mode"
    _READ_HUM_TEMP = 0xE0    # Read the last temp (from the hum measurement)
    _READ_TEMP=0xF3          # Read the temp "no hold master mode"
    _WRITE_USER_REG=0xE6
    _READ_USER_REG=0xE7

    def __init__(self,bus=1):
        '''Initialize the class. The only argument the bus=1 for RPi, or 0,1,2 for BeagleBone.'''

        self._dev = smb.SMBus(bus)
        self._dev.write_byte(self._ADDRESS,self._RESET)
        time.sleep(0.1)

    def _crc(self, data):
        """Compute the CRC for CRC generator polynomial of x8 + x5 + x4 + 1
         from 2 data bytes
        """
        remnant = 0
        for b in data:
            remnant ^= b
            for bit in range(8):
                if remnant & 128:
                    remnant = (remnant << 1) ^ 0x31
                else:
                    remnant = (remnant << 1)
        return(remnant & 0xFF)


    def Read_Humidity(self):
        '''Read the humidity and return as a float %'''
        read = smb.i2c_msg.read(0x40,3) # Prepare the read of 3 words.
        self._dev.write_byte(self._ADDRESS,self._READ_HUM)
        time.sleep(0.2)                 # Wait for conversion
        try:
            self._dev.i2c_rdwr(read)
        except Exception as e:          # Probably didn't wait long enough
            print(e)
        raw_val = (ord(read.buf[0])<<8 ) + ord(read.buf[1])
        humidity = 125.*raw_val/65536. - 6.
        if self._crc([ord(read.buf[0]),ord(read.buf[1])]) !=ord(read.buf[2]) :
            print("The CRC is not correct.")
        return(humidity)

    def Read_Temperature(self,reg=0xF3):
        '''Read the temperature and return as float degrees C.
        optional argument: reg = register to read from.'''
        read = smb.i2c_msg.read(0x40,3) # Prepare the read of 3 words.
        self._dev.write_byte(self._ADDRESS,self._READ_TEMP)
        time.sleep(0.2)                 # Wait for conversion
        try:
            self._dev.i2c_rdwr(read)
        except Exception as e:          # Probably didn't wait long enough
            print(e)
        raw_val = (ord(read.buf[0])<<8 ) + ord(read.buf[1])
        temperature = 175.72*raw_val/65536. - 46.85
        if self._crc([ord(read.buf[0]),ord(read.buf[1])])!=ord(read.buf[2]):
            print("The CRC is not correct.")
        return(temperature)

    def Read_Humi_Temp(self):
        '''Read the temperature and humidity, return as list [humi,temp]'''
        humi = self.Read_Humidity()
        temp = self.Read_Temperature(self._READ_HUM_TEMP)
        return([humi,temp])


    def Set_Resolution(self, res):
        """
        Sets the resolution of temperature and humidity readings.

        0 : Humidity 12 bits,  Temperature 14 bits
        1 : Humidity  8 bits,  Temperature 12 bits
        2 : Humidity 10 bits,  Temperature 13 bits
        3 : Humidity 11 bits,  Temperature 11 bits
        """
        # read register and only update resolution bits
        value = self._dev.read_byte_data(self._ADDRESS,self._READ_USER_REG)
        value_new = (value & 0x7E) | (res & 1) | ((res & 2)<<6)
        self._dev.write_byte_data(self._ADDRESS,self._WRITE_USER_REG,value_new)

    def Get_Resolution(self):
        """
        Gets the resolution of temperature and humidity readings.

        0 : Humidity 12 bits,  Temperature 14 bits
        1 : Humidity  8 bits,  Temperature 12 bits
        2 : Humidity 10 bits,  Temperature 13 bits
        3 : Humidity 11 bits,  Temperature 11 bits
        """
        value = self._dev.read_byte_data(self._ADDRESS,self._READ_USER_REG)
        stat = ((value[0]&128)>>6) | (value[0]&1)
        return stat

    def Set_Heater_Level(self, level):
        """
        Sets the heating level to be used if the heater
        is switched on.

        0:  3.09 mA
        1:  9.18 mA
        ....
        2: 15.24 mA
        ....
        4: 27.39 mA
        ....
        8: 51.69 mA
        ....
        15: 94.20 mA
        """
        # read register and only update heater bits
        val=self._dev.read_byte_data(self._ADDRESS,0x11)
        v = (val & 0xF0) | (level & 0x0F)
        self._dev.write_byte_data(self._ADDRESS,0x51, v)

    def Get_Heater_Level(self):
        """
        Gets the heating level to be used if the heater
        is switched on.

        0:  3.09 mA
        1:  9.18 mA
        2: 15.24 mA
        ....
        4: 27.39 mA
        ....
        8: 51.69 mA
        ....
        15: 94.20 mA
        """
        level = self._dev.read_byte_data(self._ADDRESS,0x11) # level
        return level&0x0F

    def Heater_on(self):
        """
        Switches the heater on.
        """
        # read register and only update heater bit
        val=self._dev.read_byte_data(self._ADDRESS, 0xE7)
        v = (val & 0xFB) | 4
        self._dev.write_byte_data(self._ADDRESS,0xE6, v)

    def Heater_off(self):
        """
        Switches the heater off.
        """
        # read register and only update heater bit
        # read register and only update heater bit
        val=self._dev.read_byte_data(self._ADDRESS, 0xE7)
        v = (val & 0xFB)
        self._dev.write_byte_data(self._ADDRESS,0xE6, v)

    def Firmware_Revision(self):
        """
        Returns the firmware revision.

        A value of 0xFF means revision 1.0, a value of 0x20 means
        revision 2.0.
        """
        write= smb.i2c_msg.write(0x40,[0x84, 0xB8])
        read = smb.i2c_msg.read(0x40,1)
        self._dev.i2c_rdwr(write,read) # id1
        if ord(read.buf[0]) == 0xFF:
            return("V 1.0")
        elif ord(read.buf[0]) == 0x20:
            return("V 2.0")
        else:
            return("V ???")

    def Electronic_id1(self):
        """
        Returns the first four bytes of the electronic Id.
        If the CRC is incorrect 0 is returned.
        """
        write= smb.i2c_msg.write(0x40,[0xFA, 0x0F])
        read = smb.i2c_msg.read(0x40,8)
        self._dev.i2c_rdwr(write,read) # id1
        id1 = [ ord(read.buf[i]) for i in range(8)]
        if self._crc([id1[0], id1[2], id1[4], id1[6]]) == id1[7]:
            return (id1[0]<<24)|(id1[2]<<16)|(id1[4]<<8)|id1[6]
        else:
            return 0

    def Electronic_id2(self):
        """
        Returns the second four bytes of the electronic Id.
        If the CRC is incorrect 0 is returned.
        """
        write= smb.i2c_msg.write(0x40,[0xFC, 0xC9])
        read = smb.i2c_msg.read(0x40,8)
        self._dev.i2c_rdwr(write,read) # id2
        id2 = [ ord(read.buf[i]) for i in range(8)]
        if self._crc([id2[0], id2[1], id2[3], id2[4]]) == id2[5]:
            return (id2[0]<<24)|(id2[1]<<16)|(id2[3]<<8)|id2[4]
        else:
            return 0

    def Part_number(self):
        '''Returns which part number is connected. Either
        Si7013, Si7020 or Si7021 '''
        id2 = self.Electronic_id2()
        id2 = id2>>24
        if id2 == 13:
            return('Si7013')
        elif id2 == 20:
            return('Si7020')
        elif id2 == 21:
            return('Si7021')
        else:
            return('Engineering sample {}'.format(id2))

def main():
    '''Main code for testing.'''

    SI = SI7021()
    [humi,temp] = SI.Read_Humi_Temp()
    print(" {:^7}    {:^7}".format("Temp [C]","Humidity [%]"))
    print(" {:>7.3f}    {:>7.2f}".format(temp,humi))

    firmw = SI.Firmware_Revision()
    print("Firmware rev {}".format(firmw))
    id1 = SI.Electronic_id1()
    id2 = SI.Electronic_id2()
    print("Electronic ID: 0x{:x} 0x{:x}".format(id1,id2))


if __name__ == '__main__':
    main()
