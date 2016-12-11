################################################
#
# Makefile Template for Arduino Libraries
# Author: Maurik Holtrop
# Copyleft.
#
# Version 2
#
################################################

# Type of board, for the pin layout library.
# You find the possible board definitions in ../include/variants
#
BOARD_TYPE=atmega328p

# The type of the chip as the avr-gcc compiler expects it.
# You can find the types of chips supported with "avr-gcc -dumpspecs | grep mmcu="
#
# Examples: atmega168p  atmega328p  atmega168 
MCU=atmega328p

# Frequency of CPU.
# Without a crystal, this is 1 MHz standard, or 8 MHz with a fuse setting.
# With a crystal, it is either 16 MHz or 20 MHz.
#F_CPU=16000000
F_CPU=20000000
#F_CPU=1000000

#
# The system type controls where the library is installed. 
# This is useful if you are working with different chips.
# Make sure that your program is linked with the correct library for your chip.
#
SYSTEM_TYPE=$(MCU)_$(F_CPU)
# Alternate, specify by hand:
#SYSTEM_TYPE=atmega168_20MHz

# id to use with programmer
# default: PROGRAMMER_MCU=$(MCU)
# In case the programer used, e.g avrdude, doesn't
# accept the same MCU name as avr-gcc (for example
# for ATmega8s, avr-gcc expects 'atmega8' and 
# avrdude requires 'm8')
# Newer avrdude does better with that!
#
PROGRAMMER_MCU=$(MCU)

#
# Name of the programmer to use. This is defined in the
# file: /usr/local/etc/avrdude.conf
# Or in  ~/.avrduderc
# For the Raspberry pi, we defined this in the latter file
# to use linuxgpio with specific pins.
AVRDUDE_PROGRAMMERID=RaspberryPi

#
# If you use the Avrisp mkII programmer
#
#AVRDUDE_PROGRAMMERID=avrisp2
#AVRDUDE_PORT=usb


# port--serial or parallel port to which your 
# hardware programmer is attached
#
#AVRDUDE_PORT=/dev/tty.usbserial-A9005cJ0
#AVRDUDE_PORT=/dev/tty.usbserial-A6008hex
#AVRDUDE_EXTRA_FLAGS= -F -b57600 -V
#


