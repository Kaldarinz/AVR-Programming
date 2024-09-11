#ifndef __AVR_ATmega328P__
#define __AVR_ATmega328P__
#endif

// ------- Preamble -------- //
#include <avr/io.h>
#include <util/delay.h>
#include <avr/power.h>

#include "USART.h"
#include "i2c_isr.h"

// -------- Defines -------- //

#define BMP085_ADDRESS          0b01110111
#define BMP085_TEMP_MSB         0xF6
#define BMP085_TEMP_LSB         0xF7
#define BMP085_CTRL_REG         0xF4
#define BMP085_TEMP_STRT        0x2E

#define TWSR_READ_MASK 0b11111000

#define FREQ_100  100000
// -------- Functions --------- //
int main(void) {

  uint16_t ut;
  uint8_t data[2];
  uint8_t com_ind = 0;
  uint8_t cur_stat = 0;
  // -------- Inits --------- //
  clock_prescale_set(clock_div_1);                             /* 8MHz */
  initUSART();
  printString("\r\n====  i2c Thermometer  ====\r\n");
  i2c_init(FREQ_100);
  //printByte(i2c_status);
  uint8_t temp_start[2] = {BMP085_CTRL_REG, BMP085_TEMP_STRT};
  uint8_t temp_req[1] = {BMP085_TEMP_MSB};
  // ------ Event loop ------ //
  while (1) {
    cur_stat = i2c_run();
/*     if (cur_stat)
    {
      printString("Status code: ");
      printHexByte(cur_stat);
      printString("\n");
    } */
    if ( (i2c_status!=I2C_BUSY)&&(com_ind==0) )
    {
      i2c_master_send(BMP085_ADDRESS, temp_start, 2);
      com_ind = 1;
    }
    else if ( (i2c_status!=I2C_BUSY)&&(com_ind==1) )
    {
      i2c_master_send(BMP085_ADDRESS, temp_req, 1);
      com_ind = 2;
    }
    else if ( (i2c_status!=I2C_BUSY)&&(com_ind==2) )
    {
      i2c_master_read(BMP085_ADDRESS, data, 2);
      com_ind = 3;
    }
    else if ( (i2c_status!=I2C_BUSY)&&(com_ind==3) )
    {
      ut = data[0]<<8;
      ut += data[1];
      printString("Temperature = ");
      // printWord(ut);
      printWord(ut);
      printString("\r\n");
      /* Once per second */
      _delay_ms(1000);
      com_ind = 0;
    }
  }                                                  /* End event loop */
  return 0;                            /* This line is never reached */
}
