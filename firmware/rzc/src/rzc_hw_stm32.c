/**
 * @file    rzc_hw_stm32.c
 * @brief   STM32 hardware stubs for RZC (Rear Zone Controller)
 * @date    2026-03-03
 *
 * @details Stub implementations of all Main_Hw_* externs declared in
 *          rzc/src/main.c. Every function returns E_OK or is a no-op
 *          so the STM32 build links. Real HAL implementations come in
 *          Phase F4.
 *
 * @safety_req N/A — stub only, not for production
 * @copyright Taktflow Systems 2026
 */

#include "Platform_Types.h"
#include "Std_Types.h"

/* ==================================================================
 * Error Handler — required by CubeMX HAL_FDCAN_MspInit()
 * ================================================================== */

/**
 * @brief  Error handler — infinite loop, watchdog will reset
 * @note   Required by shared HAL MspInit code. Real implementation
 *         comes in Phase F4 when RZC gets full bring-up.
 */
void Error_Handler(void)
{
    for (;;)
    {
        /* Watchdog will reset */
    }
}

/* ==================================================================
 * Timing stubs — TODO:HARDWARE call CubeMX / HAL in Phase F4
 * ================================================================== */

/**
 * @brief  Initialize system clocks — no-op stub
 * @note   TODO:HARDWARE — call CubeMX SystemClock_Config()
 */
void Main_Hw_SystemClockInit(void)
{
    /* TODO:HARDWARE — call CubeMX SystemClock_Config() */
}

/**
 * @brief  Configure MPU regions — no-op stub
 * @note   TODO:HARDWARE — configure MPU via HAL_MPU_ConfigRegion()
 */
void Main_Hw_MpuConfig(void)
{
    /* TODO:HARDWARE — configure MPU via HAL_MPU_ConfigRegion() */
}

/**
 * @brief  Initialize SysTick timer — stores period for future use
 * @param  periodUs  Tick period in microseconds
 * @note   TODO:HARDWARE — call HAL_SYSTICK_Config() with computed reload value
 */
void Main_Hw_SysTickInit(uint32 periodUs)
{
    (void)periodUs;
    /* TODO:HARDWARE — use HAL_SYSTICK_Config(periodUs * (SystemCoreClock / 1000000u)) */
}

/**
 * @brief  Wait for interrupt — no-op stub
 * @note   TODO:HARDWARE — call __WFI() on target
 */
void Main_Hw_Wfi(void)
{
    /* TODO:HARDWARE — __WFI() */
}

/**
 * @brief  Get elapsed time since SysTickInit
 * @return 0u (stub — no hardware timer running)
 * @note   TODO:HARDWARE — read HAL_GetTick() or DWT cycle counter
 */
uint32 Main_Hw_GetTick(void)
{
    /* TODO:HARDWARE — return real tick from HAL_GetTick() or DWT */
    return 0u;
}

/* ==================================================================
 * Self-test stubs (Main_Hw_* from main.c)
 * ================================================================== */

/**
 * @brief  BTS7960 motor driver GPIO self-test — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — toggle BTS7960 INH/IS pins and read back
 */
Std_ReturnType Main_Hw_Bts7960GpioTest(void)
{
    /* TODO:HARDWARE — real BTS7960 GPIO test */
    return E_OK;
}

/**
 * @brief  ACS723 current sensor zero calibration self-test — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — read ADC at zero current and verify offset
 */
Std_ReturnType Main_Hw_Acs723ZeroCalTest(void)
{
    /* TODO:HARDWARE — real ACS723 zero-cal test */
    return E_OK;
}

/**
 * @brief  NTC thermistor range self-test — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — read NTC ADC and verify within expected range
 */
Std_ReturnType Main_Hw_NtcRangeTest(void)
{
    /* TODO:HARDWARE — real NTC range test */
    return E_OK;
}

/**
 * @brief  Encoder stuck self-test — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — check encoder pulse count after brief motor drive
 */
Std_ReturnType Main_Hw_EncoderStuckTest(void)
{
    /* TODO:HARDWARE — real encoder stuck test */
    return E_OK;
}

/**
 * @brief  CAN loopback self-test — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — perform real CAN loopback via HAL_CAN
 */
Std_ReturnType Main_Hw_CanLoopbackTest(void)
{
    /* TODO:HARDWARE — real CAN loopback test */
    return E_OK;
}

/**
 * @brief  MPU verify self-test — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — verify MPU region configuration registers
 */
Std_ReturnType Main_Hw_MpuVerifyTest(void)
{
    /* TODO:HARDWARE — real MPU verify test */
    return E_OK;
}

/**
 * @brief  RAM pattern self-test — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — write/read march-C pattern to SRAM
 */
Std_ReturnType Main_Hw_RamPatternTest(void)
{
    /* TODO:HARDWARE — real RAM pattern test */
    return E_OK;
}

/**
 * @brief  Plant stack canary — no-op stub
 * @note   TODO:HARDWARE — write canary word at end of stack region
 */
void Main_Hw_PlantStackCanary(void)
{
    /* TODO:HARDWARE — plant canary at stack boundary */
}
