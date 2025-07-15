#ifndef MODBUS_RTU_SLAVE_H
#define MODBUS_RTU_SLAVE_H

#include <stdint.h>

#define MODBUS_REG_COUNT 32

extern int16_t holding_registers[MODBUS_REG_COUNT];

void modbus_handle_rx_byte(uint8_t byte);
void modbus_on_frame_timeout(void);
void modbus_poll(void);

#endif
