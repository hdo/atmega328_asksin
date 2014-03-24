/* Copyright Rudolf Koenig, 2008.
   Released under the GPL Licence, Version 2
   Inpired by the MyUSB USBtoSerial demo, Copyright (C) Dean Camera, 2008.
*/

#include <avr/boot.h>
#include <avr/power.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include <string.h>

#include "board.h"

#include "spi.h"
#include "cc1100.h"
#include "clock.h"
#include "delay.h"
#include "display.h"
#include "serial.h"
#include "fncollection.h"
#include "led.h"
#include "ringbuffer.h"
#include "rf_receive.h"
#include "rf_send.h"
#include "ttydata.h"

#ifdef HAS_ASKSIN
#include "rf_asksin.h"
#endif

#ifdef HAS_MORITZ
#include "rf_moritz.h"
#endif



volatile extern uint32_t ticks;  // 1/125 sec resolution // see clock.h

const PROGMEM t_fntab fntab[] = {

  { 'C', ccreg },
  { 'F', fs20send },
#ifdef HAS_ASKSIN
  { 'A', asksin_func },
#endif
#ifdef HAS_MORITZ
  { 'Z', moritz_func },
#endif
  { 'V', version },
  { 'X', set_txreport },

  { 'l', ledfunc },
  { 't', gettime },

  { 0, 0 },
};

int
main(void)
{
  wdt_disable();

  led_init();

#ifdef LED_RGB
  led_off(LED_CHANNEL_GREEN);
  led_off(LED_CHANNEL_RED);
  led_off(LED_CHANNEL_BLUE);
#else
  LED_ON();
#endif


  spi_init();


  OCR0A  = 249;                            // Timer0: 0.008s = 8MHz/256/250 == 125Hz
  TCCR0B = _BV(CS02);
  TCCR0A = _BV(WGM01);
  TIMSK0 = _BV(OCIE0A);

  TCCR1A = 0;
  TCCR1B = _BV(CS11) | _BV(WGM12);         // Timer1: 1us = 8MHz/8

  clock_prescale_set(clock_div_1);

  MCUSR &= ~(1 << WDRF);                   // Enable the watchdog
  wdt_enable(WDTO_2S);

  uart_init( UART_BAUD_SELECT_DOUBLE_SPEED(UART_BAUD_RATE,F_CPU) );

  tx_init();
  input_handle_func = analyze_ttydata;

  display_channel = DISPLAY_USB;

#ifdef LED_RGB
  my_delay_ms(200);
  led_on(LED_CHANNEL_RED);
  my_delay_ms(200);
  led_off(LED_CHANNEL_RED);
  led_on(LED_CHANNEL_GREEN);
  my_delay_ms(200);
  led_off(LED_CHANNEL_GREEN);
  led_on(LED_CHANNEL_BLUE);
  my_delay_ms(200);
  led_off(LED_CHANNEL_BLUE);
#else
  LED_OFF();
#endif

  sei();

  for(;;) {
	led_process(ticks);

    uart_task();
    RfAnalyze_Task();
    Minute_Task();
#ifdef HAS_ASKSIN
    rf_asksin_task();
#endif
#ifdef HAS_MORITZ
    rf_moritz_task();
#endif
  }

}
