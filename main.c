#include "em_chip.h"
#include "iadc_config.h"
#include "uart_comm.h"
#include <stdio.h>

int main(void)
{
  CHIP_Init();
  initIADC_PC09();
  uart_init();

  char buffer[64];  // bufor na sformatowany tekst

    while (1) {
      uint32_t mv = iadc_read_mv();  // odczyt ADC w mV

      // konwersja liczby do stringa
      sprintf(buffer, "ADC: %lu mV\r\n", mv);

      // wyślij string przez UART
      uart_send_string(buffer);

      // prosty delay
      for (volatile int i = 0; i < 500000; i++);
    }
 }
