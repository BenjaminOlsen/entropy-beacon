/*
 * System initialization for STM32L432KC.
 * Sets up the vector table offset and FPU.
 * Called by startup code before main().
 */

#include "stm32l4xx.h"

/* Default: MSI at 4 MHz */
uint32_t SystemCoreClock = 4000000U;
const uint8_t AHBPrescTable[16] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
                                    1U, 2U, 3U, 4U, 6U, 7U, 8U, 9U};
const uint8_t APBPrescTable[8]  = {0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U};
const uint32_t MSIRangeTable[12] = {100000U,   200000U,   400000U,   800000U,
                                    1000000U,  2000000U,  4000000U,  8000000U,
                                    16000000U, 24000000U, 32000000U, 48000000U};

void SystemInit(void)
{
    /* FPU settings: enable CP10 and CP11 */
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 20U) | (3UL << 22U));
#endif

    /* Set vector table offset */
    SCB->VTOR = FLASH_BASE;
}
