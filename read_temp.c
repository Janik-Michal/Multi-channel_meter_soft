#include "read_temp.h"
#include "em_iadc.h"

float iadc_read_temperature_celsius(void)
{
  IADC_command(IADC0, iadcCmdStartSingle);

  while (!(IADC0->STATUS & _IADC_STATUS_SINGLEFIFODV_MASK));

  IADC_Result_t result = IADC_pullSingleFifoResult(IADC0);
  uint16_t raw_value = result.data;

  // ADC → napięcie (Vout)
  float v_out = ((float)raw_value * 1.25f) / 4095.0f;

  float V0 = 0.412f;     // napięcie przy 0°C
  float V1 = 0.643f;     // napięcie przy 25.68°C
  float T0 = 0.0f;
  float T1 = 25.68f;

  float temp = (v_out - V0) * (T1 - T0) / (V1 - V0) + T0;
  return temp;
}
