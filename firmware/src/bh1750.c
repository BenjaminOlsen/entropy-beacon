#include "bh1750.h"

HAL_StatusTypeDef bh1750_init(I2C_HandleTypeDef *hi2c)
{
    HAL_StatusTypeDef status;
    uint8_t cmd;

    /* Power on */
    cmd = BH1750_CMD_POWER_ON;
    status = HAL_I2C_Master_Transmit(hi2c, BH1750_ADDR, &cmd, 1, HAL_MAX_DELAY);
    if (status != HAL_OK) {
        return status;
    }

    /* Start continuous high-resolution measurement */
    cmd = BH1750_CMD_CONT_HRES;
    status = HAL_I2C_Master_Transmit(hi2c, BH1750_ADDR, &cmd, 1, HAL_MAX_DELAY);
    return status;
}

HAL_StatusTypeDef bh1750_read(I2C_HandleTypeDef *hi2c, uint16_t *lux)
{
    uint8_t buf[2];
    HAL_StatusTypeDef status;

    status = HAL_I2C_Master_Receive(hi2c, BH1750_ADDR, buf, 2, HAL_MAX_DELAY);
    if (status != HAL_OK) {
        return status;
    }

    /* Raw value is big-endian, divide by 1.2 to get lux */
    uint16_t raw = (uint16_t)(buf[0] << 8 | buf[1]);
    *lux = raw * 10 / 12;

    return HAL_OK;
}
