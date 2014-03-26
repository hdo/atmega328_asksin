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
#include "math_utils.h"



volatile extern uint32_t ticks;  // 1/125 sec resolution // see clock.h

const PROGMEM t_fntab fntab[] = {

  { 'C', ccreg },
  { 'A', asksin_func },
  { 'Z', moritz_func },
  { 'V', version },

  { 0, 0 },
};


/* ADC initialisieren */
void ADC_Init(void) {

  // die Versorgungsspannung AVcc als Refernz wählen:
  ADMUX = (1<<REFS0);
  // oder interne Referenzspannung als Referenz für den ADC wählen:
  // ADMUX = (1<<REFS1) | (1<<REFS0);

  // Bit ADFR ("free running") in ADCSRA steht beim Einschalten
  // schon auf 0, also single conversion
  ADCSRA = (1<<ADPS1) | (1<<ADPS0);     // Frequenzvorteiler
  ADCSRA |= (1<<ADEN);                  // ADC aktivieren

  /* nach Aktivieren des ADC wird ein "Dummy-Readout" empfohlen, man liest
     also einen Wert und verwirft diesen, um den ADC "warmlaufen zu lassen" */

  ADCSRA |= (1<<ADSC);                  // eine ADC-Wandlung
  while (ADCSRA & (1<<ADSC) ) {         // auf Abschluss der Konvertierung warten
  }
  /* ADCW muss einmal gelesen werden, sonst wird Ergebnis der nächsten
     Wandlung nicht übernommen. */
  (void) ADCW;
}

/* ADC Einzelmessung */
uint16_t ADC_Read( uint8_t channel )
{
  // Kanal waehlen, ohne andere Bits zu beeinflußen
  ADMUX = (ADMUX & ~(0x1F)) | (channel & 0x1F);
  ADCSRA |= (1<<ADSC);            // eine Wandlung "single conversion"
  while (ADCSRA & (1<<ADSC) ) {   // auf Abschluss der Konvertierung warten
  }
  return ADCW;                    // ADC auslesen und zurückgeben
}

/* ADC Mehrfachmessung mit Mittelwertbbildung */
/* beachte: Wertebereich der Summenvariablen */
uint16_t ADC_Read_Avg( uint8_t channel, uint8_t nsamples )
{
  uint32_t sum = 0;

  for (uint8_t i = 0; i < nsamples; ++i ) {
    sum += ADC_Read( channel );
  }

  return (uint16_t)( sum / nsamples );
}



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

  uint32_t last_adc_ticks = 0;

  ADC_Init();

  for(;;) {
	led_process(ticks);

    uart_task();
    Minute_Task();
    rf_asksin_task();
    rf_moritz_task();

    // each tick = 8ms
    // perform adc every 2 seconds
    if (math_calc_diff(ticks, last_adc_ticks) > 250) {
    	last_adc_ticks = ticks;
    	DS("perform adc ...");
    	uint16_t adcval = ADC_Read_Avg(2, 4);  // Kanal 2, Mittelwert aus 4 Messungen // mach was mit adcval
    	DU(adcval, 5);
    	DS("\r\n");
    }
  }

}
