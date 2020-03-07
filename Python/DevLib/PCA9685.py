#!/usr/bin/env python
#
# Class for reading and controlling the MLX90614 IR Thermometer.
#
# Author: Maurik Holtrop @ University of New Hampshire
#
# Manufacturer NXP:
# https://www.nxp.com/products/power-management/lighting-driver-and-controller-ics/
# ic-led-controllers/16-channel-12-bit-pwm-fm-plus-ic-bus-led-controller:PCA9685
# Data Sheet:  https://www.nxp.com/docs/en/data-sheet/PCA9685.pdf
#
# Other info: Exploring Raspberry Pi, chapter 9, page 381.
# Adafruit: https://www.adafruit.com/product/815
# Info on SG-92 Servo motors: http://www.ee.ic.ac.uk/pcheung/teaching/DE1_EE/stores/sg90_datasheet.pdf
#
# Basic Operation:
#
# Set the frequency you want to run the PWM at. Default is 200 Hz, servos like 50 Hz.
# For each channel, you set 2 12-bit values, the time until "on" and the time until "off". These values are
# compared against a continuously running 12-bit counter.
#
import time

try:
    import smbus
except ImportError:
    smbus = None
    print("No smbus found. Please install smbus")


class PCA9685(object):
    """Class for controlling the PCA9685 chip, a 16 channel, 12-bit PWM controller."""

    _Osc_Calibration = 27346212.75

    class MyValues:
        """Class for getting the value of the chip, which mimics a list."""
        def __init__(self, getter, setter, maxlen):
            self._getter = getter
            self._setter = setter
            self._maxlen = maxlen
            self._n = 0

        def __getitem__(self, idx):
            return self._getter(idx)

        def __setitem__(self, idx, val):
            self._setter(idx, val)

        def __len__(self):
            return self._maxlen

        def __iter__(self):
            self._n = 0
            return self

        def __next__(self):
            if self._n < len(self):
                result = self[self._n]
                self._n += 1
                return result
            else:
                raise StopIteration

        def __repr__(self):
            return str(self)

        def __str__(self):
            tmplist = [x for x in self]
            return str(tmplist)

    def __init__(self, bus=1, addr=0x40):
        """Initialize the PCA9685 chip.
        Parameters:
        ===========
        bus      - The SMBus to use. Default =1
        address  - The SMBus (I2C) address to use, default = 0x5a
        """

        try:
            self._dev = smbus.SMBus(bus)
        except IOError:
            print("Error opening SMBus {}. Please make sure the Raspberry Pi is setup to read this bus.".format(bus))

        self._addr = addr
        self._frequency = self.frequency_read  # Default frequency is 200 Hz.

        # These behave like properties that are list-like. So PCA9685.phase[0]=0.2 works.
        self.phase = self.MyValues(self.get_phase, self.set_phase, 16)
        self.duty = self.MyValues(self.get_duty, self.set_duty, 16)
        self.on_time_ms = self.MyValues(self.get_on_time_ms, self.set_on_time_ms, 16)
        self.off_time_ms = self.MyValues(self.get_off_time_ms, self.set_off_time_ms, 16)

    def get_mode1_reg(self):
        """Read the content of the mode1 register."""
        return self._dev.read_byte_data(self._addr, 0x0)

    def set_mode1_reg(self, val):
        """Set the content of mode1 register to val.
        Mode1 bits, * indicates default:
        7 = restart write 1. Read: 1 = ready for restart.
        6 = EXTCLK, 0 = internal clock (*), 1 = external clock
        5 = Auto Increment, 0 = disabled(*), 1 = enabled.
        4 = SLEEP,  0 = normal mode, 1 = low power sleep mode.
        3-1 = SUBADDR - 0 = do not respond (*)
        0 = ALLCALL - 0 = do not respond to ALL LED, 1 = do respond (*)"""
        assert 0 <= val < 256
        self._dev.write_byte_data(self._addr, 0x0, int(val))

    def sleep(self):
        """Set device to enter sleep mode = all outputs off = 0"""
        m1 = self.get_mode1_reg()
        m1 = m1 | 0x10  # Set bit4
        self.set_mode1_reg(m1)

    def wake(self):
        """Wakeup from sleep and resume operation."""
        # See 7.3.1.1 in data sheet.
        m1 = self.get_mode1_reg()
        if not (m1 & 0x80):            # Hold until bit 7 is 1.
            m1 = self.get_mode1_reg()
        m1 = m1 & ~0x10                # Reset the sleep bit
        self.set_mode1_reg(m1)
        time.sleep(0.001)              # Wait 1ms
        m1 = m1 | 0x80                 # Set the reset bit.
        self.set_mode1_reg(m1)

    @property
    def is_sleeping(self):
        """Returns True if device is sleeping."""
        return self.get_mode1_reg() & 0x10 == 0x10

    def get_freq_reg(self):
        """Read the raw 8-bit frequency register from the device."""
        return self._dev.read_byte_data(self._addr, 0xFE)

    def set_freq_reg(self, val):
        """Set the raw 8-bit frequency register on the device."""
        assert 0x03 <= val <= 0xFF
        if not self.is_sleeping:
            wake = True
            self.sleep()  # You can only change frequency in sleep mode.
        else:
            wake = False
        self._dev.write_byte_data(self._addr, 0xFE, val)  # Set new reg value.

        if wake:
            self.wake()   # Wake up gain.

    @property
    def frequency_read(self):
        """Read the currently set frequency from the device."""
        freq_reg = self.get_freq_reg()
        self._frequency = self._Osc_Calibration / ((freq_reg + 1) * 4096)
        return self._frequency

    @property
    def frequency(self):
        return self._frequency

    @frequency.setter
    def frequency(self, val):
        """Set a new frequency for internal oscillator."""
        assert 23.847680097680097 <= val <= 1526.2515262515262
        self._frequency = val
        self.set_freq_reg(int(round(self._Osc_Calibration/(val*4096))) - 1)

    def get_on_pos(self, i):
        """Get the raw 'on' position value for channel i"""
        assert 0 <= i <= 15
        loc = 0x06 + 4*int(i)
        lo = self._dev.read_byte_data(self._addr, loc)
        hi = self._dev.read_byte_data(self._addr, loc+1)
        return (hi << 8)+lo

    def set_on_pos(self, i, val):
        """Set the 'on' position for channel i to val"""
        assert 0 <= i <= 15
        assert 0 <= val < 4096
        loc = 0x06 + 4*int(i)
        lo = int(val) & 0xFF
        hi = int(val) >> 8
        self._dev.write_byte_data(self._addr, loc, lo)
        self._dev.write_byte_data(self._addr, loc+1, hi)

    def get_on_time_ms(self, i):
        """Return the 'on' position in milli-seconds"""
        pos = self.get_on_pos(i)
        return 1000*pos/4095/self._frequency

    def set_on_time_ms(self, i, ms):
        """Set the 'on' position in milli-seconds"""
        self.set_on_pos(i, self._frequency*4095*ms/1000)

    def get_off_time_ms(self, i):
        """Return the 'on' position in milli-seconds"""
        pos = self.get_off_pos(i)
        return 1000 * pos / 4095 / self._frequency

    def set_off_time_ms(self, i, ms):
        """Set the 'on' position in milli-seconds"""
        self.set_off_pos(i, self._frequency * 4095 * ms / 1000)

    def get_phase(self, i):
        """Get the phase for channel i as a percentage."""
        pos = self.get_on_pos(i)
        phase = 100.*pos/4096
        return phase

    def set_phase(self, i, phase):
        """Set the phase for channel i, where phase is in percentage.
        This keeps the duty % the same."""
        old_on_pos = self.get_on_pos(i)
        old_off_pos = self.get_off_pos(i)
        new_on_pos = int(phase*4096/100)
        new_off_pos = old_off_pos + (new_on_pos - old_on_pos)
        if new_off_pos > 4095:
            new_off_pos -= 4096
        self.set_on_pos(i, new_on_pos)
        self.set_off_pos(i, new_off_pos)

    def get_off_pos(self, i):
        """Get the raw 'off' position value for channel i."""
        assert 0 <= i <= 15
        loc = 0x08 + 4*int(i)
        lo = self._dev.read_byte_data(self._addr, loc)
        hi = self._dev.read_byte_data(self._addr, loc+1)
        return (hi << 8)+lo

    def set_off_pos(self, i, val):
        """Set the raw 'off' position value for channel i."""
        assert 0 <= i <= 15
        assert 0 <= val < 4096
        loc = 0x08 + 4*int(i)
        lo = int(val) & 0xFF
        hi = int(val) >> 8
        self._dev.write_byte_data(self._addr, loc, lo)
        self._dev.write_byte_data(self._addr, loc+1, hi)

    def get_duty(self, i):
        """Get the duty factor for channel i as a percentage."""
        assert 0 <= i <= 15
        on_pos = self.get_on_pos(i)
        off_pos = self.get_off_pos(i)
        if off_pos > on_pos:
            duty = 100.*(off_pos - on_pos)/4096
        else:
            duty = 100.*(4096+off_pos - on_pos)/4096

        return duty

    def set_duty(self, i, val):
        """Set the duty factor for channel i to val as a percentage."""
        assert 0 <= i <= 15
        assert 0. <= val <= 100.
        on_pos = self.get_on_pos(i)
        off_pos = int(0.5+val*4096./100) + on_pos
        if off_pos > 4096:
            off_pos = off_pos - 4096
        self.set_off_pos(i, off_pos)

    def __getitem__(self, item):
        """Allows use of list like addressing. Returns the duty %"""
        return self.get_duty(item)

    def __setitem__(self, key, value):
        """Allows setting duty as %"""
        self.set_duty(key, value)
