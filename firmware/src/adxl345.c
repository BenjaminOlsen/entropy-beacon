#include "adxl345.h"

static HAL_StatusTypeDef adxl345_write_reg(I2C_HandleTypeDef *hi2c,
                                           uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return HAL_I2C_Master_Transmit(hi2c, ADXL345_ADDR, buf, 2, HAL_MAX_DELAY);
}

HAL_StatusTypeDef adxl345_init(I2C_HandleTypeDef *hi2c)
{
    HAL_StatusTypeDef status;

    /* Verify device ID (should be 0xE5) */
    uint8_t reg = ADXL345_REG_DEVID;
    uint8_t devid = 0;
    status = HAL_I2C_Master_Transmit(hi2c, ADXL345_ADDR, &reg, 1, HAL_MAX_DELAY);
    if (status != HAL_OK) return status;
    status = HAL_I2C_Master_Receive(hi2c, ADXL345_ADDR, &devid, 1, HAL_MAX_DELAY);
    if (status != HAL_OK) return status;
    if (devid != 0xE5) return HAL_ERROR;

    /* Full resolution, +/-4g range */
    status = adxl345_write_reg(hi2c, ADXL345_REG_DATA_FORMAT,
                               ADXL345_FULL_RES | ADXL345_RANGE_4G);
    if (status != HAL_OK) return status;

    /* 200 Hz output data rate */
    status = adxl345_write_reg(hi2c, ADXL345_REG_BW_RATE, ADXL345_RATE_200HZ);
    if (status != HAL_OK) return status;

    /* Start measurement */
    status = adxl345_write_reg(hi2c, ADXL345_REG_POWER_CTL, 0x08);
    return status;
}

HAL_StatusTypeDef adxl345_read(I2C_HandleTypeDef *hi2c, adxl345_data_t *data)
{
    uint8_t reg = ADXL345_REG_DATAX0;
    uint8_t buf[6];
    HAL_StatusTypeDef status;

    status = HAL_I2C_Master_Transmit(hi2c, ADXL345_ADDR, &reg, 1, HAL_MAX_DELAY);
    if (status != HAL_OK) return status;

    status = HAL_I2C_Master_Receive(hi2c, ADXL345_ADDR, buf, 6, HAL_MAX_DELAY);
    if (status != HAL_OK) return status;

    data->x = (int16_t)(buf[1] << 8 | buf[0]);
    data->y = (int16_t)(buf[3] << 8 | buf[2]);
    data->z = (int16_t)(buf[5] << 8 | buf[4]);

    return HAL_OK;
}
