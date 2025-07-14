#include "modbus_rtu_slave.h"
#include <string.h>
#include <stdio.h>
#include <em_usart.h>
#include <em_gpio.h>
#include <em_cmu.h>
#include <em_core.h>

#define SLAVE_ADDRESS 1
#define NUM_HOLDING_REGS 5
#define MODBUS_MAX_REQUEST_LEN 256

static uint8_t rx_buffer[MODBUS_MAX_REQUEST_LEN];
static uint8_t rx_index = 0;

uint16_t modbus_regs[NUM_HOLDING_REGS];
extern float temperatures_C[NUM_HOLDING_REGS];

static uint32_t system_millis = 0;  // Licznik milisekund

// ⏱️ Funkcja do pobrania czasu
uint32_t get_tick_ms(void) {
    return system_millis;
}

// ⏱️ Inicjalizacja SysTick – wywoływana np. w main()
void systick_init(void) {
    // SysTick co 1 ms (zakładając 14 MHz clocku, modyfikuj jak trzeba)
    SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000);
}

// ⏱️ Przerwanie SysTick – inkrementuje millis
void SysTick_Handler(void) {
    system_millis++;
}

// CRC16 MODBUS
static uint16_t modbus_crc16(const uint8_t *buf, uint8_t len) {
    uint16_t crc = 0xFFFF;
    for (uint8_t pos = 0; pos < len; pos++) {
        crc ^= buf[pos];
        for (uint8_t i = 0; i < 8; i++) {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

void modbus_poll(void)
{
    static uint32_t last_rx_time_ms = 0;
    uint32_t now = get_tick_ms();

    // ⏱️ Timeout odbioru
    if ((rx_index > 0) && (now - last_rx_time_ms > 20)) {
        printf("RX timeout — reset\n");
        rx_index = 0;
    }

    // 1. Odczytaj dane z USART
    while (USART0->STATUS & USART_STATUS_RXDATAV) {
        uint8_t byte = USART_Rx(USART0);
        if (rx_index < MODBUS_MAX_REQUEST_LEN) {
            rx_buffer[rx_index++] = byte;
            last_rx_time_ms = get_tick_ms();  // zapis czasu ostatniego bajtu
        } else {
            rx_index = 0;
        }
    }

    if (rx_index < 8)
        return;

    printf("Received %d bytes: ", rx_index);
    for (int i = 0; i < rx_index; i++) {
        printf("%02X ", rx_buffer[i]);
    }
    printf("\n");

    if (rx_buffer[0] != SLAVE_ADDRESS) {
        printf("Wrong address: %d\n", rx_buffer[0]);
        rx_index = 0;
        return;
    }

    if (rx_buffer[1] != 0x03) {
        printf("Unsupported function: %02X\n", rx_buffer[1]);
        rx_index = 0;
        return;
    }

    uint16_t crc_received = (rx_buffer[7] << 8) | rx_buffer[6];
    uint16_t crc_calc = modbus_crc16(rx_buffer, 6);
    printf("CRC received: %04X, calculated: %04X\n", crc_received, crc_calc);
    if (crc_calc != crc_received) {
        printf("CRC mismatch!\n");
        rx_index = 0;
        return;
    }

    uint16_t start_addr = (rx_buffer[2] << 8) | rx_buffer[3];
    uint16_t quantity   = (rx_buffer[4] << 8) | rx_buffer[5];

    if (quantity == 0 || (start_addr + quantity) > NUM_HOLDING_REGS) {
        printf("Invalid register range: start=%d, qty=%d\n", start_addr, quantity);
        rx_index = 0;
        return;
    }

    for (int i = 0; i < NUM_HOLDING_REGS; i++) {
        modbus_regs[i] = (uint16_t)(temperatures_C[i] * 100.0f + 0.5f);
    }

    uint8_t response[3 + quantity * 2 + 2];
    response[0] = SLAVE_ADDRESS;
    response[1] = 0x03;
    response[2] = quantity * 2;

    for (int i = 0; i < quantity; i++) {
        uint16_t reg = modbus_regs[start_addr + i];
        response[3 + i * 2] = reg >> 8;
        response[4 + i * 2] = reg & 0xFF;
    }

    uint16_t crc = modbus_crc16(response, 3 + quantity * 2);
    response[3 + quantity * 2] = crc & 0xFF;
    response[4 + quantity * 2] = crc >> 8;

    printf("Sending response (%d bytes): ", 3 + quantity * 2 + 2);
    for (int i = 0; i < 3 + quantity * 2 + 2; i++) {
        printf("%02X ", response[i]);
    }
    printf("\n");

    for (int i = 0; i < 3 + quantity * 2 + 2; i++) {
        USART_Tx(USART0, response[i]);
    }
    while (!(USART0->STATUS & USART_STATUS_TXC));

    rx_index = 0;
}
