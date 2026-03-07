/*
 * REB-01 Remote Entropy Beacon - Sensor Readout
 *
 * Initializes I2C1, reads ADXL345 accelerometer and BH1750 light
 * sensor, and blinks the LED. Sensor values can be inspected via
 * debugger (live watch on accel_data and lux).
 */

#include "stm32l4xx_hal.h"
#include "adxl345.h"
#include "bh1750.h"
#include "uart.h"
#include <stdio.h>

#define LED_PIN    GPIO_PIN_3
#define LED_PORT   GPIOB

static I2C_HandleTypeDef hi2c1;

static void SystemClock_Config(void);
static void GPIO_Init(void);
static void I2C1_Init(void);
static void Error_Handler(void);

/* Sensor data — inspect via debugger */
volatile adxl345_data_t accel_data;
volatile uint16_t lux;

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    uart_init();

    uart_puts("REB-01 booting...\r\n");

    I2C1_Init();
    uart_puts("I2C1 ok\r\n");

    if (adxl345_init(&hi2c1) != HAL_OK) {
        uart_puts("ADXL345 init FAILED\r\n");
        Error_Handler();
    }
    uart_puts("ADXL345 ok\r\n");

    if (bh1750_init(&hi2c1) != HAL_OK) {
        uart_puts("BH1750 init FAILED\r\n");
        Error_Handler();
    }
    uart_puts("BH1750 ok\r\n");

    char buf[80];

    while (1)
    {
        adxl345_data_t ad;
        uint16_t lx;

        if (adxl345_read(&hi2c1, &ad) == HAL_OK)
            accel_data = ad;

        if (bh1750_read(&hi2c1, &lx) == HAL_OK)
            lux = lx;

        snprintf(buf, sizeof(buf), "X:%6d Y:%6d Z:%6d  lux:%5u\r\n",
                 accel_data.x, accel_data.y, accel_data.z, lux);
        uart_puts(buf);

        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(100);
    }
}

/*
 * Configure system clock to 80 MHz using MSI + PLL.
 * MSI at 4 MHz -> PLL -> 80 MHz SYSCLK.
 */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = 0;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 40;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        Error_Handler();

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
        Error_Handler();
}

/*
 * PB3 push-pull output for LED.
 * PB6 (SCL) and PB7 (SDA) are configured by I2C1_Init via HAL.
 */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
}

/*
 * I2C1 on PB6 (SCL) / PB7 (SDA), 400 kHz fast mode.
 */
static void I2C1_Init(void)
{
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x00702991;  /* 400 kHz @ 80 MHz PCLK1 */
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
        Error_Handler();
}

static void Error_Handler(void)
{
    while (1) {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        for (volatile uint32_t i = 0; i < 200000; i++);
    }
}
