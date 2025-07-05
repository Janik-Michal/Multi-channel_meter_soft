#include "em_usart.h"
#include "em_gpio.h"
#include "em_cmu.h"

#define UART USART0

void uart_init(void)
{
  // Włącz zegary dla GPIO i USART0
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_USART0, true);

  // Konfiguracja pinów:
  // PA6 jako TX (wysyła dane do FTDI RXD)
  // PA5 jako RX (odbiera dane z FTDI TXD)
  // PA4 jako DIR (jeśli używasz np. RS485 – tu zostawione jako TX enable)

  GPIO_PinModeSet(gpioPortA, 6, gpioModePushPull, 1);  // TX (PA6)
  GPIO_PinModeSet(gpioPortA, 5, gpioModeInput, 0);     // RX (PA5)
  GPIO_PinModeSet(gpioPortA, 4, gpioModePushPull, 1);  // DIR - domyślnie TX enable

  // Konfiguracja USART w trybie asynchronicznym
  USART_InitAsync_TypeDef init = USART_INITASYNC_DEFAULT;
  init.baudrate = 115200;

  USART_InitAsync(UART, &init);

  // Routing: PA6 (TX), PA5 (RX)
  GPIO->USARTROUTE[0].TXROUTE = (gpioPortA << _GPIO_USART_TXROUTE_PORT_SHIFT) |
                                (6 << _GPIO_USART_TXROUTE_PIN_SHIFT);  // PA6 = TX
  GPIO->USARTROUTE[0].RXROUTE = (gpioPortA << _GPIO_USART_RXROUTE_PORT_SHIFT) |
                                (5 << _GPIO_USART_RXROUTE_PIN_SHIFT);  // PA5 = RX

  GPIO->USARTROUTE[0].ROUTEEN = GPIO_USART_ROUTEEN_RXPEN | GPIO_USART_ROUTEEN_TXPEN;
}

void uart_send_byte(uint8_t data)
{
  USART_Tx(UART, data);
}

uint8_t uart_receive_byte(void)
{
  while (!(UART->STATUS & USART_STATUS_RXDATAV))
    ;
  return USART_Rx(UART);
}

void uart_send_string(const char* str)
{
  while (*str)
    uart_send_byte((uint8_t)*str++);
}
