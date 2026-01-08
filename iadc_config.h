#ifndef IADC_CONFIG_H
#define IADC_CONFIG_H

#define NUM_INPUTS 10

extern volatile double scanResult[NUM_INPUTS];
void initIADC(void);

#endif
