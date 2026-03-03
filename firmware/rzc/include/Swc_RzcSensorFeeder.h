/**
 * @file    Swc_RzcSensorFeeder.h
 * @brief   RZC sensor feeder — injects virtual sensor data into MCAL (SIL only)
 * @date    2026-03-03
 *
 * @details  Reads virtual sensor CAN signals (0x401) from Com and injects
 *           values into MCAL ADC stubs so that existing SWC fault detection
 *           (CurrentMonitor, TempMonitor, Battery) works identically to real
 *           hardware.  Entire module is compiled out on real hardware builds
 *           via #ifdef PLATFORM_POSIX.
 *
 * @standard AUTOSAR SWC pattern
 * @copyright Taktflow Systems 2026
 */
#ifndef SWC_RZC_SENSOR_FEEDER_H
#define SWC_RZC_SENSOR_FEEDER_H

/**
 * @brief  Initialize the RZC sensor feeder
 */
void Swc_RzcSensorFeeder_Init(void);

/**
 * @brief  Cyclic main function — reads virtual sensor Com signals,
 *         injects into MCAL ADC stubs.  Must run BEFORE motor/battery SWCs.
 */
void Swc_RzcSensorFeeder_MainFunction(void);

#endif /* SWC_RZC_SENSOR_FEEDER_H */
