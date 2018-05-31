# Phys605 module
### Author: Maurik Holtrop

These modules are used in the Physics 605 lab at the University of New Hampshire.
The modules are drivers that allow you to easily communicate with a number of sensors and
other ICs. The drivers are named after the number on the chip.

The Modules should work with a Raspberry Pi using RPI.GPIO or with a Beagle Bone Black using
Adafruit_BBIO. Other required libraries are "smbus" and "smbus2" to communicate with the I2C
protocol and "spidev" to communicate with the SPI protocol. These libraries can generally be
installed with:
    pip install smbus, smbus2, spidev

## Modules:

1. AD9850  - Module for driving the AD9850 based frequency synthesizers.
1. BME280  - Module for reading out the BME280 or BMP280 Temperature, Pressure, Humidity sensor.
1. DS3231  - Module for reading out DS3231 based Real Time Clock modules.
1. ISL29125- Module for reading out the ISL29125 Red-Green-Blue light color sensor.
1. MAX7219 - Module for driving the MAX7219 based displays, either 8x8 dots, or 8 7-segment numbers.
1. MCP320x - Module for reading the MCP3208 (and related MCP3202/4) 12-bit ADCs using the GPIO or SPI pins.
1. MCP4251 - Module for driving an MCP4251 Digital Programmable Resistors. Works with MPC4241, MCP4242, MCP4251, MCP4252, MCP4261, MCP4262, MCP4141, MCP4142, MCP4151, MCP4152, MCP4161, MCP4162
1. SI7021  - Module for reading the SI7021 temperature and humidity sensor.
1. SN74HC165 - Simple module for reading out the SN74HC165, Parallel In to Serial Out shift register.
1. SSD1331 - Module to drive an SSD1331 OLED display module. 
