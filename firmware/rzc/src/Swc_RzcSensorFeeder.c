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

/** @brief  Gate: hold nominal defaults until plant-sim sends real data.
 *          Com shadow buffer initializes to 0; battery_voltage=0 triggers
 *          false undervoltage fault (threshold = RZC_BATT_DISABLE_LOW_MV).
 *          Any non-zero battery_voltage means plant-sim has started. */
static uint8 SensorFeeder_DataValid;
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

#ifdef PLATFORM_POSIX
    SensorFeeder_DataValid = 0u;
    /* Inject nominal defaults before plant-sim starts sending 0x401.
     * Motor current=0, motor temp=0 are safe (no overcurrent/overtemp).
     * Battery voltage needs nominal value to prevent false undervoltage
     * (RZC_BATT_DISABLE_LOW_MV = 8000 mV).
     * Nominal: 12600 mV.  Reverse-scale: 12600 * 4095 / 13200 = 3909 raw ADC. */
    Adc_Posix_InjectValue(RZC_BATTERY_VOLTAGE_ADC_GROUP,
                          RZC_BATTERY_VOLTAGE_ADC_CH,
                          3909u);
#endif
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

    /* Hold nominal defaults until plant-sim sends real data on CAN 0x401.
     * Com shadow buffer defaults to 0.  Plant-sim sends battery_voltage in
     * mV (nominal ~12600) — any non-zero value means real data arrived. */
    if (SensorFeeder_DataValid == 0u)
    {
        if (battery_voltage != 0u)
        {
            SensorFeeder_DataValid = 1u;
        }
        else
        {
            battery_voltage = 12600u; /* nominal 12.6V in mV */
            /* motor_current=0, motor_temp=0 are safe (idle) */
        }
    }

    /* Inject into MCAL ADC stubs with reverse-scaling.
     * Plant-sim sends engineering units; IoHwAb reads raw ADC and
     * scales to engineering units.  We must reverse the IoHwAb
     * formulas so the round-trip produces the correct value.
     *
     * IoHwAb formulas (VREF=3300, ADC_MAX=4095):
     *   Current_mA  = raw * 3300 / 4095     → raw = mA  * 4095 / 3300
     *   Temp_dC     = raw * 1000 / 4095     → raw = dC  * 4095 / 1000
     *   Voltage_mV  = raw * 3300 * 4 / 4095 → raw = mV  * 4095 / 13200
     */
    {
        uint16 raw_current = (uint16)(((uint32)motor_current * 4095u) / 3300u);
        uint16 raw_temp    = (uint16)(((uint32)motor_temp    * 4095u) / 1000u);
        uint16 raw_batt    = (uint16)(((uint32)battery_voltage * 4095u) / 13200u);

        Adc_Posix_InjectValue(RZC_MOTOR_CURRENT_ADC_GROUP,
                              RZC_MOTOR_CURRENT_ADC_CH,
                              raw_current);

        Adc_Posix_InjectValue(RZC_MOTOR_TEMP_ADC_GROUP,
                              RZC_MOTOR_TEMP_ADC_CH,
                              raw_temp);

        Adc_Posix_InjectValue(RZC_BATTERY_VOLTAGE_ADC_GROUP,
                              RZC_BATTERY_VOLTAGE_ADC_CH,
                              raw_batt);
    }
#else
    (void)SensorFeeder_Initialized;
#endif
}
