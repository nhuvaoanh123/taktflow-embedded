/**
 * @file    Gpt_Hw_STM32.c
 * @brief   STM32G474 hardware backend for GPT MCAL driver
 * @date    2026-03-08
 *
 * @details Uses HAL_GetTick() (1ms SysTick, already running from HAL_Init)
 *          to provide microsecond-resolution elapsed time measurement.
 *          Each channel records its start tick and computes elapsed time
 *          on GetCounter calls.
 *
 *          Resolution: 1ms (1000us steps) since HAL SysTick runs at 1ms.
 *          Sufficient for 10ms BSW scheduling and WdgM deadline monitoring.
 *
 * @safety_req SWR-BSW-010: GPT Driver for Timing
 * @traces_to  SYS-053, TSR-046, TSR-047
 *
 * @copyright Taktflow Systems 2026
 */

#include "Platform_Types.h"
#include "Std_Types.h"
#include "Gpt.h"
#include "stm32g4xx_hal.h"

/* ==================================================================
 * Static state
 * ================================================================== */

#define GPT_HW_MAX_CHANNELS  4u

typedef struct {
    boolean running;
    uint32  target_us;
    uint32  start_tick_ms;
} Gpt_Hw_ChannelType;

static Gpt_Hw_ChannelType gpt_channels[GPT_HW_MAX_CHANNELS];

/* ==================================================================
 * Gpt_Hw_* API implementations
 * ================================================================== */

/**
 * @brief  Initialize GPT hardware (reset all channels)
 * @return E_OK always
 */
Std_ReturnType Gpt_Hw_Init(void)
{
    uint8 i;
    for (i = 0u; i < GPT_HW_MAX_CHANNELS; i++) {
        gpt_channels[i].running       = FALSE;
        gpt_channels[i].target_us     = 0u;
        gpt_channels[i].start_tick_ms = 0u;
    }
    return E_OK;
}

/**
 * @brief  Start a timer channel
 * @param  Channel  Channel index (0..3)
 * @param  Value    Target value in microseconds
 * @return E_OK on success, E_NOT_OK on invalid channel
 */
Std_ReturnType Gpt_Hw_StartTimer(uint8 Channel, uint32 Value)
{
    if (Channel >= GPT_HW_MAX_CHANNELS) {
        return E_NOT_OK;
    }

    gpt_channels[Channel].running       = TRUE;
    gpt_channels[Channel].target_us     = Value;
    gpt_channels[Channel].start_tick_ms = HAL_GetTick();

    return E_OK;
}

/**
 * @brief  Stop a timer channel
 * @param  Channel  Channel index (0..3)
 * @return E_OK on success, E_NOT_OK on invalid channel
 */
Std_ReturnType Gpt_Hw_StopTimer(uint8 Channel)
{
    if (Channel >= GPT_HW_MAX_CHANNELS) {
        return E_NOT_OK;
    }

    gpt_channels[Channel].running = FALSE;
    return E_OK;
}

/**
 * @brief  Get elapsed time on a timer channel (microseconds)
 * @param  Channel  Channel index (0..3)
 * @return Elapsed microseconds; 0 if stopped or invalid
 *
 * @note   Resolution is 1ms (1000us steps) due to HAL SysTick.
 *         For sub-ms timing, use DWT cycle counter instead.
 */
uint32 Gpt_Hw_GetCounter(uint8 Channel)
{
    uint32 now_ms;
    uint32 elapsed_ms;

    if (Channel >= GPT_HW_MAX_CHANNELS) {
        return 0u;
    }
    if (gpt_channels[Channel].running == FALSE) {
        return 0u;
    }

    now_ms = HAL_GetTick();
    elapsed_ms = now_ms - gpt_channels[Channel].start_tick_ms;

    return elapsed_ms * 1000u; /* Convert ms to us */
}
