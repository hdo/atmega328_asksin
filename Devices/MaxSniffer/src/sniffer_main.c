/* Copyright Huy Do, 2014.
 * huydo1@gmail.com
 * License: Released under the GPL Licence, Version 2
 *
 * Work is based on culfw: http://culfw.de/culfw.html
 */

#include <avr/power.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

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
#include "rf_send.h"
#include "ttydata.h"
#include "rf_asksin.h"
#include "rf_moritz.h"



volatile extern uint32_t ticks;  // 1/125 sec resolution // see clock.h
extern MAX_RF_DATA max_data;

const PROGMEM t_fntab fntab[] = {

  { 'C', ccreg },
  { 'A', asksin_func },
  { 'Z', moritz_func },
  { 'V', version },

  { 0, 0 },
};

int
main(void)
{

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

  uart_init( UART_BAUD_SELECT_DOUBLE_SPEED(UART_BAUD_RATE,F_CPU) );

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

  /* start moritz function */
  moritz_func("Zr\n");
  for(;;) {
	led_process(ticks);

    uart_task();
    Minute_Task();
    rf_asksin_task();
    rf_moritz_task();
    if (rf_moritz_data_available()) {
        DC('Z');
        uint8_t *rf_data = (uint8_t*) &max_data;
        for (uint8_t i=0; i<=*rf_data; i++) {
        	DH2( *rf_data++ );
        }
        DNL();
        DS("length: ");
        DU(max_data.length, 2);
        DNL();
        DS("msg count: ");
        DU(max_data.message_count, 2);
        DNL();
        DS("msg type: ");
        DU(max_data.message_type, 2);
        DNL();
    }
  }

}
