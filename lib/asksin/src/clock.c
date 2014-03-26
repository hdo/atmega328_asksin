#include <avr/io.h>
#include <avr/interrupt.h>

#include "board.h"
#include "led.h"
#include "fncollection.h"
#include "clock.h"
#include "display.h"
#include "rf_send.h"                   // credit_10ms


volatile uint32_t ticks;
volatile uint8_t  clock_hsec;

// count & compute in the interrupt, else long runnning tasks would block
// a "minute" task too long
ISR(TIMER0_COMPA_vect, ISR_BLOCK)
{

     // 125Hz
     ticks++; 
     if(++clock_hsec>=125)  
	  clock_hsec = 0;

}

void
Minute_Task(void)
{
  static uint8_t last_tick;
  if((uint8_t)ticks == last_tick)
    return;
  last_tick = (uint8_t)ticks;


  if(clock_hsec>0)     // Note: this can skip some hsecs
    return;

  // one second, 1% duty cycle, 10ms resolution => this is simple ;-)
  if (credit_10ms < MAX_CREDIT)
    credit_10ms += 1;

  static uint8_t clock_sec;
  clock_sec++;
  if(clock_sec != 60)                   // minute from now on
    return;
  clock_sec = 0;

}
