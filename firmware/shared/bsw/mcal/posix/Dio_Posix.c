/**
 * @file    Dio_Posix.c
 * @brief   POSIX DIO stub — implements Dio_Hw_* externs from Dio.h
 * @date    2026-02-23
 *
 * @details Simulated digital I/O using a static array of channel values.
 *          No real GPIO hardware — channels are software-emulated for
 *          simulated ECUs (BCM, ICU, TCU) running in Docker containers.
 *          Provides injection API for SIL testing of sensor inputs.
 *
 * @safety_req SWR-BSW-009: DIO Driver for Digital I/O
 * @traces_to  SYS-050, TSR-005, TSR-033
 *
 * @copyright Taktflow Systems 2026
 */

#include "Platform_Types.h"
#include "Std_Types.h"

/* ---- Module state ---- */

#define DIO_POSIX_MAX_CHANNELS  256u

static uint8 dio_channel_state[DIO_POSIX_MAX_CHANNELS];

/* ---- Dio_Hw_* implementations ---- */

/**
 * @brief  Read pin level from simulated DIO channel
 * @param  ChannelId  Channel index (0..255)
 * @return STD_HIGH or STD_LOW
 */
uint8 Dio_Hw_ReadPin(uint8 ChannelId)
{
    /* uint8 range 0..255 fits in 256-element array — no bounds check needed */
    return dio_channel_state[ChannelId];
}

/**
 * @brief  Write pin level to simulated DIO channel
 * @param  ChannelId  Channel index (0..255)
 * @param  Level      STD_HIGH or STD_LOW
 */
void Dio_Hw_WritePin(uint8 ChannelId, uint8 Level)
{
    /* uint8 range 0..255 fits in 256-element array — no bounds check needed */
    dio_channel_state[ChannelId] = (Level != STD_LOW) ? STD_HIGH : STD_LOW;
}

/* ---- SIL injection / readback API (plant simulator + test harness) ---- */

/**
 * @brief  Inject a DIO pin level (simulates external input signal)
 * @param  ChannelId  Channel index (0..255)
 * @param  Level      STD_HIGH or STD_LOW
 *
 * @details Allows the plant simulator or test harness to inject external
 *          sensor states (e.g. E-stop switch, limit switch, encoder pulse)
 *          that the ECU application reads via Dio_ReadChannel().
 *          Functionally identical to Dio_Hw_WritePin but named explicitly
 *          for test clarity and discovery.
 */
void Dio_Posix_InjectPin(uint8 ChannelId, uint8 Level)
{
    dio_channel_state[ChannelId] = (Level != STD_LOW) ? STD_HIGH : STD_LOW;
}

/**
 * @brief  Read a DIO pin level (observe ECU output state)
 * @param  ChannelId  Channel index (0..255)
 * @return STD_HIGH or STD_LOW
 *
 * @details Allows the plant simulator or test harness to observe what
 *          the ECU application has written to output pins (e.g. motor
 *          enable, relay control, LED, buzzer). Named explicitly for
 *          test clarity and discovery.
 */
uint8 Dio_Posix_ReadPin(uint8 ChannelId)
{
    return dio_channel_state[ChannelId];
}

/**
 * @brief  Reset all DIO channels to STD_LOW
 *
 * @details Convenience function for test setup — clears all channel
 *          state to a known baseline before running test scenarios.
 */
void Dio_Posix_ResetAll(void)
{
    uint16 i;
    for (i = 0u; i < DIO_POSIX_MAX_CHANNELS; i++) {
        dio_channel_state[i] = STD_LOW;
    }
}
