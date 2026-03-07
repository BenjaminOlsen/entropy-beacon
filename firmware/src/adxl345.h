#ifndef ADXL345_H
#define ADXL345_H

#include "stm32l4xx_hal.h"
#include <stdint.h>

#define ADXL345_ADDR       (0x53 << 1)  /* HAL uses 8-bit address (left-shifted) */

/* Registers */
#define ADXL345_REG_DEVID       0x00
#define ADXL345_REG_BW_RATE     0x2C
#define ADXL345_REG_POWER_CTL   0x2D
#define ADXL345_REG_DATA_FORMAT 0x31
#define ADXL345_REG_DATAX0      0x32  /* 6 bytes: X0,X1,Y0,Y1,Z0,Z1 */

/* BW_RATE values */
#define ADXL345_RATE_200HZ  0x0B
#define ADXL345_RATE_100HZ  0x0A

/* DATA_FORMAT values */
#define ADXL345_RANGE_2G    0x00
#define ADXL345_RANGE_4G    0x01
#define ADXL345_RANGE_8G    0x02
#define ADXL345_RANGE_16G   0x03
#define ADXL345_FULL_RES    0x08

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} adxl345_data_t;

HAL_StatusTypeDef adxl345_init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef adxl345_read(I2C_HandleTypeDef *hi2c, adxl345_data_t *data);

#endif
