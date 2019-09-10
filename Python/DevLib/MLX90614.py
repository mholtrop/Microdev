#!/usr/bin/env python
#
# Class for reading and controlling the MLX90614 IR Thermometer.
#
# Author: Maurik Holtrop @ University of New Hampshire
#
# Sources:  https://www.mouser.com/datasheet/2/734/MLX90614-Datasheet-Melexis-953298.pdf
# I2C Bus access for Python, see: https://pypi.python.org/pypi/smbus2
# CRC code: See: http://www.sunshine2k.de/articles/coding/crc/understanding_crc.html
#

# This is a single function crc8 that computes the CRC-8 check
# for an 8-bit CRC with polynomial x8+x2+x+1
#
# See: http://www.sunshine2k.de/articles/coding/crc/understanding_crc.html
#
import sys
import time

# try:
#     import RPi.GPIO as GPIO
# except:
#     try:
#         import Adafruit_BBIO as GPIO
#     except:
#         print("Could not find a GPIO implementation")
#         raise("NO GPIO")

try:
    import smbus
except:
    print("No smbus found. Please install smbus")

class MLX90614(object):
    '''Class for reading and controlling the MLX90614 IR Thermometer.'''

    # EEPROM MAP:
    # Note the EEPROM command 0x20 is already added to the address.
    EEPROM_T0_max   = 0x20
    EEPROM_T0_min   = 0x21
    EEPROM_PWMCTRL  = 0x22
    EEPROM_Ta_range = 0x23
    EEPROM_Emmis_cor= 0x24
    EEPROM_Config   = 0x25
    EEPROM_SMB_addr = 0x2E
    EEPROM_ID_0     = 0x3C
    EEPROM_ID_1     = 0x3D
    EEPROM_ID_2     = 0x3E
    EEPROM_ID_3     = 0x3F

    # RAM MAP:
    DATA_RAW_ch1 = 0x04
    DATA_RAW_ch2 = 0x05
    DATA_Tamb         = 0x06
    DATA_Tobj1        = 0x07
    DATA_Tobj2        = 0x08

    GAIN_TABLE = [1,3,6,12.5,25,50,100]
    FIR_TABLE = [8,16,32,64,128,256,512,1024]
    IIR_TABLE = [50,25,17,13,100,80,67,57]


    class my_values(object):
        """Class for getting the value of the chip, which mimics a list."""
        def __init__(self,getter,MAX):
            self._getter = getter
            self._MAX = MAX
            self._n = 0

        def __getitem__(self,idx):
            return(self._getter(idx))

        def __setitem__(self,idx,val):
            raise ValueError("The ADC values cannot be written to, only read.")

        def __len__(self):
            return(self._MAX)

        def __iter__(self):
            self._n=0
            return(self)

        def next(self):
            return(self.__next__())

        def __next__(self):
            if self._n < len(self):
                result = self[self._n]
                self._n += 1
                return(result)
            else:
                raise StopIteration

        def __repr__(self):
            return(str(self))

        def __str__(self):
            tmplist = [ x for x in self ]
            return(str(tmplist))

    def __init__(self,bus=1,address=0x5a):
        '''Initialize MLX90614
        Parameters:
        ===========
        bus      - The SMBus to use. Default =1
        address  - The SMBus (I2C) address to use, default = 0x5a
        '''

        try:
            self._dev = smbus.SMBus(bus)
        except IOError:
            print("Error opening SMBus {}. Please make sure the Raspberry Pi is setup to read this bus.".format(bus))
            return(None)

        self._address=address            # Set by the hardware = 0b1101000
        self._temps = self.my_values(self.read_temps,3)


    def _danger_write_new_address(self,new_address):
        '''THIS IS A DANGEROUS THING TO DO. MAKE SURE NOTHING ELSE IS ON THE I2C BUS!!!!
        This function writes to the global address 0x0, so all devices on the bus
        will recieve these commands.
        In the first step, the exising address is erased to 0x0, in the second step,
        the new address is written. The device needs a reboot for the change to take place.
        Parameters:
        ===========
        new_address  - the new addres. Must be between 0x01 and 0x7F'''

        assert 0x01 <= new_address <= 0x7F

        try:
            dat = self._dev.read_i2c_block_data(0x00,0x2E,3)   # Read the old address.
            old_address = dat[0]
        except:
            print("Could not read successfully from 0x0. Address not changed.")
            return

        try:
            crc = self.crc8([0,self.EEPROM_SMB_addr,0,0])
            self._dev.write_i2c_block_data(0x00,0x2E,[0,0,crc])  # Erase address to 0x0
        except:
            print("Address erase write error, Address not changed.")
            print(sys.exc_info()[0])
            return

        time.sleep(0.1)
        dat = self._dev.read_i2c_block_data(0x00,0x2E,3)     # Read it to confirm.
        if dat[0]!= 0:
            print("Address erase failed, Address not changed.")
            return

        try:
            crc = self.crc8([0,self.EEPROM_SMB_addr,new_address,0])
            self._dev.write_i2c_block_data(0x00,0x2E,[new_address,0,crc])
        except:
            print("New Address write error, Address stuck at 0x0. Sorry!")
            print(sys.exc_info()[0])
            return

        time.sleep(0.1)
        dat = self._dev.read_i2c_block_data(0x00,0x2E,3)     # Read it to confirm.
        if dat[0]!= new_address:
            print("New Address write failed, Address stuck at 0x0. Sorry!")
            return

        print("Address successfully changed from 0x{:02x} to 0x{:02x}".format(old_address,new_address))
        print("Please reset (reboot) the device.")

    def _read_config(self):
        '''Read the config register and return it as int '''
        dat= self._dev.read_i2c_block_data(self._address,self.EEPROM_Config,3)
        return( dat[0] + (dat[1]<<8))

    def _write_config(self,conf):
        '''Write a new config into the config register.
        Parameters:
        ===========
        conf  -  New 16 bit value for the config register.'''

        # Frist we need to write 0x0000 to the register to erase it.
        dat=[0,0]
        dat.append(self.crc8([(self._address<<1),self.EEPROM_Config,dat[0],dat[1]]))
        self._dev.write_i2c_block_data(self._address,self.EEPROM_Config,dat)
        time.sleep(0.1)

        # Now write the new data.
        dat=[ (conf& 0xFF), (conf>>8)]
        dat.append(self.crc8([(self._address<<1),self.EEPROM_Config,dat[0],dat[1]]))
        self._dev.write_i2c_block_data(self._address,self.EEPROM_Config,dat)
        time.sleep(0.1)
        check=self._read_config()
        assert conf == check

    def enter_sleep_mode(self):
        '''Send the MLX90614 into sleep (low power) mode.
        A wakeup or a reboot has to be performed before it can be read again.
        Currently, the only wakeup I know that works is to power down the device.'''
        crc = self.crc8([self._address<<1,0xFF])
        self._dev.write_byte_data(self._address,0xFF,crc)

    def wake_up_from_sleep(self,rst,scl=3,sda=2):
        """Wake up from sleep mode. Only needed when actually sleeping.
        This requires the GPIO lines for the I2C to be controlled directly, which
        seems to lock them up after.
        Instead, use a GPIO pin 'rst' to reset the device on low."""
        import RPi.GPIO as GPIO
        GPIO.setmode(GPIO.BCM)
        # GPIO.setwarnings(False) # Since scl and sda are already in use :-(
        # GPIO.setup(scl,GPIO.OUT)
        # GPIO.setup(sda,GPIO.OUT)
        # Now toggle the GPIOs...... BUT, that permanently hangs the i2c bus.
        # DANG
        # GPIO.output(sda,1)
        # GPIO.output(scl,0)
        # GPIO.output(sda,1)
        # GPIO.output(scl,0)
        # time.sleep(0.034)
        # GPIO.output(sda,1)
        # GPIO.output(scl,0)
        # GPIO.output(sda,0)
        GPIO.setup(rst,GPIO.OUT)
        GPIO.output(rst,0)
        GPIO.output(rst,1)

    def get_id_number(self):
        """Get the device ID number as a list of 8 bytes. """
        id=[]
        for i in range(4):
            dat = self._dev.read_i2c_block_data(self._address,0x3C+i,3)
            id.append(dat[0])
            id.append(dat[1])
        return(id)

    def set_emissivity_correction(self,em):
        """Set the emissivity correction. See page 48 of data sheet.
        Parameters:
        ===========
        em   -  emissivity, value between 0 and 1, where 1 is default."""
        emcor = int(em*0xFFFF)
        dat=[ (emcor& 0xFF), (emcor>>8)]
        dat.append(self.crc8([(self._address<<1),self.EEPROM_Emmis_cor,dat[0],dat[1]]))
        self._dev.write_i2c_block_data(self._address,self.EEPROM_Emmis_cor,dat)

    def get_emissivity_correction(self):
        """Read the current emissivity correction from the device."""
        dat = self._dev.read_i2c_block_data(self._address,self.EEPROM_Emmis_cor,3)
        emcor = float(dat[0] + (dat[1]<<8))/float(0xFFFF)
        return(emcor)

    def set_gain(self,gain):
        """Set the internal amplifier gain. Default is 50.
        Parameters:
        ============
        gain   -  Amplifier gain. Must be one of [1,3,6,12.5,25,50,100]"""
        assert gain in self.GAIN_TABLE
        gain_code = self.GAIN_TABLE.index(gain)
        config = self._read_config()
        config = (config & 0xC7FF ) | (gain_code<<11)
        print("Set the config register to 0x{:04x} ".format(config))
        self._write_config(config)

    def get_gain(self):
        """Read the gain from the device """
        config = self._read_config()
        gain_code = (config & 0x3800)>>11
        gain = self.GAIN_TABLE[gain_code]
        return(gain)

    def set_FIR(self,fir):
        """Set the Finite Impulse Response (FIR). See page 16 of data sheet.
        and: https://www.melexis.com/en/documents/documentation/application-notes/application-note-mlx90614-onchip-digital-signal-filters
        Parameters:
        ============
        fir  - FIR setting, must be one of [8,16,32,64,128,256,512,1024]
        Values below 128 are not recommended, but do depend on the IIR setting."""

        assert fir in self.FIR_TABLE
        fir_code = self.FIR_TABLE.index(fir)
        config = self._read_config()
        config = (config & 0xF8FF ) | (fir_code<<8)
        self._write_config(config)

    def get_FIR(self):
        """Read the FIR from the device"""
        config = self._read_config()
        fir_code = (config & 0x0700 )>>8
        fir = self.FIR_TABLE[fir_code]
        return(fir)

    def set_IIR(self,iir):
        """Set the Infinite Impulse Response (IIR). See page 16 of data sheet.
        Parameters:
        ===========
        iir  -  IIR setting, but be one of [50,25,17,13,100,80,67,57] """
        assert iir in self.IIR_TABLE
        iir_code = self.IIR_TABLE.index(iir)
        config = self._read_config()
        config = (config & 0xFFF8 ) | (iir_code)
        self._write_config(config)

    def get_IIR(self):
        """Read the FIR from the device"""
        config = self._read_config()
        iir_code = (config & 0x0007 )
        iir = self.IIR_TABLE[iir_code]
        return(iir)


    def read_temps(self,ch):
        '''Read the temperature in C for channel ch
        Parameters:
        ===========
        ch  = channel to read, either:
        Ambient Temp = 0
        IR Temp 1    = 1
        IR Temp 2    = 2
        '''
        assert 0<= ch < 3
        dat_loc = ch+self.DATA_Tamb
        dat = self._dev.read_i2c_block_data(self._address,dat_loc,3)  # Read 3 bytes from device.
        temp = (dat[0] + (dat[1]<<8))/50. - 273.15

        crc = self.crc8([self._address<<1,dat_loc,(self._address<<1)+1,dat[0],dat[1]])
        if crc != dat[2]:
            print("CRC error: 0x{:02x} != 0x{:02x}".format(crc,dat[3]))

        return(temp)

    @property
    def temps(self):
        """Property that returns the three temperatures as a list of 3 float.
        Units are Centigrade"""
        return(self._temps)

    def crc8(self,dat,tail=0):
        '''Caculcate the CRC8 for x8+x2+x+1 (10000111).
        parameters:
        ============
        dat  - input data as integer to calculate crc on. Expected 8 bits.
        tail - usually zero, but can be a crc to check. If correct return 0.

        returns: CRC code.'''

        generator = (0b100000111) & 0xFF   # Yes, this is 0x07
        crc=0

        if isinstance(dat,int):
            dats=[]
            while dat:
                dats.insert(0,dat&0xFF)  # We need high bytes to low bytes: insert 8 bits at [0]
                dat >>= 8                # Shift to the next 8 bits.
        elif isinstance(dat,list):
            dats=dat
        else:
            print("I do not understand the input:",dat)
            return(0)

        for d in dats:
            crc = crc ^ d

            for i in range(8):
                if crc & 0x80:
                    crc = (((crc << 1)&0xFF) ^ generator)
                else:
                    crc = ((crc << 1)&0xFF)

        return(crc)
