/*
 * STM32L4xx HAL configuration for REB-01
 * Enable only the modules we need.
 */

#ifndef STM32L4XX_HAL_CONF_H
#define STM32L4XX_HAL_CONF_H

#define HAL_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED

/* Oscillator values */
#if !defined(HSE_VALUE)
  #define HSE_VALUE    8000000U  /* External oscillator (not used on Nucleo) */
#endif

#if !defined(MSI_VALUE)
  #define MSI_VALUE    4000000U  /* MSI default */
#endif

#if !defined(HSI_VALUE)
  #define HSI_VALUE    16000000U
#endif

#if !defined(LSE_VALUE)
  #define LSE_VALUE    32768U
#endif

#if !defined(LSI_VALUE)
  #define LSI_VALUE    32000U
#endif

#if !defined(HSI48_VALUE)
  #define HSI48_VALUE  48000000U
#endif

#if !defined(HSE_STARTUP_TIMEOUT)
  #define HSE_STARTUP_TIMEOUT  100U
#endif

#if !defined(MSI_STARTUP_TIMEOUT)
  #define MSI_STARTUP_TIMEOUT  100U
#endif

#if !defined(HSI_STARTUP_TIMEOUT)
  #define HSI_STARTUP_TIMEOUT  100U
#endif

#if !defined(LSE_STARTUP_TIMEOUT)
  #define LSE_STARTUP_TIMEOUT  5000U
#endif

#if !defined(EXTERNAL_SAI1_CLOCK_VALUE)
  #define EXTERNAL_SAI1_CLOCK_VALUE  48000U
#endif

#if !defined(EXTERNAL_SAI2_CLOCK_VALUE)
  #define EXTERNAL_SAI2_CLOCK_VALUE  48000U
#endif

/* System tick */
#define TICK_INT_PRIORITY  0x0FU
#define USE_RTOS           0U
#define PREFETCH_ENABLE    0U
#define INSTRUCTION_CACHE_ENABLE   1U
#define DATA_CACHE_ENABLE          1U

/* Assert macro — disabled in production, maps to void */
#ifdef USE_FULL_ASSERT
  #define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
  void assert_failed(uint8_t *file, uint32_t line);
#else
  #define assert_param(expr) ((void)0U)
#endif

/* HAL module includes */
#ifdef HAL_RCC_MODULE_ENABLED
  #include "stm32l4xx_hal_rcc.h"
#endif
#ifdef HAL_GPIO_MODULE_ENABLED
  #include "stm32l4xx_hal_gpio.h"
#endif
#ifdef HAL_DMA_MODULE_ENABLED
  #include "stm32l4xx_hal_dma.h"
#endif
#ifdef HAL_CORTEX_MODULE_ENABLED
  #include "stm32l4xx_hal_cortex.h"
#endif
#ifdef HAL_FLASH_MODULE_ENABLED
  #include "stm32l4xx_hal_flash.h"
#endif
#ifdef HAL_I2C_MODULE_ENABLED
  #include "stm32l4xx_hal_i2c.h"
#endif
#ifdef HAL_PWR_MODULE_ENABLED
  #include "stm32l4xx_hal_pwr.h"
#endif
#ifdef HAL_UART_MODULE_ENABLED
  #include "stm32l4xx_hal_uart.h"
#endif

#endif /* STM32L4XX_HAL_CONF_H */
