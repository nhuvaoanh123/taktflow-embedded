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

#endif /* PLATFORM_POSIX */

/* ==================================================================
 * API: Swc_FzcSensorFeeder_Init
 * ================================================================== */

void Swc_FzcSensorFeeder_Init(void)
{
    /* Stateless — no initialization needed */
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

    /* Inject steering angle into SPI POSIX stub.
     * IoHwAb_ReadSteeringAngle() -> Spi_SyncTransmit() -> Spi_Hw_Transmit()
     * will return this injected value (with small oscillation for stuck
     * detection bypass). */
    Spi_Posix_InjectAngle((uint16)steer_angle);

    /* Inject brake position into ADC POSIX stub.
     * IoHwAb_ReadBrakePosition() -> iohwab_read_adc(group 3) -> Adc_ReadGroup()
     * will return this injected value.
     * Plant-sim sends 0-1000 (ADC counts). IoHwAb scales: (raw*1000)/4095.
     * We need to reverse-scale: raw = (adc_counts * 4095) / 1000. */
    {
        uint16 raw_adc = (uint16)(((uint32)brake_pos * 4095u) / 1000u);
        Adc_Posix_InjectValue(FZC_BRAKE_ADC_GROUP, FZC_BRAKE_ADC_CHANNEL, raw_adc);
    }
#endif /* PLATFORM_POSIX */
}
