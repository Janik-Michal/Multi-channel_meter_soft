#include "read_temp.h"
#include "em_iadc.h"

extern volatile double scanResult[];

void iadc_convert_all_to_temperature(float *temperatures_C, int NUM_SCAN_CHANNELS)
{
  float V0 = 0.412f * 1000.0f;  // mV przy 0°C
  float V1 = 0.643f * 1000.0f;  // mV przy 25.68°C
  float T0 = 0.0f;
  float T1 = 25.68f;

  for (int i = 0; i < NUM_SCAN_CHANNELS; i++) {
    float v_mV = (float)scanResult[i];
    temperatures_C[i] = (v_mV - V0) * (T1 - T0) / (V1 - V0) + T0;
  }
}
