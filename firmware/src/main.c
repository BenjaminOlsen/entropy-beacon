/*
 * REB-01 Remote Entropy Beacon - Entropy Pipeline
 *
 * Collects accelerometer bursts, extracts entropy bits via delta-sign,
 * runs health tests (RCT/APT), estimates min-entropy (MCV), and
 * prints results over UART.
 */

#include "stm32l4xx_hal.h"
#include "adxl345.h"
#include "bh1750.h"
#include "uart.h"
#include "entropy/entropy.h"
#include <cfx/ed25519.h>
#include <stdio.h>
#include <string.h>

#define LED_PIN    GPIO_PIN_3
#define LED_PORT   GPIOB

/* Burst sizes */
#define ACCEL_BURST    64
#define LUX_BURST      8    /* ~1 reading per 40ms, 8 over the accel burst */
#define ACCEL_BITS     (ACCEL_BURST - 1)   /* 63 delta-sign bits per axis */
#define LUX_BITS       (LUX_BURST - 1)     /* 7 delta-sign bits from lux */
#define TOTAL_BITS     (ACCEL_BITS * 3 + LUX_BITS)  /* 189 + 7 = 196 */

/* Health test thresholds (for binary source, per NIST SP 800-90B) */
#define RCT_CUTOFF  20   /* 20 identical bits in a row = fail */
#define APT_WINDOW  64
#define APT_CUTOFF  48   /* >48 of 64 same as ref = fail */

static I2C_HandleTypeDef hi2c1;

static void SystemClock_Config(void);
static void GPIO_Init(void);
static void I2C1_Init(void);
static void Error_Handler(void);

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

    char buf[140];

    /******************************************************************/
    /* signing keypair ************************************************/
    /*  TODO: provision per-device seed in flash!!!! ******************/
    /******************************************************************/
    static const uint8_t seed[32] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
    };
    uint8_t pk[32], sk[64];
    cfx_ed25519_create_keypair(pk, sk, seed);

    uart_puts("Ed25519 pk: ");
    for (int i = 0; i < 32; i++) {
        snprintf(buf, sizeof(buf), "%02x", pk[i]);
        uart_puts(buf);
    }
    uart_puts("\r\n");

    /* Health test state */
    entropy_rct_t rct;
    entropy_apt_t apt;
    entropy_rct_init(&rct, RCT_CUTOFF);
    entropy_apt_init(&apt, APT_WINDOW, APT_CUTOFF);

    /* Entropy pool — accumulates until we have 256 bits of real entropy */
    entropy_pool_t pool;
    entropy_pool_init(&pool);

    uint32_t cycle = 0;
    uint32_t output_count = 0;

    while (1)
    {
#if VERBOSE
        uint32_t t_start = HAL_GetTick();
#endif
        /* Collect a burst of accelerometer + lux samples */
        int16_t samples_x[ACCEL_BURST];
        int16_t samples_y[ACCEL_BURST];
        int16_t samples_z[ACCEL_BURST];
        uint16_t samples_lux[LUX_BURST];
        int lux_idx = 0;

        for (int i = 0; i < ACCEL_BURST; i++) {
            adxl345_data_t ad;
            if (adxl345_read(&hi2c1, &ad) == HAL_OK) {
                samples_x[i] = ad.x;
                samples_y[i] = ad.y;
                samples_z[i] = ad.z;
            }

            /* Read lux every 8th accel sample (~40ms apart) */
            if (i % (ACCEL_BURST / LUX_BURST) == 0 && lux_idx < LUX_BURST) {
                uint16_t lx;
                if (bh1750_read(&hi2c1, &lx) == HAL_OK) {
                    samples_lux[lux_idx] = lx;
                }
                lux_idx++;
            }

            HAL_Delay(3);  /* ~333 Hz (ADXL345 at 1600 Hz ODR) */
        }

        /* Extract delta-sign bits from accel axes + lux */
        uint8_t bits[TOTAL_BITS];
        size_t n = 0;
        n += entropy_delta_sign(samples_x, ACCEL_BURST, &bits[n], ACCEL_BITS);
        n += entropy_delta_sign(samples_y, ACCEL_BURST, &bits[n], ACCEL_BITS);
        n += entropy_delta_sign(samples_z, ACCEL_BURST, &bits[n], ACCEL_BITS);
        n += entropy_delta_sign_u16(samples_lux, lux_idx, &bits[n], LUX_BITS);

        /* Run health tests on extracted bits */
        int rct_fail = 0;
        int apt_fail = 0;
        for (size_t i = 0; i < n; i++) {
            if (entropy_rct_update(&rct, bits[i])) {
                rct_fail = 1;
            }
            if (entropy_apt_update(&apt, bits[i])) {
                apt_fail = 1;
            }
        }

        uint32_t h_milli = entropy_mcv_estimate(bits, n);
#if VERBOSE
        /* Estimate min-entropy */
        uint32_t burst_entropy = (uint32_t)n * h_milli;

        /* Last samples as representative readings */
        int16_t last_x = samples_x[ACCEL_BURST - 1];
        int16_t last_y = samples_y[ACCEL_BURST - 1];
        int16_t last_z = samples_z[ACCEL_BURST - 1];
        uint16_t last_lux = (lux_idx > 0) ? samples_lux[lux_idx - 1] : 0;

        uint32_t t_ms = HAL_GetTick() - t_start;

        snprintf(buf, sizeof(buf),
                 "[%lu] X:%d Y:%d Z:%d lux:%u | bits:%u h:%lu.%03lu ent:%lu.%03lu rct:%s apt:%s pool:%lu %lums\r\n",
                 (unsigned long)cycle,
                 (int)last_x, (int)last_y, (int)last_z,
                 (unsigned)last_lux,
                 (unsigned)n,
                 (unsigned long)(h_milli / 1000),
                 (unsigned long)(h_milli % 1000),
                 (unsigned long)(burst_entropy / 1000),
                 (unsigned long)(burst_entropy % 1000),
                 rct_fail ? "FAIL" : "ok",
                 apt_fail ? "FAIL" : "ok",
                 (unsigned long)pool.accum_millibits,
                 (unsigned long)t_ms);
        uart_puts(buf);
#endif

        /* bits into pool (skip if health tests failed) */
        if (!rct_fail && !apt_fail) {
            int ready = entropy_pool_feed(&pool, bits, n, h_milli);

            if (ready) {
                uint8_t conditioned[32];
                if (entropy_pool_output(&pool, conditioned) == 0) {
                    output_count++;

                    /* Sign the conditioned output */
                    uint8_t sig[64];
                    cfx_ed25519_sign(sig, conditioned, 32, sk);

#if VERBOSE
                    snprintf(buf, sizeof(buf), "  >> [#%lu]: ", (unsigned long)output_count);
                    uart_puts(buf);
#endif
                    /* Print 32 bytes entropy as hex */
                    for (int i = 0; i < 32; i++) {
                        snprintf(buf, sizeof(buf), "%02x", conditioned[i]);
                        uart_puts(buf);
                    }
                    uart_puts("\r\n");

                    /* Print 64 bytes signature as hex */
                    uart_puts("  sig: ");
                    for (int i = 0; i < 64; i++) {
                        snprintf(buf, sizeof(buf), "%02x", sig[i]);
                        uart_puts(buf);
                    }
                    uart_puts("\r\n");
                }
            }
        }

        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        cycle++;
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

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
        Error_Handler();
    }
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

    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }
}

static void Error_Handler(void)
{
    while (1) {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        for (volatile uint32_t i = 0; i < 200000; i++) {}
    }
}
