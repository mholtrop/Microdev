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
 * The chip, when running uses about 2.1 mA by itself. With 2 RED LEDS over 100 Ohm at lowest power, the current is 2.1 mA.
 * So running at lowest power is pretty inefficient from a purist point of view. 
 * At max power with 2 LEDS the current is 30.5 mA.
 * 
 * When off, the chip goes into sleep mode. I tried to get this to be as low power as I could, but it is still ~15 uA on the ATMege168,
 * However for the ATMega328p it is 1.1 uA with the brownout detection off. 
 * This blog https://blog.duk.io/sleeping-atmega328-on-1ua-with-timer-wakeup/ seems to get it much lower, but doesn't give the details.
 * 
 * Fuse settings:
 *   avrdude -p m168 -P usb -c avrispmkII -v -v -U lfuse:w:0x62:m -U hfuse:w:0xdf:m -U efuse:w:0xf9:m
 *   avrdude -p m328p -P usb -c avrispmkII -v -v -U lfuse:w:0x62:m -U hfuse:w:0xd9:m -U efuse:w:0xff:m
 * 
 */

// #define SCOPE_MONITOR 1
#define XSTR(x) STR(x)
#define STR(x) #x

// One loop is measured to take about 32.07 ms, so there are 31.18 loops per second, 1871 loops per minute.
#define TURN_OFF_TIME 60*1871UL        // Turn off after one hour. 
#define TURN_OFF_DIM_TIME 5*1871UL   // 5 minute grace period with dimmed light.

#define LEDS_ON_PB (_BV(PB1) | _BV(PB2))
#define LEDS_OFF_PB (~LEDS_ON_PB)
#define BUTTON_PORT DDB0
#define BUTTON_PIN  PB0

#ifndef F_CPU
#define 	F_CPU   1000000UL  // CPU runs at 1 MHz
#endif
#define __DELAY_BACKWARD_COMPATIBLE__
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>  
#include <avr/wdt.h>
#include <util/delay.h>

volatile uint8_t power_level = 0;

void restart_blink(void){
  // Three bright blinks when system restarts.
  PORTB |= 0b00000110;; // LEDs ON
  _delay_loop_2(10000);
  PORTB &= ~0b00000110;; // LEDs OFF
  _delay_loop_2(10000);
  PORTB |= 0b00000010;; // LED ON
  _delay_loop_2(10000);
  PORTB &= ~0b00000110;; // LEDs OFF
  PORTB |=  0b00000100;; // LED ON
  _delay_loop_2(10000);
  PORTB &= ~0b00000110;; // LEDs OFF
}

ISR (PCINT0_vect)   // Interrupt for button push to wake up chip.
{
  if(bit_is_clear(PINB, PB0)){  // The button is down.
    if( power_level == 0) power_level = 1;   // So we don't go right back to sleep.

// #ifdef SCOPE_MONITOR
//       PORTD = (( (power_level & 0b00000111) << 5) & 0b11100000) | (PORTD & 0b00011111);  
// #endif
  }
}

void WDT_off(void){
  // This turns off the watchdog timer, which will save power in sleep mode.
  wdt_reset();
  /* Clear WDRF in MCUSR */
  MCUSR &= ~(1<<WDRF);
  /* Write logical one to WDCE and WDE */
  /* Keep old prescaler setting to prevent unintentional time-out */
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  /* Turn off WDT */
  WDTCSR = 0x00;
  wdt_disable();
}

int main (void) {
  
  unsigned long loop_time = 0;

  #define N_POWER_LEVELS 12
  uint16_t power_levels[N_POWER_LEVELS] = {0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
  uint8_t  power_level_up_down = 1;
  uint16_t duty_max = 1024;
  uint16_t duty_factor = 0;

  unsigned char button_state = 0;
  unsigned char last_button_state = 0;
  unsigned int button_on_time = 0;
  uint16_t button_off_time = 0;


  // Setup the output pins. 
  DDRB = LEDS_ON_PB;    // LEDS on PB1, PB2, button on PB0
  PORTB = 0xFF;   // ON or PULL-UP
  DDRB &= ~ _BV(BUTTON_PORT); // Set PB0 as input
  PORTB |= _BV(BUTTON_PIN);  // Set PB0 as pullup 

  //DDRC = 0x0; // All PORTC pins are inputs.
  //PORTC = 0x0; // no pullups.

#ifdef SCOPE_MONITOR
  DDRD = 0b11100000; // For scope monitoring of the code.
  PORTD = 0x0; // OFF
#endif
 
  PCICR |= _BV(PCIE0);    // set PCIE0 to enable PCMSK0 scan
  PCMSK0 |= _BV(PCINT0);  // set PCINT0 to trigger an interrupt on state change

  // DDRD &= ~(1 << DDD2); // Set PD2 as input
  // PORTD |= (1 << PD2);  // Set PD2 as pullup
  // EICRA = (EICRA & ~(_BV(ISC00) | _BV(ISC01))) | (_BV(ISC00));  // set INT0 to trigger on ISCx = 00 (low level), 01 (any change), 10 (falling edge), 11 (rising edge)
  // EIMSK |= _BV(INT0);   // Turns on INT0

  // set_sleep_mode(SLEEP_MODE_PWR_DOWN); // choose power down mode when sleep is triggered.
  SMCR = ( SMCR & ~(_BV(SM0) | _BV(SM2))) | _BV(SM1); // Set the sleep mode to power down mode SMx = 010.
  SMCR |= _BV(SE); // Enable sleep mode.

  // Turn off the ADC Comparator
  ACSR |= _BV(ACD);

  // Turn off the Analog Comparator
  ADCSRA &= ~_BV(ADEN);

  // Turn off the Watchdog Timer
  WDT_off();

  restart_blink();

  sei(); // Global enable interrupts
  
  while (1) {

#ifdef SCOPE_MONITOR
    PORTD |= _BV(PD7); // PD7 On
#endif
    if( power_level == 0){
      // sleep_cpu(); // Go to sleep.
      duty_factor = 0;
      loop_time = 0;
      PORTB &= LEDS_OFF_PB; // LEDs OFF
#ifdef SCOPE_MONITOR
      PORTD &= ~0b11100000;   // Monitor PORTD OFF
#endif
      __asm__ __volatile__ ( "sleep" "\n\t" :: );
    }

    // Debounce the button on PB0, which has a pullup resistor turned on and is connected to ground.
    if(bit_is_set(PINB, PB0)) {  // Button is not pressed.
      button_off_time++;
      if(button_off_time >= UINT16_MAX - 1) button_off_time = UINT16_MAX - 1;
      button_on_time = 0;
      if(button_off_time > 1){
        button_state = 0;
        last_button_state = 0;
      }
    } else {
      button_on_time++;
      if( button_on_time > 1){
        button_state = 1;
        if(button_state != last_button_state){
          if(button_off_time < 40){
            if(power_level_up_down){ 
              power_level++;
              if(power_level >= N_POWER_LEVELS){
                power_level = N_POWER_LEVELS - 1;
                power_level_up_down = 0;
              }
            }else{
              power_level--;
              if(power_level == 0){
                power_level_up_down = 1;
              }
            }
          }else{
            power_level = 0;
            duty_factor = 0;
          }
          last_button_state = button_state;
        }
        button_off_time = 0;
      }
    }

#ifdef SCOPE_MONITOR
    PORTD &= ~_BV(PD7); // PD7 Off
#endif

    duty_factor = power_levels[power_level];
    for (unsigned char i = 0; i < 8; i++){

#ifdef SCOPE_MONITOR
        PORTD |= _BV(PD5);
#endif

        // If 0, just stay off.
        if(duty_factor  > 0){
           PORTB |= LEDS_ON_PB; // LEDs ON
           _delay_loop_2(duty_factor);
        }
#ifdef SCOPE_MONITOR
        PORTD &= ~_BV(PD5);
        PORTD |= _BV(PD6);
#endif
        PORTB &= LEDS_OFF_PB; // LEDs OFF
        // No off if at duty_max.
        if(duty_factor < duty_max){
          _delay_loop_2(duty_max - duty_factor);
        }
#ifdef SCOPE_MONITOR
        PORTD &= ~_BV(PD6);
#endif
    }
    loop_time++;
    if(loop_time == TURN_OFF_TIME){
      power_level = power_level / 2;
    }
    if(loop_time > (TURN_OFF_TIME + TURN_OFF_DIM_TIME) ){
      power_level = 0;
      loop_time = 0;
    }
  } 
}
