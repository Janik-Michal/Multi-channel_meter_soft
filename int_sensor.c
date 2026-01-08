#include "em_i2c.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include <stdint.h>

#define TMP1075_ADDR        0x48  // 7-bit address (0x90 >> 1)
#define TMP1075_REG_TEMP    0x00
#define i2c                 I2C0

void i2c_init(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_I2C0, true);

  // PB00 = SDA, PB01 = SCL
  GPIO_PinModeSet(gpioPortB, 0, gpioModeWiredAndPullUp, 1);
  GPIO_PinModeSet(gpioPortB, 1, gpioModeWiredAndPullUp, 1);

  I2C_Init_TypeDef init = I2C_INIT_DEFAULT;
  init.freq = I2C_FREQ_STANDARD_MAX;

  I2C_Init(i2c, &init);

  GPIO->I2CROUTE[0].SDAROUTE = (gpioPortB << _GPIO_I2C_SDAROUTE_PORT_SHIFT) |
                               (0 << _GPIO_I2C_SDAROUTE_PIN_SHIFT);
  GPIO->I2CROUTE[0].SCLROUTE = (gpioPortB << _GPIO_I2C_SCLROUTE_PORT_SHIFT) |
                               (1 << _GPIO_I2C_SCLROUTE_PIN_SHIFT);
  GPIO->I2CROUTE[0].ROUTEEN = GPIO_I2C_ROUTEEN_SDAPEN | GPIO_I2C_ROUTEEN_SCLPEN;
}
float tmp1075_read_temperature(void)
{
  uint8_t temp_reg = TMP1075_REG_TEMP;

  // Start + address + write
  I2C_TransferSeq_TypeDef seq;
  seq.addr  = TMP1075_ADDR << 1;
  seq.flags = I2C_FLAG_WRITE_READ;
  seq.buf[0].data = &temp_reg;
  seq.buf[0].len  = 1;

  static uint8_t temp_data[2];
  seq.buf[1].data = temp_data;
  seq.buf[1].len  = 2;

  I2C_TransferReturn_TypeDef result = I2C_TransferInit(i2c, &seq);

  while (result == i2cTransferInProgress)
  {
    result = I2C_Transfer(i2c);
  }

  if (result != i2cTransferDone)
    return -1000.0f;  // Error

  // Data temperature TMP1075 (16-bit)
  int16_t raw_temp = (temp_data[0] << 8) | temp_data[1];
  raw_temp >>= 4;  // TMP1075 – 12-bit

  float temp_celsius = raw_temp * 0.0625f;
  return temp_celsius;
}
