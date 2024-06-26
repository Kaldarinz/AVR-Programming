#define __AVR_ATmega328P__


// ------- Preamble -------- //
#include <avr/io.h>
#include "pinDefines.h"

#include <util/delay.h>
#define DEBOUNCE_TIME  1000                            /* microseconds */

uint8_t debounce(void) {
  if (bit_is_clear(BUTTON_PIN, BUTTON)) {      /* button is pressed now */
    _delay_us(DEBOUNCE_TIME);
    if (bit_is_clear(BUTTON_PIN, BUTTON)) {            /* still pressed */
      return (1);
    }
  }
  return 0;
}

int main(void) {
  // -------- Inits --------- //
  uint8_t buttonWasPressed=0;                                 /* state */
  BUTTON_PORT |= (1 << BUTTON);     /* enable the pullup on the button */
  LED_DDR = (1 << LED0);                      /* set up LED for output */

  // ------ Event loop ------ //
  while (1) {
    if (debounce()) {                        /* debounced button press */
      if (buttonWasPressed == 0) {     /* but wasn't last time through */
        LED_PORT ^= (1 << LED0);                        /* do whatever */
        buttonWasPressed = 1;                      /* update the state */
      }
    }
    else {                                /* button is not pressed now */
      buttonWasPressed = 0;                        /* update the state */
    }

  }                                                  /* End event loop */
  return 0;                            /* This line is never reached */
}
