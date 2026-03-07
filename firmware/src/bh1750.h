#ifndef BH1750_H
#define BH1750_H

#include "stm32l4xx_hal.h"
#include <stdint.h>

#define BH1750_ADDR  (0x23 << 1)  /* HAL uses 8-bit address (left-shifted) */

/* Commands */
#define BH1750_CMD_POWER_ON    0x01
#define BH1750_CMD_RESET       0x07
#define BH1750_CMD_CONT_HRES   0x10  /* Continuous high-res mode, 1 lx resolution */

HAL_StatusTypeDef bh1750_init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef bh1750_read(I2C_HandleTypeDef *hi2c, uint16_t *lux);

#endif
