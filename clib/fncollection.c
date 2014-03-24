#include <avr/eeprom.h>
#include <avr/interrupt.h>

#include "board.h"
#include "display.h"
#include "delay.h"
#include "fncollection.h"
#include "cc1100.h"
#include "../version.h"
#include "clock.h"

uint8_t led_mode = 2;   // Start blinking

// LED
void
ledfunc(char *in)
{
  fromhex(in+1, &led_mode, 1);
  if(led_mode & 1)
    LED_ON();
  else
    LED_OFF();
}


void
version(char *in)
{
#if defined(CUL_HW_REVISION)
  if (in[1] == 'H') {
    DS_P( PSTR(CUL_HW_REVISION) );
    DNL();
    return;
  }
#endif

#ifdef MULTI_FREQ_DEVICE     // check 433MHz version marker
  if (!bit_is_set(MARK433_PIN, MARK433_BIT))
    DS_P( PSTR("V " VERSION " " BOARD_ID_STR433) );
  else
#endif
  DS_P( PSTR("V " VERSION " " BOARD_ID_STR) );
  DNL();
}
