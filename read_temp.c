#include "read_temp.h"
#include "em_iadc.h"

extern volatile double scanResult[];
#define PT1000_LUT_SIZE 26

typedef struct {
    float temperature_C;
    float resistance_ohm;
    float voltage_mV;
} PT1000_LUT_Entry;

          // --- LUT PT1000 --- //

PT1000_LUT_Entry pt1000_lut[PT1000_LUT_SIZE] = {
    { -40.0f, 842.70f, 45.32f },
    { -35.0f, 862.50f, 92.07f },
    { -30.0f, 882.20f, 138.48f },
    { -25.0f, 901.90f, 184.78f },
    { -20.0f, 921.60f, 230.95f },
    { -15.0f, 941.30f, 277.05f },
    { -10.0f, 960.90f, 322.8f },
    {  -5.0f, 980.50f, 367.38f },
    {   0.0f, 1000.0f, 413.72f },
    {   5.0f, 1019.5f, 458.93f },
    {  10.0f, 1039.0f, 504.0f },
    {  15.0f, 1058.5f, 549.0f },
    {  20.0f, 1077.9f, 593.7f },
    {  25.0f, 1097.3f, 638.23f },
    {  30.0f, 1116.7f, 682.7f },
    {  35.0f, 1136.0f, 726.81f },
    {  40.0f, 1155.4f, 771.1f },
    {  45.0f, 1174.7f, 815.01f },
    {  50.0f, 1194.0f, 858.8f },
    {  55.0f, 1213.2f, 902.34f },
    {  60.0f, 1232.4f, 945.8f },
    {  65.0f, 1251.6f, 989.06f },
    {  70.0f, 1270.8f, 1032.4f },
    {  75.0f, 1290.0f, 1075.4f },
    {  80.0f, 1309.0f, 1118.0f },
    {  85.0f, 1328.0f, 1160.47f }
};

      // --- interpolation temperature --- //

float pt1000_get_temperature_from_voltage(float v_mV) {
    for (int i = 0; i < PT1000_LUT_SIZE - 1; i++) {
        float v1 = pt1000_lut[i].voltage_mV;
        float v2 = pt1000_lut[i + 1].voltage_mV;

        if (v_mV >= v1 && v_mV <= v2) {
            float t1 = pt1000_lut[i].temperature_C;
            float t2 = pt1000_lut[i + 1].temperature_C;

            float t = t1 + (v_mV - v1) * (t2 - t1) / (v2 - v1);
            return t;
        }
    }
    return -999.0f;
}

void iadc_convert_all_to_temperature(float *temperatures_C, int NUM_SCAN_CHANNELS)
{
  for (int i = 0; i < NUM_SCAN_CHANNELS; i++) {
    float v_mV = (float)scanResult[i];
    temperatures_C[i] = pt1000_get_temperature_from_voltage(v_mV);
  }
}


