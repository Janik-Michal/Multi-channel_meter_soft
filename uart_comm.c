#include "em_usart.h"
#include "em_gpio.h"
#include "em_cmu.h"
#include "modbus_rtu_slave.h"
#include "flash_handle.h"

#define UART USART0

void USART0_RX_IRQHandler(void)
{
    if (UART->IF & USART_IF_RXDATAV) {
        uint8_t byte = USART_Rx(USART0);
        modbus_handle_rx_byte(byte);
    }
    USART_IntClear(USART0, USART_IF_RXDATAV);
}

void uart_set_baud_from_flash(void)
{
    Internal_data_struct *calib = flash_data_struct_getter();

    uint32_t baud;
    switch (calib->Modbus_baud) {
        case 0: baud = 9600; break;
        case 1: baud = 19200; break;
        case 2: baud = 38400; break;
        case 3: baud = 57600; break;
        case 4: baud = 115200; break;
        default: baud = 115200; break;
    }

    USART_Enable(UART, usartDisable);
    UART->CMD = USART_CMD_RXDIS | USART_CMD_TXDIS;

    USART_InitAsync_TypeDef init = USART_INITASYNC_DEFAULT;
    init.baudrate = baud;
    USART_InitAsync(UART, &init);

    USART_Enable(UART, usartEnable);
}

void uart_init(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_USART0, true);

  uart_set_baud_from_flash();
  // GPIO configuration:
  GPIO_PinModeSet(gpioPortA, 6, gpioModePushPull, 1);  // TX (PA6)
   GPIO_PinModeSet(gpioPortA, 5, gpioModeInput, 0);     // RX (PA5)

   GPIO->USARTROUTE[0].TXROUTE = (gpioPortA << _GPIO_USART_TXROUTE_PORT_SHIFT) |
                                 (6 << _GPIO_USART_TXROUTE_PIN_SHIFT);  // PA6 = TX
   GPIO->USARTROUTE[0].RXROUTE = (gpioPortA << _GPIO_USART_RXROUTE_PORT_SHIFT) |
                                 (5 << _GPIO_USART_RXROUTE_PIN_SHIFT);  // PA5 = RX
   GPIO->USARTROUTE[0].ROUTEEN = GPIO_USART_ROUTEEN_RXPEN | GPIO_USART_ROUTEEN_TXPEN;

  USART_IntEnable(UART, USART_IEN_RXDATAV);
}

void uart_send_byte(uint8_t data)
{
 // GPIO_PinOutSet(gpioPortA, 4);                 // === MODBUS RS485 ===

  USART_Tx(UART, data);

 // while (!(UART->STATUS & USART_STATUS_TXC));   // === MODBUS RS485 ===
 // GPIO_PinOutClear(gpioPortA, 4);               // === MODBUS RS485 ===
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
