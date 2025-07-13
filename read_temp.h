#ifndef READ_TEMP_H
#define READ_TEMP_H

void iadc_convert_all_to_temperature(const float *voltages_mV, float *temperatures_C, int numChannels);

#endif // READ_TEMP_H
