#include <avr/eeprom.h>
#include <avr/interrupt.h>

#include "board.h"
#include "display.h"
#include "delay.h"
#include "fncollection.h"
#include "cc1100.h"
#include "version.h"
#include "clock.h"

void
version(char *in)
{
  DS_P( PSTR("V " VERSION " " BOARD_ID_STR) );
  DNL();
}
