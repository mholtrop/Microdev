//
//  MySerial.hpp
//  LEDBall
//
//  Created by Maurik Holtrop on 11/25/16.
//  Copyright © 2016 Holtrop Coding. All rights reserved.
//
//  This class implements a dedicated interrupt driven serial interface using the UART0
//  on the atmega168/328 chip.
//  It borrows a lot from the Arduino HardwareSerial. It differs in that it handles the
//  incoming strings up to a newline, then hands off the command in that string to appropriate handler.
//
#ifndef MySerial
#define MySerial

#include <stdio.h>
#include <inttypes.h>
#include "HardwareSerial.h"

class MySerial  {
  
public:
  MySerial(void){};
  
};


#endif /* MySerial_hpp */
