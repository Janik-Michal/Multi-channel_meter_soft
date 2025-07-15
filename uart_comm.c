#include "em_usart.h"
#include "em_gpio.h"
#include "em_cmu.h"
#include "modbus_rtu_slave.h"

#define UART USART0

void USART0_RX_IRQHandler(void)
{
    if (UART->IF & USART_IF_RXDATAV) {
        uint8_t byte = USART_Rx(USART0);
        modbus_handle_rx_byte(byte);
    }
}

void uart_init(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_USART0, true);

  // Inicjalizacja USART w trybie asynchronicznym (8N1)
  USART_InitAsync_TypeDef init = USART_INITASYNC_DEFAULT;
  init.baudrate = 115200;
  USART_InitAsync(UART, &init);

                        // =====  USB =====

  GPIO_PinModeSet(gpioPortA, 6, gpioModePushPull, 1);  // TX (PA6)
  GPIO_PinModeSet(gpioPortA, 5, gpioModeInput, 0);     // RX (PA5)

  GPIO->USARTROUTE[0].TXROUTE = (gpioPortA << _GPIO_USART_TXROUTE_PORT_SHIFT) |
                                (6 << _GPIO_USART_TXROUTE_PIN_SHIFT);  // PA6 = TX
  GPIO->USARTROUTE[0].RXROUTE = (gpioPortA << _GPIO_USART_RXROUTE_PORT_SHIFT) |
                                (5 << _GPIO_USART_RXROUTE_PIN_SHIFT);  // PA5 = RX
  GPIO->USARTROUTE[0].ROUTEEN = GPIO_USART_ROUTEEN_RXPEN | GPIO_USART_ROUTEEN_TXPEN;

  USART_IntEnable(UART, USART_IEN_RXDATAV);
  /*
   *
                        // === MODBUS RS485 ===

  // GPIO konfiguracja:
  GPIO_PinModeSet(gpioPortA, 5, gpioModePushPull, 1);  // TX (PA5)
  GPIO_PinModeSet(gpioPortA, 6, gpioModeInput, 0);     // RX (PA6)
  GPIO_PinModeSet(gpioPortA, 4, gpioModePushPull, 0);  // DIR (PA4)

  // ROUTING - RS485
  GPIO->USARTROUTE[0].TXROUTE = (gpioPortA << _GPIO_USART_TXROUTE_PORT_SHIFT) |
                                (5 << _GPIO_USART_TXROUTE_PIN_SHIFT);  // PA5 = TX
  GPIO->USARTROUTE[0].RXROUTE = (gpioPortA << _GPIO_USART_RXROUTE_PORT_SHIFT) |
                                (6 << _GPIO_USART_RXROUTE_PIN_SHIFT);  // PA6 = RX
  GPIO->USARTROUTE[0].ROUTEEN = GPIO_USART_ROUTEEN_RXPEN | GPIO_USART_ROUTEEN_TXPEN;
  */
}

void uart_send_byte(uint8_t data)
{
 // GPIO_PinOutSet(gpioPortA, 4);                 // === MODBUS RS485 ===

  USART_Tx(UART, data);

  //while (!(UART->STATUS & USART_STATUS_TXC));   // === MODBUS RS485 ===
  //GPIO_PinOutClear(gpioPortA, 4);               // === MODBUS RS485 ===
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
