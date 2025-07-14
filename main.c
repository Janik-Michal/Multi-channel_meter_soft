#include "em_chip.h"
#include "iadc_config.h"
#include "uart_comm.h"
#include "modbus_rtu_slave.h"
#include "read_temp.h"
#include <stdio.h>

#define NUM_SCAN_CHANNELS 5

float temperatures_C[NUM_SCAN_CHANNELS];
extern volatile double scanResult[NUM_SCAN_CHANNELS];

int main(void)
{
    CHIP_Init();
    uart_init();       // USART0 + RS-485 (PA4 = DIR)
    initIADC();
    systick_init();// Inicjalizacja ADC

    char buffer[64];
    float voltages_mV[NUM_SCAN_CHANNELS];

    while (1) {
        iadc_convert_all_to_temperature(voltages_mV, temperatures_C, NUM_SCAN_CHANNELS);

        //for (int i = 0; i < NUM_SCAN_CHANNELS; i++) {
         //   snprintf(buffer, sizeof(buffer), "CH[%d]: %.2f °C\r\n", i, temperatures_C[i]);
         //   uart_send_string(buffer);
        //}

       // uart_send_string("------\r\n");

        modbus_poll(); // <-- obsługa Modbus RTU

        //for (volatile int i = 0; i < 100000; i++);
    }
}
