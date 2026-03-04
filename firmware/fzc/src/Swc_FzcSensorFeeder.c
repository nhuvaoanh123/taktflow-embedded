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

/** @brief  Gate: hold center defaults until plant-sim sends real data.
 *          Com shadow buffer initializes to 0; steer_angle=0 maps to
 *          -45° actual which triggers false steering plausibility fault.
 *          Any non-zero steer_angle means plant-sim has started. */
static uint8 SensorFeeder_DataValid;

#endif /* PLATFORM_POSIX */

/* ==================================================================
 * API: Swc_FzcSensorFeeder_Init
 * ================================================================== */

void Swc_FzcSensorFeeder_Init(void)
{
#ifdef PLATFORM_POSIX
    SensorFeeder_DataValid = 0u;
    /* Inject safe center values immediately so that SWC fault detection
     * sees nominal sensor readings from the very first cycle — before
     * plant-sim has started sending CAN 0x400.
     * Center steering: 8191 = 14-bit midpoint → 0° actual angle.
     * Brake: 0 raw ADC = no brake applied (matches idle cmd=0).
     * MainFunction will continue holding these defaults until
     * SensorFeeder_DataValid flips to 1 (first non-zero steer_angle). */
    Spi_Posix_InjectAngle(8191u);
    Adc_Posix_InjectValue(FZC_BRAKE_ADC_GROUP, FZC_BRAKE_ADC_CHANNEL, 0u);
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

    /* Hold center defaults until plant-sim sends real data on CAN 0x400.
     * Com shadow buffer defaults to 0.  Plant-sim sends steer_angle=8191
     * (center) in idle — any non-zero value means real data arrived.
     * Edge case: plant-sim sends raw 0 (-45° full lock) — held as default
     * until a subsequent non-zero frame, which is correct because -45°
     * at boot is physically impossible (vehicle starts centered). */
    if (SensorFeeder_DataValid == 0u)
    {
        if (steer_angle != 0u)
        {
            SensorFeeder_DataValid = 1u;
        }
        else
        {
            steer_angle = 8191u; /* 14-bit center = 0° steering */
            /* brake_pos 0 = no brake — already correct for idle */
        }
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
}
