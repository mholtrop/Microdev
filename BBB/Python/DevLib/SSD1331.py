# SSD1331 OLED Display Driver
#
# Author: Maurik Holtrop
#
# This code is loosely based on what I learned from the AdaFruit Driver: https://github.com/adafruit/Adafruit-SSD1331-OLED-Driver-Library-for-Arduino
# and works with the AdaFruit OLED Breakout Board - 16-bit Color 1.5"  : https://www.adafruit.com/product/684
#
# Datasheet:
import time
from spidev import SpiDev
import RPi.GPIO as GPIO

class SSD1331:

    # SSD1331 Commands
    _CMD_DRAWLINE     = 0x21
    _CMD_DRAWRECT     = 0x22
    _CMD_FILL         = 0x26
    _CMD_SETCOLUMN    = 0x15
    _CMD_SETROW       = 0x75
    _CMD_CONTRASTA    = 0x81
    _CMD_CONTRASTB    = 0x82
    _CMD_CONTRASTC    = 0x83
    _CMD_MASTERCURRENT= 0x87
    _CMD_SETREMAP     = 0xA0
    _CMD_STARTLINE    = 0xA1
    _CMD_DISPLAYOFFSET= 0xA2
    _CMD_NORMALDISPLAY= 0xA4
    _CMD_DISPLAYALLON = 0xA5
    _CMD_DISPLAYALLOFF= 0xA6
    _CMD_INVERTDISPLAY= 0xA7
    _CMD_SETMULTIPLEX = 0xA8
    _CMD_SETMASTER    = 0xAD
    _CMD_DISPLAYOFF   = 0xAE
    _CMD_DISPLAYON    = 0xAF
    _CMD_POWERMODE    = 0xB0
    _CMD_PRECHARGE    = 0xB1
    _CMD_CLOCKDIV     = 0xB3
    _CMD_PRECHARGEA   = 0x8A
    _CMD_PRECHARGEB   = 0x8B
    _CMD_PRECHARGEC   = 0x8C
    _CMD_PRECHARGELEVEL=0xBB
    _CMD_VCOMH        = 0xBE

    _WIDTH = 96
    _HEIGHT= 64


    def __init__(self,RSpin,DSpin,SPIcs=0,SPIclk=10000000,SPImosi=0):
        '''Initialize the class.
            RSpin  = Reset GPIO pin      , connected to RS
            DSpin  = Data Select GPIO pin, connected to DS
            SPIsc  = SPI Chip select     , connected to CS, default=0
            SPIclk = SPI Clock           ,                  default=1000000
            SPImosi= SPI Mosi            ,                  default=0
        Currently the class only implements hardware SPI    '''

        self._RS = RSpin
        self._DS = DSpin
        self._SPIcs = SPIcs
        self._SPIclk= SPIclk

        self._spi = SpiDev(0,SPIcs)
        self._spi.mode = 3
        self._spi.max_speed_hz=SPIclk

        GPIO.setmode(GPIO.BCM)
        GPIO.setup(self._RS,GPIO.OUT)
        GPIO.setup(self._DS,GPIO.OUT)

        self.ReInitialize()

    def WriteCommand(self,c):
        '''Write command 'c' over the SPI.'''

        GPIO.output(self._DS,0)  # Data Select low selects commands.

        if type(c) is int:
            self._spi.writebytes([c])
        else:
            self._spi.writebytes(c)

    def WriteData(self,c):
        '''Write command 'c' over the SPI.'''

        GPIO.output(self._DS,1)  # Data Select low selects commands.

        if type(c) is int:
            self._spi.writebytes([c])
        else:
            for i in range(len(c)//4096):     # Break the array into 4096 bytes max.
                self._spi.writebytes(c[i*4096:(i+1)*4096])

    def Reset(self):
        '''Reset the chip'''
        GPIO.output(self._RS,1)
        time.sleep(0.05)
        GPIO.output(self._RS,0)
        time.sleep(0.05)
        GPIO.output(self._RS,1)
        time.sleep(0.05)

    def ReInitialize(self):
        '''Reinitialize the chip that drives the display.'''

        self.Reset()
        # Initialization Sequence
        commands=[
            self._CMD_DISPLAYOFF, # 0xAE
            self._CMD_SETREMAP  , # 0xA0
            0x72,   # Bit0 = 0 = Horizontal address increment.
                    # Bit1 = 1 = RAM column 0 - 95 maps to Pin Seg 95-0
                    # Bit2 = 0 = RGB order.
                    # Bit3 = 0 = Disable left-right swapping on COM
                    # Bit4 = 1 = Scan from COM[N-1] to COM0
                    # Bit5 = 1 = Enable COM split IDD Even
                    # Bit7,6 = 0,1 = 65k color format.
        ]
        self.WriteCommand(commands)
        # RGB data order
        self.WriteCommand(0x72)
        self.WriteCommand(self._CMD_STARTLINE) 	# 0xA1
        self.WriteCommand(0x0)
        self.WriteCommand(self._CMD_DISPLAYOFFSET) 	# 0xA2
        self.WriteCommand(0x0)
        self.WriteCommand(self._CMD_NORMALDISPLAY)  	# 0xA4
        self.WriteCommand(self._CMD_SETMULTIPLEX) 	# 0xA8
        self.WriteCommand(0x3F)  			# 0x3F 1/64 duty
        self.WriteCommand(self._CMD_SETMASTER)  	# 0xAD
        self.WriteCommand(0x8E)
        self.WriteCommand(self._CMD_POWERMODE)  	# 0xB0
        self.WriteCommand(0x0B)
        self.WriteCommand(self._CMD_PRECHARGE)  	# 0xB1
        self.WriteCommand(0x31)
        self.WriteCommand(self._CMD_CLOCKDIV)  	# 0xB3
        self.WriteCommand(0xF0)  # 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
        self.WriteCommand(self._CMD_PRECHARGEA)  	# 0x8A
        self.WriteCommand(0x64)
        self.WriteCommand(self._CMD_PRECHARGEB)  	# 0x8B
        self.WriteCommand(0x78)
        self.WriteCommand(self._CMD_PRECHARGEA)  	# 0x8C
        self.WriteCommand(0x64)
        self.WriteCommand(self._CMD_PRECHARGELEVEL)  	# 0xBB
        self.WriteCommand(0x3A)
        self.WriteCommand(self._CMD_VCOMH)  		# 0xBE
        self.WriteCommand(0x3E)
        self.WriteCommand(self._CMD_MASTERCURRENT)  	# 0x87
        self.WriteCommand(0x06)
        self.WriteCommand(self._CMD_CONTRASTA)  	# 0x81
        self.WriteCommand(0x91)
        self.WriteCommand(self._CMD_CONTRASTB)  	# 0x82
        self.WriteCommand(0x50)
        self.WriteCommand(self._CMD_CONTRASTC)  	# 0x83
        self.WriteCommand(0x7D)
        self.WriteCommand(self._CMD_DISPLAYON)	#--turn on oled panel


    def GoTo(self,xy0,xy1=None):
        '''Move the cursor to xy0=[x,y] with end of writing at xy1'''
        if xy1 is None:
            xy1=[self._WIDTH-1,self._HEIGHT-1]

        if xy0[0] >= self._WIDTH or xy0[1] >= self._HEIGHT:
            return

        # set x and y coordinate
        self.WriteCommand(self._CMD_SETCOLUMN);
        self.WriteCommand(xy0[0]);
        self.WriteCommand(xy1[0]);

        self.WriteCommand(self._CMD_SETROW);
        self.WriteCommand(xy0[1]);
        self.WriteCommand(xy1[1]);

    def DrawPixel(self,xy,color):
        '''Color Pixel at xy=[x,y] with color'''
        self.GoTo(xy[0], xy[1]);
        self.WriteData([color >> 8,color&0x0F]);

    def DrawLine(self,xy0,xy1,color):
        '''Draw a line from xy0 ([x,y]) to xy1 in color'''
        if xy0[0]<0 or xy0[0]>self._WIDTH or xy0[1]<0 or xy0[1]>self._HEIGHT:
            return(-1)
        if xy1[0]<0 or xy1[0]>self._WIDTH or xy1[1]<0 or xy1[1]>self._HEIGHT:
            return(-2)
        self.WriteCommand([self._CMD_DRAWLINE]+xy0+xy1+[color])

    def DrawBox(self,xy0,xy1,color,fill):
        '''Draw a Box with corners xy0 ([x,y]) to xy1 in color with fill color.
        If fill is None, the fill will be turned off.'''
        if xy0[0]<0 or xy0[0]>self._WIDTH or xy0[1]<0 or xy0[1]>self._HEIGHT:
            return(-1)
        if xy1[0]<0 or xy1[0]>self._WIDTH or xy1[1]<0 or xy1[1]>self._HEIGHT:
            return(-2)
        self.WriteCommand(self._CMD_FILL)
        if fill is None:
            self.WriteCommand(0)
        else:
            self.WriteCommand(1)
        self.WriteCommand([self._CMD_DRAWRECT]+xy0+xy1+self.MakeColorCBA(color)+self.MakeColorCBA(fill))

    def DrawImage(self,image,xy0=None,xy1=None):
        '''Draw the image at xy. Usually xy=[0,0] (default
        :param image: The image to render
        :type image: PIL.Image.Image '''
        if xy0 is None:
            xy0=[0,0]
        if xy1 is None:
            xy1=[self._WIDTH-1,self._HEIGHT-1]

        buf = [0]*(xy1[0]-xy0[0]+1)*(xy1[1]-xy0[1]+1)*2
        i=0
        for pixel in image.getdata():
            buf[i]   = (pixel[0] & 0xF8) | (pixel[1] >> 5)
            buf[i+1] = (pixel[1] << 5 )  | (pixel[2] >> 3 )
            i += 2
        self.GoTo(xy0,xy1)
        self.WriteData(buf)

    def MakeColor(self,R,G=None,B=None):
        '''Create a 16-bit color from [R,G,B] intput'''
        if type(R) is tuple or type(R) is list:
            C=R
        else:
            C=[R,G,B]
        color= ((C[0]&0xF8)<<8) | ((C[1]&0xFC)<<3) | ((C[2]&0xF8)>>3)
        return(color)

    def Make2Color(self,C):
        '''Create a 2 byte color '''
        color= self.MakeColor(C)
        return([color>>8,color&0xFF])

    def MakeColorCBA(self,color):
        '''Internally used to decode color to CBA and send out.'''
        if color is None:
            return([0,0,0])

        if type(color) is tuple or type(color) is list:
            C = color[0]>>3
            B = color[1]>>2
            A = color[2]>>3
        else: # Color is 16-bit 565 format
            C = color>>11 & 0x1F
            B = color>>5  & 0x3F
            A = color     & 0x1F
        return([C,B,A])
