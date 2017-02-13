#!/usr/bin/env python
#
#
import sys
import time
import serial
import threading
import random as ran

class uart_test:

    def __init__(self):
        self.ser = serial.Serial("/dev/tty.usbserial-AH03FHRY",38400,rtscts=True)
        time.sleep(0.1)
        print("Started uart_test")
        self.alive = True
        self.receiver_thread = threading.Thread(target=self.reader, name='rx')
        print("started thread")
        self.receiver_thread.start()
        
    def reader(self):
        """Simple reader routine to spin in a thread"""
        sys.stdout.write("Reader reporting\n")
        sys.stdout.flush()
        while self.alive == True:
            if self.ser.in_waiting:
                data = self.ser.read(self.ser.in_waiting)
                if data:
                    sys.stdout.write(data)
                    sys.stdout.flush()
                else:
                    print(".")
                    sys.stdout.flush()


    def out(self,data):
        """ Output data on the serial """
        self.ser.write(data)

    def end(self):
        """Join with the thread."""
        utest.alive=False
        self.receiver_thread.join()
        print("Thread done")        

utest = uart_test()
utest.out("This is a test\n")
utest.out("Next line\n")
utest.out("We are OK\n")
for i in range(30):
    utest.out("Now on "+str(i)+" OK abcdefg\n")
    
for i in range(40):
    time.sleep(0.5)

utest.end()



