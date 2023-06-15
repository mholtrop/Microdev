/*
 * Red Light 
 *
 *  Simple code to control some RED LEDs for Minka to read by at night.
 *
 *  Created on: June 15, 2023
 *      Author: Maurik
 * 
 * Notes:
 * If the LED goes from a PORT to a 100 Ohm resistor to ground, the current at 3.5V VCC is about 12mA. At 50% duty cycle, the LED 
 * is then fairly bright. Total current into the system with setup is 10mA, so we loose about 1/2 the power. Yeah, whatever.
 * 
 * 
 */

#define BAUD 9600

#define 	F_CPU   1000000UL  // CPU runs at 1 MHz
#define __DELAY_BACKWARD_COMPATIBLE__
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// void delay_us(double __us){
//   double __tmp ;
//   uint8_t __ticks;
// 	double __tmp2 ;
// 	__tmp = ((F_CPU) / 3e6) * __us;
// 	__tmp2 = ((F_CPU) / 4e6) * __us;
// 	if (__tmp < 1.0)
// 		__ticks = 1;
// 	else if (__tmp2 > 65535)
// 	{
// 		_delay_ms(__us / 1000.0);
// 	}
// 	else if (__tmp > 255)
// 	{
// 		uint16_t __ticks=(uint16_t)__tmp2;
// 		_delay_loop_2(__ticks);
// 		return;
// 	}
// 	else
// 		__ticks = (uint8_t)__tmp;
// 	_delay_loop_1(__ticks);
// }



int main (void) {

  DDRD |= _BV(7);
  DDRB |= _BV(1);
  
  sei();

  // Set interrupts enabled.
  DDRB = 0b000111110;    // LEDS on PB1, PB2, PB3, PB4, PB5
  PORTB = 0b000111110;   // ON
  DDRD = 0b11100000; // 
  PORTD = 0x00; // OFF

  DDRB &= ~(1 << DDB0); // Set PB0 as input
  PORTB |= (1 << PB0);  // Set PB0 as pullup 

  uint16_t duty_max = 1024;
  uint16_t duty_factor = 0;

  unsigned int loop_time = 0;
  unsigned char button_state = 0;
  unsigned char last_button_state = 0;
  unsigned int button_on_time = 0;
  unsigned int button_off_time = 0;
  unsigned int button_presses = 0;

  _delay_loop_2(10);

  while (1) {

    // Debounce the button on PB0, which has a pullup resistor turned on and is connected to ground.

    if (!bit_is_clear(PINB, PB0)) {  // Button is pressed.
      // PORTD |= (1<< PD7);
      button_off_time++;
      button_on_time = 0;
      if(button_off_time > 1){
        button_state = 0;
        last_button_state = 0;
      }
    } else {
      // PORTD &= ~(1<< PD7);
      button_on_time++;
      if( button_on_time > 1){
        button_state = 1;
        if(button_state != last_button_state){
          if(button_off_time < 40){
            duty_factor += 1 + duty_factor;  // Sequence is 1, 3, 7, 15, 31, 63, 127, 255, 511, 1023
            if(duty_factor > duty_max) duty_factor = 0;
            button_presses++;
          }else{
            duty_factor = 0;
            button_presses = 0;
          }
          PORTD = (button_presses << 5) & 0b11100000;
          last_button_state = button_state;
        }
        button_off_time = 0;
      }
    }

    for (unsigned char i = 0; i < 8; i++){
        PORTB &= ~0b000111110;; // LEDs OFF
        if(duty_factor < duty_max){
          _delay_loop_2(duty_max - duty_factor);
        }
        // If 0 or 1, just stay off.
        if(duty_factor  > 0){
           PORTB |= 0b000111110;; // LEDs ON
           _delay_loop_2(duty_factor);
        }        
       
    }
  loop_time++; 
  } 
}
