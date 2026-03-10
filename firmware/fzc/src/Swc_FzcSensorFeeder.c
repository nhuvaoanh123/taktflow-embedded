/**
 * @file    Swc_FzcSensorFeeder.c
 * @brief   FZC sensor feeder — plant-sim virtual sensors → IoHwAb injection
 * @date    2026-03-03
 *
 * @details  SIL/HIL-only module. Reads virtual sensor values from Com RX
 *           (CAN 0x600, sent by plant-sim at 10ms) and injects them into
 *           IoHwAb via the unified injection API (IoHwAb_Inject.h).
 *
 *           Injection targets:
 *           - Steering angle → IoHwAb_Inject_SetSensorValue → IoHwAb_ReadSteeringAngle
 *           - Brake position → IoHwAb_Inject_SetSensorValue → IoHwAb_ReadBrakePosition
 *
 *           This file is NOT compiled on target hardware. On POSIX it links
 *           against IoHwAb_Posix.c, on HIL against IoHwAb_Hil.c — the unified
 *           IoHwAb_Inject API is implemented by both. Zero #ifdef PLATFORM_*.
 *
 * @safety_req SWR-BSW-014 (IoHwAb injection path)
 * @traces_to  TSR-030, TSR-031
 *
 * @standard AUTOSAR SWC pattern
 * @copyright Taktflow Systems 2026
 */

#include "Swc_FzcSensorFeeder.h"
#include "Fzc_Cfg.h"
#include "Com.h"
#include "IoHwAb_Inject.h"

/* ==================================================================
 * API: Swc_FzcSensorFeeder_Init
 * ================================================================== */

void Swc_FzcSensorFeeder_Init(void)
{
    /* Inject safe center values immediately so that SWC fault detection
     * sees nominal sensor readings from the very first cycle — before
     * plant-sim has started sending CAN 0x600.
     * Center steering: 8191 = 14-bit midpoint → 0° actual angle.
     * Brake: 0 = no brake applied (matches idle cmd=0). */
    IoHwAb_Inject_SetSensorValue(IOHWAB_INJECT_STEERING, 8191u);
    IoHwAb_Inject_SetSensorValue(IOHWAB_INJECT_BRAKE_POSITION, 0u);
}

/* ==================================================================
 * API: Swc_FzcSensorFeeder_MainFunction (10ms cyclic)
 * ================================================================== */

void Swc_FzcSensorFeeder_MainFunction(void)
{
    uint32 steer_angle = 0u;
    uint32 brake_pos   = 0u;

    /* Read virtual sensor signals from Com RX (CAN 0x600 from plant-sim) */
    (void)Com_ReceiveSignal(FZC_COM_SIG_RX_VIRT_STEER_ANGLE, &steer_angle);
    (void)Com_ReceiveSignal(FZC_COM_SIG_RX_VIRT_BRAKE_POS,   &brake_pos);

    /* Substitute center when steer_angle is 0 (14-bit raw 0 = -45° full lock).
     * steer_angle==0 occurs in two scenarios:
     *   (a) Com shadow buffer initial state (no CAN 0x600 received yet)
     *   (b) Com RX deadline timeout zeroed the shadow buffer (plant-sim
     *       stopped during container restart — CAN 0x600 gap > 100ms)
     * In both cases, injecting center (8191 = 0°) is safe because:
     *   - At boot/restart the vehicle starts centered (never at -45°)
     *   - Injecting 0 would trigger false steering plausibility fault
     * Note: plant-sim encodes -45° as raw 0, but that physical state is
     * impossible at boot.  During normal operation plant-sim sends >0. */
    if (steer_angle == 0u)
    {
        steer_angle = 8191u; /* 14-bit center = 0° steering */
        /* brake_pos 0 = no brake — already correct for idle */
    }

    /* Inject engineering-unit values directly via unified inject API.
     * No reverse-scaling needed — IoHwAb_Posix returns injected values
     * as-is, and IoHwAb_Hil maps to override channels internally. */
    IoHwAb_Inject_SetSensorValue(IOHWAB_INJECT_STEERING,
                                  (uint16)steer_angle);
    IoHwAb_Inject_SetSensorValue(IOHWAB_INJECT_BRAKE_POSITION,
                                  (uint16)brake_pos);
}
