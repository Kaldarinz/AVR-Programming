#define __AVR_ATmega328P__
#define __AVR_ATmega32U4__

#include <avr/io.h>
#include <avr/delay.h>

#define DELAY     100

#define LED_PORT    PORTB
#define LED_PIN     PINB
#define LED_DDR     DDRB

int main(void) {
  LED_DDR = 0xff;
  uint8_t i = 0;

  while (1)
  {
    while(i < 7)
      {
        LED_PORT |= (1 << i);
        i++;
        _delay_ms(DELAY);
      }
      LED_PORT |= (1 << i);
      _delay_ms(DELAY);
    while (i > 0)
      {
        LED_PORT >>= 1;
        i--;
        _delay_ms(DELAY);
      }
      LED_PORT >>= 1;
      _delay_ms(DELAY);
  }

  
  return 0;
}