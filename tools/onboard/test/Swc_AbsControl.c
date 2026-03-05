/**
 * @file    Swc_AbsControl.c
 * @brief   Minimal test SWC — ABS control stub for onboarding layer E2E test
 *
 * @details Reads wheel speed from RTE, writes brake command to RTE.
 *          This is a stub — real customer code would have actual ABS logic.
 */
#include "Swc_AbsControl.h"
#include "Rte.h"

/* Use the generated signal IDs — they'll be available via <Ecu>_Cfg.h
 * which is included by the generated main.c before this SWC. */
#ifndef ABS_SIG_WHEEL_SPEED_FL
#define ABS_SIG_WHEEL_SPEED_FL  16u
#endif
#ifndef ABS_SIG_ABS_BRAKE_CMD_FL
#define ABS_SIG_ABS_BRAKE_CMD_FL  20u
#endif

static uint8 abs_initialized;

void Swc_AbsControl_Init(void)
{
    abs_initialized = 1u;
}

void Swc_AbsControl_MainFunction(void)
{
    uint32 wheel_speed = 0u;
    uint32 brake_cmd = 0u;

    if (abs_initialized == 0u) {
        return;
    }

    /* Read wheel speed */
    (void)Rte_Read(ABS_SIG_WHEEL_SPEED_FL, &wheel_speed);

    /* Simple stub: if wheel speed > 100, apply braking */
    if (wheel_speed > 100u) {
        brake_cmd = 50u;
    }

    /* Write brake command */
    (void)Rte_Write(ABS_SIG_ABS_BRAKE_CMD_FL, brake_cmd);
}
