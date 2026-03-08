/**
 * @file    Swc_FzcSensorFeeder.c
 * @brief   FZC sensor feeder — plant-sim virtual sensors → MCAL injection
 * @date    2026-03-03
 *
 * @details  SIL-only module. Reads virtual sensor values from Com RX
 *           (CAN 0x400, sent by plant-sim at 10ms) and injects them into
 *           MCAL POSIX stubs so that existing SWC fault detection code
 *           (Swc_Steering, Swc_Brake) receives realistic physics data.
 *
 *           Injection targets:
 *           - Steering angle → Spi_Posix_InjectAngle() → IoHwAb_ReadSteeringAngle
 *           - Brake position → Adc_Posix_InjectValue() → IoHwAb_ReadBrakePosition
 *
 *           Entire module compiled out on real hardware (PLATFORM_POSIX guard).
 *
 *           HIL mode (PLATFORM_HIL): same Com RX path but injects via
 *           IoHwAb_Hil_SetOverride() in engineering units — no reverse-scaling.
 *
 * @safety_req SWR-BSW-014 (IoHwAb injection path)
 * @traces_to  TSR-030, TSR-031
 *
 * @standard AUTOSAR SWC pattern
 * @copyright Taktflow Systems 2026
 */

#include "Swc_FzcSensorFeeder.h"
#include "Fzc_Cfg.h"

#ifdef PLATFORM_POSIX

#include "Com.h"

/* MCAL injection externs (POSIX only — no header file) */
extern void Spi_Posix_InjectAngle(uint16 angle);
extern void Adc_Posix_InjectValue(uint8 Group, uint8 Channel, uint16 Value);

/* No state variable needed — zero-substitution is stateless (see MainFunction) */

#endif /* PLATFORM_POSIX */

#ifdef PLATFORM_HIL

#include "Com.h"
#include "IoHwAb.h"

#endif /* PLATFORM_HIL */

/* ==================================================================
 * API: Swc_FzcSensorFeeder_Init
 * ================================================================== */

void Swc_FzcSensorFeeder_Init(void)
{
#ifdef PLATFORM_POSIX
    /* Inject safe center values immediately so that SWC fault detection
     * sees nominal sensor readings from the very first cycle — before
     * plant-sim has started sending CAN 0x400.
     * Center steering: 8191 = 14-bit midpoint → 0° actual angle.
     * Brake: 0 raw ADC = no brake applied (matches idle cmd=0). */
    Spi_Posix_InjectAngle(8191u);
    Adc_Posix_InjectValue(FZC_BRAKE_ADC_GROUP, FZC_BRAKE_ADC_CHANNEL, 0u);
#endif

#ifdef PLATFORM_HIL
    /* HIL: inject nominal defaults via IoHwAb override so SWCs see safe
     * sensor readings before the Pi rest-bus starts sending CAN 0x400. */
    IoHwAb_Hil_SetOverride(IOHWAB_HIL_CH_STEERING, 8191u);  /* 14-bit center = 0 deg */
    IoHwAb_Hil_SetOverride(IOHWAB_HIL_CH_BRAKE, 0u);        /* no brake */
#endif
}

/* ==================================================================
 * API: Swc_FzcSensorFeeder_MainFunction (10ms cyclic)
 * ================================================================== */

void Swc_FzcSensorFeeder_MainFunction(void)
{
#ifdef PLATFORM_POSIX
    uint32 steer_angle = 0u;
    uint32 brake_pos   = 0u;

    /* Read virtual sensor signals from Com RX (CAN 0x400 from plant-sim) */
    (void)Com_ReceiveSignal(FZC_COM_SIG_RX_VIRT_STEER_ANGLE, &steer_angle);
    (void)Com_ReceiveSignal(FZC_COM_SIG_RX_VIRT_BRAKE_POS,   &brake_pos);

    /* Substitute center when steer_angle is 0 (14-bit raw 0 = -45° full lock).
     * steer_angle==0 occurs in two scenarios:
     *   (a) Com shadow buffer initial state (no CAN 0x400 received yet)
     *   (b) Com RX deadline timeout zeroed the shadow buffer (plant-sim
     *       stopped during container restart — CAN 0x400 gap > 100ms)
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

    /* Inject steering angle into SPI POSIX stub. */
    Spi_Posix_InjectAngle((uint16)steer_angle);

    /* Inject brake position into ADC POSIX stub.
     * Plant-sim sends 0-1000 (ADC counts). IoHwAb scales: (raw*1000)/4095.
     * Reverse-scale: raw = (adc_counts * 4095) / 1000. */
    {
        uint16 raw_adc = (uint16)(((uint32)brake_pos * 4095u) / 1000u);
        Adc_Posix_InjectValue(FZC_BRAKE_ADC_GROUP, FZC_BRAKE_ADC_CHANNEL, raw_adc);
    }
#endif /* PLATFORM_POSIX */

#ifdef PLATFORM_HIL
    {
        uint32 steer_angle = 0u;
        uint32 brake_pos   = 0u;

        (void)Com_ReceiveSignal(FZC_COM_SIG_RX_VIRT_STEER_ANGLE, &steer_angle);
        (void)Com_ReceiveSignal(FZC_COM_SIG_RX_VIRT_BRAKE_POS,   &brake_pos);

        /* Same zero-substitution logic as POSIX path */
        if (steer_angle == 0u)
        {
            steer_angle = 8191u;
        }

        /* HIL: inject engineering-unit values directly into IoHwAb overrides.
         * No reverse-scaling needed — override bypasses the ADC-to-eng conversion. */
        IoHwAb_Hil_SetOverride(IOHWAB_HIL_CH_STEERING, steer_angle);
        IoHwAb_Hil_SetOverride(IOHWAB_HIL_CH_BRAKE, brake_pos);
    }
#endif /* PLATFORM_HIL */
}
