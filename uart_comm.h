#ifndef UART_COMM_H
#define UART_COMM_H

#include <stdint.h>

void uart_init(void);
void uart_send_byte(uint8_t byte);
void uart_send_string(const char *str);

#endif // UART_COMM_H
