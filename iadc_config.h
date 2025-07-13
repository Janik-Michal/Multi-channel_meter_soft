#ifndef IADC_CONFIG_H
#define IADC_CONFIG_H

#define NUM_INPUTS 5  // zgodnie z Twoim plikiem conf.c

// Deklaracja globalnej tablicy z wynikami skanowania
extern volatile double scanResult[NUM_INPUTS];

// Deklaracja funkcji inicjalizacji IADC
void initIADC(void);

#endif // IADC_CONFIG_H
