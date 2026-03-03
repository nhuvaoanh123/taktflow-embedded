/**
 * @file    cvc_hw_stm32.c
 * @brief   STM32 hardware stubs for CVC (Central Vehicle Computer)
 * @date    2026-03-03
 *
 * @details Stub implementations of all Main_Hw_*, SelfTest_Hw_*, and
 *          Ssd1306_Hw_* externs declared in cvc/src/main.c and Swc_SelfTest.c.
 *          Every function returns E_OK or is a no-op so the STM32 build links.
 *          Real HAL implementations come in Phase F4.
 *
 * @safety_req N/A — stub only, not for production
 * @copyright Taktflow Systems 2026
 */

#include "Platform_Types.h"
#include "Std_Types.h"

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
 * Startup self-test stubs (Main_Hw_* from main.c)
 * ================================================================== */

/**
 * @brief  SPI loopback self-test — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — perform real SPI loopback via HAL_SPI_TransmitReceive()
 */
Std_ReturnType Main_Hw_SpiLoopbackTest(void)
{
    /* TODO:HARDWARE — real SPI loopback test */
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
 * @brief  OLED I2C ACK self-test — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — probe SSD1306 via HAL_I2C_IsDeviceReady()
 */
Std_ReturnType Main_Hw_OledAckTest(void)
{
    /* TODO:HARDWARE — real OLED ACK probe */
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

/* ==================================================================
 * Periodic self-test stubs (SelfTest_Hw_* from Swc_SelfTest.c)
 * ================================================================== */

/**
 * @brief  Periodic SPI loopback self-test — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — periodic SPI loopback via HAL
 */
Std_ReturnType SelfTest_Hw_SpiLoopback(void)
{
    /* TODO:HARDWARE — real periodic SPI loopback */
    return E_OK;
}

/**
 * @brief  Periodic CAN loopback self-test — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — periodic CAN loopback via HAL
 */
Std_ReturnType SelfTest_Hw_CanLoopback(void)
{
    /* TODO:HARDWARE — real periodic CAN loopback */
    return E_OK;
}

/**
 * @brief  NVM integrity check — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — read NVM CRC and verify
 */
Std_ReturnType SelfTest_Hw_NvmCheck(void)
{
    /* TODO:HARDWARE — real NVM integrity check */
    return E_OK;
}

/**
 * @brief  Periodic OLED I2C ACK self-test — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — periodic SSD1306 ACK probe
 */
Std_ReturnType SelfTest_Hw_OledAck(void)
{
    /* TODO:HARDWARE — real periodic OLED ACK */
    return E_OK;
}

/**
 * @brief  MPU region verify — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — verify MPU region configuration registers
 */
Std_ReturnType SelfTest_Hw_MpuVerify(void)
{
    /* TODO:HARDWARE — real MPU region verify */
    return E_OK;
}

/**
 * @brief  Stack canary check — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — compare canary word at stack boundary
 */
Std_ReturnType SelfTest_Hw_CanaryCheck(void)
{
    /* TODO:HARDWARE — real canary check */
    return E_OK;
}

/**
 * @brief  Periodic RAM pattern test — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — periodic march-C on reserved SRAM block
 */
Std_ReturnType SelfTest_Hw_RamPattern(void)
{
    /* TODO:HARDWARE — real periodic RAM pattern test */
    return E_OK;
}

/* ==================================================================
 * SSD1306 OLED I2C stub
 * ================================================================== */

/**
 * @brief  Write data to I2C bus — stub returns E_OK
 * @param  addr  7-bit I2C slave address
 * @param  data  Pointer to data buffer
 * @param  len   Number of bytes to write
 * @return E_OK always (stub — no hardware)
 * @note   TODO:HARDWARE — call HAL_I2C_Master_Transmit()
 */
Std_ReturnType Ssd1306_Hw_I2cWrite(uint8 addr, const uint8* data, uint8 len)
{
    (void)addr;
    (void)data;
    (void)len;
    /* TODO:HARDWARE — real I2C write via HAL_I2C_Master_Transmit() */
    return E_OK;
}
