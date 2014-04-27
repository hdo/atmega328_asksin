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
#include "light_ws2812.h"

#define LED_COUNT 12


volatile extern uint32_t ticks;  // 1/125 sec resolution // see clock.h
extern MAX_RF_DATA max_data;

const PROGMEM t_fntab fntab[] = {

  { 'C', ccreg },
  { 'A', asksin_func },
  { 'Z', moritz_func },
  { 'V', version },

  { 0, 0 },
};

struct cRGB led[LED_COUNT];

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t count;
} RF_RGB_PAYLOAD;

void set_ring(uint8_t red, uint8_t green, uint8_t blue, uint8_t count, uint8_t clear) {
	for(int i=0; i < LED_COUNT; i++) {
		if (clear) {
			led[i].r=0;
			led[i].g=0;
			led[i].b=0;
		}
		if (i < count) {
			led[i].r = red;
			led[i].g = green;
			led[i].b = blue;
		}
	}
	ws2812_setleds(led, LED_COUNT);
}



int
main(void)
{


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

  sei();

    /* self test rgb ring */

	my_delay_ms(100);
	for (int i = 0; i < LED_COUNT; i++) {
		set_ring(250,0,0,i,0);
		my_delay_ms(25);
	}
	for (int i = 0; i < LED_COUNT; i++) {
		set_ring(0,255,0,i,0);
		my_delay_ms(25);
	}
	for (int i = 0; i < LED_COUNT; i++) {
		set_ring(0,0,255,i,0);
		my_delay_ms(25);
	}
	for (int i = 0; i < LED_COUNT; i++) {
		set_ring(0,0,0,i,0);
		my_delay_ms(25);
	}

  my_delay_ms(100);

  /* start moritz function */
  moritz_func("Za081519\n");
  my_delay_ms(100);

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

        if (max_data.message_type == 0x75) {
        	RF_RGB_PAYLOAD *payload = (RF_RGB_PAYLOAD*) &(max_data.payload);

            DS("red: ");
            DU(payload->red, 2);
            DNL();
            DS("green: ");
            DU(payload->green, 2);
            DNL();
            DS("blue: ");
            DU(payload->blue, 2);
            DNL();
            DS("count: ");
            DU(payload->count, 2);
            DNL();
            set_ring(payload->red, payload->green, payload->blue, payload->count, 1);
        }

    }
  }

}
