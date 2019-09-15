#!/usr/bin/env python
#
# This driver reads the DS18B20 1-wire temperature sensor from Dallas Semi.
#
# The driver needs some kernel modules and overlays setup correctly.
# See: https://pinout.xyz/pinout/1_wire
#
# Wire device with pin-1 = GND, pin-2 = Data, pin-3 = Vcc =3.3V
# You need a pull-up resistor of 4.7k to Vcc.
#
#
import subprocess
import glob

class DS18B20:

    def __init__(self,pin=4):

        # Check if the modules are already installed.
        # result=subprocess.run(["lsmod"],stdout=subprocess.PIPE)
        # mods=result.stdout.decode()  # bytes object decoded to string
        mods=str(subprocess.check_output(["lsmod"]))
        if mods.find("w1_gpio")<0 :  # Assume if w1_gpio is there it is the right one.
            self.initialize_pin(pin)
        if mods.find("w1_therm")<0 :
            result=subprocess.check_output(["sudo","modprobe","w1_therm"])


    def initialize_pin(self,pin):
        '''Initializes a pin for 1-wire use. pin is one of the GPIO pins.
           This will call dtoverlay w1-gpio gpiopin=# pullup=0 '''
        result=subprocess.check_output(["sudo","dtoverlay","w1-gpio","gpiopin={}".format(pin),"pullup=0"])
        return(result)

    def discover_addresses(self):
        '''Look for possible DS18B20 sensors on the 1-wire bus and return a list of addresses.'''
        result=glob.glob("/sys/bus/w1/devices/28*")
        self._addr_list = [ l[20:] for l in result]
        return(self._addr_list)

    def read_raw(self,addr):
        '''Read the raw data from the sensor at addr '''
        sens = open("/sys/bus/w1/devices/"+str(addr)+"/w1_slave")
        rawdat = sens.read()
        sens.close()
        return(rawdat)

    def decode_raw(self,dat):
        '''Decode the raw data from a sensor and return the temperatur in C'''
        data=dat.split(" ")
        if data[11].split("\n")[0] != "YES":
            print("ERROR -- BAD DATA READ\n")
            return(-1000.)
        temp1000=int(data[20].split("=")[1])
        return(float(temp1000)/1000)

    def read_temp(self,addr):
        '''Read and decode the temperature from sensor at addr'''
        rawdat=self.read_raw(addr)
        return(self.decode_raw(rawdat))


def main():
    '''Quick test program '''
    ds = DS18B20()
    addrs=ds.discover_addresses()
    for ad in addrs:
        temp = ds.read_temp(ad)
        print("add {}  temp = {:8.4f}".format(ad,temp))

if __name__ == '__main__':
    import sys
    import time

    main()
