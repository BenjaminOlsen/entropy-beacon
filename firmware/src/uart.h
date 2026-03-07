#ifndef UART_H
#define UART_H

#include "stm32l4xx_hal.h"
#include <stddef.h>

void uart_init(void);
void uart_puts(const char *s);
void uart_write(const uint8_t *data, size_t len);

#endif
