/**
 * @file    Swc_FzcSensorFeeder.h
 * @brief   FZC sensor feeder — bridges plant-sim virtual sensors to MCAL injection
 * @date    2026-03-03
 *
 * @details  SIL-only module (PLATFORM_POSIX). Reads virtual sensor CAN data
 *           from Com RX (CAN 0x400, sent by plant-sim) and injects values into
 *           MCAL stubs (Spi_Posix, Adc_Posix) so that SWC fault detection
 *           reads realistic physics data instead of hardcoded "OK" values.
 *
 *           Run BEFORE Swc_Steering_MainFunction and Swc_Brake_MainFunction
 *           so injected values are available when SWCs read sensors.
 *
 * @safety_req SWR-BSW-014 (IoHwAb injection path)
 * @traces_to  TSR-030, TSR-031
 *
 * @standard AUTOSAR SWC pattern
 * @copyright Taktflow Systems 2026
 */
#ifndef SWC_FZC_SENSOR_FEEDER_H
#define SWC_FZC_SENSOR_FEEDER_H

/**
 * @brief  Initialize FZC sensor feeder (no-op, stateless)
 */
void Swc_FzcSensorFeeder_Init(void);

/**
 * @brief  Main function — reads virtual sensor CAN, injects into MCAL
 *         10ms cyclic, must run BEFORE steering and brake SWCs
 */
void Swc_FzcSensorFeeder_MainFunction(void);

#endif /* SWC_FZC_SENSOR_FEEDER_H */
