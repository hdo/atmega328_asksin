#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "delay.h"
#include "display.h"
#include "fncollection.h"
#include "cc1100.h"
#include "rf_moritz.h"

#define CC1100_CFG_SIZE   0x29                               // 41

uint8_t cc_on;

// NOTE: FS20 devices can receive/decode signals sent with PA ramping,
// but the CC1101 cannot
const PROGMEM const uint8_t CC1100_PA[] = {

  0x00,0x03,0x0F,0x1E,0x26,0x27,0x00,0x00,      //-10 dBm with PA ramping
  0x00,0x03,0x0F,0x1E,0x25,0x36,0x38,0x67,      // -5 dBm
  0x00,0x03,0x0F,0x25,0x67,0x40,0x60,0x50,      //  0 dBm
  0x00,0x03,0x0F,0x27,0x51,0x88,0x83,0x81,      //  5 dBm
  0x00,0x03,0x0F,0x27,0x50,0xC8,0xC3,0xC2,      // 10 dBm

  0x00,0x27,0x00,0x00,0x00,0x00,0x00,0x00,      //-10 dBm no PA ramping
  0x00,0x67,0x00,0x00,0x00,0x00,0x00,0x00,      // -5 dBm
  0x00,0x50,0x00,0x00,0x00,0x00,0x00,0x00,      //  0 dBm
  0x00,0x81,0x00,0x00,0x00,0x00,0x00,0x00,      //  5 dBm
  0x00,0xC2,0x00,0x00,0x00,0x00,0x00,0x00,      // 10 dBm

};


const PROGMEM const uint8_t CC1100_CFG[CC1100_CFG_SIZE] = {
// CULFW   IDX NAME     RESET STUDIO COMMENT
   0x0D, // 00 IOCFG2   *29   *0B    GDO2 as serial output
   0x2E, // 01 IOCFG1    2E    2E    Tri-State
   0x2D, // 02 IOCFG0   *3F   *0C    GDO0 for input
   0x07, // 03 FIFOTHR   07   *47    
   0xD3, // 04 SYNC1     D3    D3    
   0x91, // 05 SYNC0     91    91    
   0x3D, // 06 PKTLEN   *FF    3D    
   0x04, // 07 PKTCTRL1  04    04    
   0x32, // 08 PKTCTRL0 *45    32    
   0x00, // 09 ADDR      00    00    
   0x00, // 0A CHANNR    00    00    
   0x06, // 0B FSCTRL1  *0F    06    152kHz IF Frquency
   0x00, // 0C FSCTRL0   00    00    
   0x21, // 0D FREQ2    *1E    21    868.3 (def:800MHz)
   0x65, // 0E FREQ1    *C4    65    
   0x6a, // 0F FREQ0    *EC    e8    
   0x55, // 10 MDMCFG4  *8C    55    bWidth 325kHz
   0xe4, // 11 MDMCFG3  *22   *43    Drate:1500 ((256+228)*2^5)*26000000/2^28
   0x30, // 12 MDMCFG2  *02   *B0    Modulation: ASK
   0x23, // 13 MDMCFG1  *22    23    
   0xb9, // 14 MDMCFG0  *F8    b9    ChannelSpace: 350kHz
   0x00, // 15 DEVIATN  *47    00    
   0x07, // 16 MCSM2     07    07    
   0x00, // 17 MCSM1     30    30    
   0x18, // 18 MCSM0    *04    18    Calibration: RX/TX->IDLE
   0x14, // 19 FOCCFG   *36    14    
   0x6C, // 1A BSCFG     6C    6C    
   0x07, // 1B AGCCTRL2 *03   *03    42 dB instead of 33dB
   0x00, // 1C AGCCTRL1 *40   *40    
   0x90, // 1D AGCCTRL0 *91   *92    4dB decision boundery
   0x87, // 1E WOREVT1   87    87    
   0x6B, // 1F WOREVT0   6B    6B    
   0xF8, // 20 WORCTRL   F8    F8    
   0x56, // 21 FREND1    56    56    
   0x11, // 22 FREND0   *16    17    0x11 for no PA ramping
   0xE9, // 23 FSCAL3   *A9    E9    
   0x2A, // 24 FSCAL2   *0A    2A
   0x00, // 25 FSCAL1    20    00
   0x1F, // 26 FSCAL0    0D    1F    
   0x41, // 27 RCCTRL1   41    41    
   0x00, // 28 RCCTRL0   00    00    

 /*                                   
 Conf1: SmartRF Studio:               
   Xtal: 26Mhz, RF out: 0dB, PA ramping, Dev:5kHz, Data:1kHz, Modul: ASK/OOK,
   RF: 868.30MHz, Chan:350kHz, RX Filter: 325kHz
   SimpleRX: Async, SimpleTX: Async+Unmodulated
 */
};


uint8_t
cc1100_sendbyte(uint8_t data)
{
  SPDR = data;		        // send byte
  while (!(SPSR & _BV (SPIF)));	// wait until transfer finished
  return SPDR;
}


void
ccInitChip()
{
  moritz_on = 0; //loading this configuration overwrites moritz cfg

  EIMSK &= ~_BV(CC1100_INT);                 
  SET_BIT( CC1100_CS_DDR, CC1100_CS_PIN ); // CS as output

  CC1100_DEASSERT;                           // Toggle chip select signal
  my_delay_us(30);
  CC1100_ASSERT;
  my_delay_us(30);
  CC1100_DEASSERT;
  my_delay_us(45);

  ccStrobe( CC1100_SRES );                   // Send SRES command
  my_delay_us(100);

  CC1100_ASSERT;                             // load configuration
  cc1100_sendbyte( 0 | CC1100_WRITE_BURST );

  for(uint8_t i = 0; i < sizeof(CC1100_CFG); i++) {
	  cc1100_sendbyte(__LPM(CC1100_CFG+i));
  }

  CC1100_DEASSERT;

  CC1100_ASSERT;                             // setup PA table
  cc1100_sendbyte( CC1100_PATABLE | CC1100_WRITE_BURST );

  uint8_t idx = 8;

  const uint8_t *f = CC1100_PA+idx*8;
  for (uint8_t i = 0; i < 8; i++) {
	  cc1100_sendbyte(__LPM(f+i));
  }

  CC1100_DEASSERT;

  ccStrobe( CC1100_SCAL );
  my_delay_ms(1);
}



//--------------------------------------------------------------------
void
ccTX(void)
{
  uint8_t cnt = 0xff;
  EIMSK  &= ~_BV(CC1100_INT);

  // Going from RX to TX does not work if there was a reception less than 0.5
  // sec ago. Due to CCA? Using IDLE helps to shorten this period(?)
  ccStrobe(CC1100_SIDLE);
  while(cnt-- && (ccStrobe( CC1100_STX ) & 0x70) != 2)
    my_delay_us(10);
}

//--------------------------------------------------------------------
void
ccRX(void)
{
  uint8_t cnt = 0xff;

  while(cnt-- && (ccStrobe( CC1100_SRX ) & 0x70) != 1)
    my_delay_us(10);
  EIMSK |= _BV(CC1100_INT);

}


//--------------------------------------------------------------------
void
ccreg(char *in)
{
  uint8_t hb, out;

  if(fromhex(in+1, &hb, 1)) {

    if(hb == 0x99) {
      for(uint8_t i = 0; i < 0x30; i++) {
        DH2(cc1100_readReg(i));
        if((i&7) == 7)
          DNL();
      }
    } else {
      out = cc1100_readReg(hb);
      DC('C');                    // prefix
      DH2(hb);                    // register number
      DS_P( PSTR(" = ") );
      DH2(out);                  // result, hex
      DS_P( PSTR(" / ") );
      DU(out,2);                  // result, decimal
      DNL();
    }

  }
}

//--------------------------------------------------------------------
uint8_t
cc1100_readReg(uint8_t addr)
{
  CC1100_ASSERT;
  cc1100_sendbyte( addr|CC1100_READ_BURST );
  uint8_t ret = cc1100_sendbyte( 0 );
  CC1100_DEASSERT;
  return ret;
}

void
cc1100_writeReg(uint8_t addr, uint8_t data)
{
  CC1100_ASSERT;
  cc1100_sendbyte( addr|CC1100_WRITE_BURST );
  cc1100_sendbyte( data );
  CC1100_DEASSERT;
}


//--------------------------------------------------------------------
uint8_t
ccStrobe(uint8_t strobe)
{
  CC1100_ASSERT;
  uint8_t ret = cc1100_sendbyte( strobe );
  CC1100_DEASSERT;
  return ret;
}

void
set_ccoff(void)
{
#ifdef BUSWARE_CUR
  uint8_t cnt = 0xff;
  while(cnt-- && (ccStrobe( CC1100_SIDLE ) & 0x70) != 0)
    my_delay_us(10);
  ccStrobe(CC1100_SPWD);
#else
  ccStrobe(CC1100_SIDLE);
#endif
  cc_on = 0;
}

void
set_ccon(void)
{
  ccInitChip();
  cc_on = 1;
}
