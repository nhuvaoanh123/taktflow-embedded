/**
 * @file    Swc_RzcSensorFeeder.c
 * @brief   RZC sensor feeder — injects virtual sensor data into MCAL (SIL only)
 * @date    2026-03-03
 *
 * @details  Reads virtual sensor CAN signals from plant-sim (CAN 0x401) via
 *           Com_ReceiveSignal and injects values into MCAL ADC stubs using
 *           Adc_Posix_InjectValue().  This bridges the gap between plant-sim
 *           physics and firmware sensor APIs in SIL mode.
 *
 *           Signal mapping (CAN 0x401 -> ADC injection):
 *           - motor_current  (bytes 0-1) -> ADC group 0, ch 0
 *           - motor_temp     (bytes 2-3) -> ADC group 1, ch 0
 *           - battery_voltage (bytes 4-5) -> ADC group 2, ch 0
 *
 *           On real hardware this module compiles to empty stubs
 *           (entire logic guarded by #ifdef PLATFORM_POSIX).
 *
 * @standard AUTOSAR SWC pattern
 * @copyright Taktflow Systems 2026
 */

#include "Swc_RzcSensorFeeder.h"
#include "Rzc_Cfg.h"
#include "Com.h"

#ifdef PLATFORM_POSIX
/* MCAL injection API — only exists in POSIX stub builds */
extern void Adc_Posix_InjectValue(uint8 Group, uint8 Channel, uint16 Value);
#endif

/* ==================================================================
 * Module State
 * ================================================================== */

static uint8 SensorFeeder_Initialized;

/* ==================================================================
 * API: Swc_RzcSensorFeeder_Init
 * ================================================================== */

void Swc_RzcSensorFeeder_Init(void)
{
    SensorFeeder_Initialized = 1u;
}

/* ==================================================================
 * API: Swc_RzcSensorFeeder_MainFunction (10ms cyclic)
 * ================================================================== */

void Swc_RzcSensorFeeder_MainFunction(void)
{
#ifdef PLATFORM_POSIX
    uint32 motor_current;
    uint32 motor_temp;
    uint32 battery_voltage;

    if (SensorFeeder_Initialized != 1u)
    {
        return;
    }

    motor_current   = 0u;
    motor_temp      = 0u;
    battery_voltage = 0u;

    /* Read virtual sensor signals from Com (populated by CAN 0x401 RX) */
    (void)Com_ReceiveSignal(RZC_COM_SIG_RX_VIRT_MOTOR_CURRENT,   &motor_current);
    (void)Com_ReceiveSignal(RZC_COM_SIG_RX_VIRT_MOTOR_TEMP,      &motor_temp);
    (void)Com_ReceiveSignal(RZC_COM_SIG_RX_VIRT_BATTERY_VOLTAGE, &battery_voltage);

    /* Inject into MCAL ADC stubs.
     * Plant-sim sends raw values in engineering units (mA, 0.1C, mV).
     * ADC stubs store 16-bit values that IoHwAb reads and scales.
     * No reverse-scaling needed here — IoHwAb handles ADC-to-engineering
     * conversion, and the injected value IS the ADC reading. */
    Adc_Posix_InjectValue(RZC_MOTOR_CURRENT_ADC_GROUP,
                          RZC_MOTOR_CURRENT_ADC_CH,
                          (uint16)motor_current);

    Adc_Posix_InjectValue(RZC_MOTOR_TEMP_ADC_GROUP,
                          RZC_MOTOR_TEMP_ADC_CH,
                          (uint16)motor_temp);

    Adc_Posix_InjectValue(RZC_BATTERY_VOLTAGE_ADC_GROUP,
                          RZC_BATTERY_VOLTAGE_ADC_CH,
                          (uint16)battery_voltage);
#else
    (void)SensorFeeder_Initialized;
#endif
}
