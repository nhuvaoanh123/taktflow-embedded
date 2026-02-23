/**
 * @file    fzc_hw_posix.c
 * @brief   POSIX hardware stubs for FZC (Front Zone Controller) SIL simulation
 * @date    2026-02-23
 *
 * @details Implements all Main_Hw_* externs declared in fzc/src/main.c:68-81.
 *          Timing uses clock_gettime(CLOCK_MONOTONIC) for tick measurement.
 *          Self-test functions return E_OK (simulation = no hardware faults).
 *
 * @safety_req N/A — SIL simulation only, not for production
 * @copyright Taktflow Systems 2026
 */

#include "Platform_Types.h"
#include "Std_Types.h"

#ifndef PLATFORM_POSIX_TEST
#include <time.h>
#include <unistd.h>
#endif

/* ==================================================================
 * Module state
 * ================================================================== */

static uint32 tick_period_us = 10000u;  /* Default 10ms tick */

#ifndef PLATFORM_POSIX_TEST
static struct timespec tick_origin;
#endif

/* ==================================================================
 * Timing stubs (Main_Hw_* from main.c:68-72)
 * ================================================================== */

/**
 * @brief  Initialize system clocks — no-op on POSIX
 */
void Main_Hw_SystemClockInit(void)
{
    /* POSIX: no PLL configuration needed */
}

/**
 * @brief  Configure MPU — no-op on POSIX
 */
void Main_Hw_MpuConfig(void)
{
    /* POSIX: no MPU hardware */
}

/**
 * @brief  Initialize SysTick timer — record base timestamp
 * @param  periodUs  Tick period in microseconds
 */
void Main_Hw_SysTickInit(uint32 periodUs)
{
    tick_period_us = periodUs;
#ifndef PLATFORM_POSIX_TEST
    clock_gettime(CLOCK_MONOTONIC, &tick_origin);
#endif
}

/**
 * @brief  Wait for interrupt — sleep for 1ms on POSIX
 */
void Main_Hw_Wfi(void)
{
#ifndef PLATFORM_POSIX_TEST
    usleep(1000u); /* 1ms sleep — simulates idle wait */
#endif
}

/**
 * @brief  Get elapsed time since SysTickInit in microseconds
 * @return Elapsed microseconds (wraps at uint32 max)
 */
uint32 Main_Hw_GetTick(void)
{
#ifndef PLATFORM_POSIX_TEST
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    uint32 elapsed_us = (uint32)(
        ((now.tv_sec - tick_origin.tv_sec) * 1000000u) +
        ((now.tv_nsec - tick_origin.tv_nsec) / 1000u)
    );
    return elapsed_us;
#else
    return 0u;
#endif
}

/* ==================================================================
 * Self-test stubs (Main_Hw_* from main.c:74-81)
 * ================================================================== */

/**
 * @brief  Steering servo neutral self-test — always passes in SIL
 * @return E_OK
 */
Std_ReturnType Main_Hw_ServoNeutralTest(void)
{
    return E_OK;
}

/**
 * @brief  SPI sensor self-test — always passes in SIL
 * @return E_OK
 */
Std_ReturnType Main_Hw_SpiSensorTest(void)
{
    return E_OK;
}

/**
 * @brief  UART lidar self-test — always passes in SIL
 * @return E_OK
 */
Std_ReturnType Main_Hw_UartLidarTest(void)
{
    return E_OK;
}

/**
 * @brief  CAN loopback self-test — always passes in SIL
 * @return E_OK
 */
Std_ReturnType Main_Hw_CanLoopbackTest(void)
{
    return E_OK;
}

/**
 * @brief  MPU verify self-test — always passes in SIL
 * @return E_OK
 */
Std_ReturnType Main_Hw_MpuVerifyTest(void)
{
    return E_OK;
}

/**
 * @brief  RAM pattern self-test — always passes in SIL
 * @return E_OK
 */
Std_ReturnType Main_Hw_RamPatternTest(void)
{
    return E_OK;
}

/**
 * @brief  Plant stack canary — no-op on POSIX
 */
void Main_Hw_PlantStackCanary(void)
{
    /* POSIX: stack protection handled by OS */
}
