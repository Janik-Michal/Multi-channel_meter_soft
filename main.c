#include "em_chip.h"
#include "iadc_config.h"
#include "uart_comm.h"
#include <stdio.h>

int main(void)
{
  CHIP_Init();
  initIADC_PC09();  // Używasz PC08 zgodnie z wcześniejszą rozmową
  uart_init();

  char buffer[64];

  while (1) {
    // Sformatuj z dwoma miejscami po przecinku
    sprintf(buffer, "Temp: %.2f C\r\n", iadc_read_temperature_celsius());

    // Wyślij przez UART
    uart_send_string(buffer);
    for (volatile int i = 0; i < 1000000; i++);
  }
}


