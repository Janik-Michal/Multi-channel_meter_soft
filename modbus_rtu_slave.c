#include "modbus_rtu_slave.h"
#include "em_timer.h"
#include <string.h>
#include <stdbool.h>

#define MODBUS_BUFFER_SIZE 256
#define MODBUS_ADDRESS     1
#define MODBUS_REG_COUNT   32

static volatile uint8_t modbus_rx_buf[MODBUS_BUFFER_SIZE];
static volatile uint8_t modbus_rx_count = 0;
static volatile bool modbus_frame_ready = false;

int16_t holding_registers[MODBUS_REG_COUNT] = {0,0,0,0,0};

extern void uart_send_byte(uint8_t data);

// CRC16 Modbus
static uint16_t modbus_crc16(const uint8_t *buffer, uint8_t length) {
    uint16_t crc = 0xFFFF;
    for (uint8_t pos = 0; pos < length; pos++) {
        crc ^= buffer[pos];
        for (uint8_t i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else
                crc >>= 1;
        }
    }
    return crc;
}

void modbus_handle_rx_byte(uint8_t byte) {
    if (modbus_frame_ready) return;  // Drop new bytes until frame processed
    if (modbus_rx_count < MODBUS_BUFFER_SIZE) {
        modbus_rx_buf[modbus_rx_count++] = byte;
        // Restart timer on every byte received
        TIMER_CounterSet(TIMER0, 0);
    } else {
        modbus_rx_count = 0;
    }
}

// Call from TIMER0 ISR after 3.5 char silence (~1 ms at 115200 baud)
void modbus_on_frame_timeout(void) {
    if (modbus_rx_count > 4) {
        modbus_frame_ready = true;
    } else {
        modbus_rx_count = 0;
    }
}

void modbus_poll(void) {
    if (!modbus_frame_ready) return;

    const uint8_t *req = modbus_rx_buf;
    uint8_t slave = req[0];
    uint8_t func  = req[1];

    if (slave != MODBUS_ADDRESS) goto cleanup;

    uint16_t crc_calc = modbus_crc16(req, modbus_rx_count - 2);
    uint16_t crc_recv = req[modbus_rx_count - 2] | (req[modbus_rx_count - 1] << 8);

    if (crc_calc != crc_recv) goto cleanup;

    uint8_t resp[MODBUS_BUFFER_SIZE];
    uint8_t resp_len = 0;

    if (func == 0x03 && modbus_rx_count >= 8) {
        uint16_t start = (req[2] << 8) | req[3];
        uint16_t count = (req[4] << 8) | req[5];

        if ((start + count) > MODBUS_REG_COUNT) {
            resp[0] = MODBUS_ADDRESS;
            resp[1] = func | 0x80;
            resp[2] = 0x02; // Illegal data address
            resp_len = 3;
        } else {
            resp[0] = MODBUS_ADDRESS;
            resp[1] = func;
            resp[2] = count * 2;
            for (uint16_t i = 0; i < count; i++) {
                uint16_t val = holding_registers[start + i];
                resp[3 + i * 2] = (val >> 8) & 0xFF;
                resp[4 + i * 2] = val & 0xFF;
            }
            resp_len = 3 + count * 2;
        }
    } else {
        resp[0] = MODBUS_ADDRESS;
        resp[1] = func | 0x80;
        resp[2] = 0x01; // Illegal function
        resp_len = 3;
    }

    uint16_t crc = modbus_crc16(resp, resp_len);
    resp[resp_len++] = crc & 0xFF;
    resp[resp_len++] = (crc >> 8) & 0xFF;

    for (uint8_t i = 0; i < resp_len; i++)
        uart_send_byte(resp[i]);

cleanup:
    modbus_rx_count = 0;
    modbus_frame_ready = false;
}
