/**
 * @file    Icu_Cfg.h
 * @brief   ICU configuration — all ICU-specific ID definitions
 * @date    2026-02-23
 *
 * @details Unified configuration header for the Instrument Cluster Unit.
 *          Contains RTE signal IDs, Com RX PDU IDs, temperature/battery
 *          zone thresholds, heartbeat timeout, and ncurses color pairs.
 *
 *          ICU is a listen-only consumer — no TX PDU IDs.
 *
 * @safety_req SWR-ICU-001 to SWR-ICU-010
 * @traces_to  SSR-ICU-001 to SSR-ICU-010, TSR-022, TSR-046
 *
 * @standard AUTOSAR, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#ifndef ICU_CFG_H
#define ICU_CFG_H

/* ====================================================================
 * RTE Signal IDs (extends BSW well-known IDs at offset 16)
 * ==================================================================== */

#define ICU_SIG_MOTOR_RPM         16u
#define ICU_SIG_TORQUE_PCT        17u
#define ICU_SIG_MOTOR_TEMP        18u
#define ICU_SIG_BATTERY_VOLTAGE   19u
#define ICU_SIG_VEHICLE_STATE     20u
#define ICU_SIG_ESTOP_ACTIVE      21u
#define ICU_SIG_HEARTBEAT_CVC     22u
#define ICU_SIG_HEARTBEAT_FZC     23u
#define ICU_SIG_HEARTBEAT_RZC     24u
#define ICU_SIG_OVERCURRENT_FLAG  25u
#define ICU_SIG_LIGHT_STATUS      26u
#define ICU_SIG_INDICATOR_STATE   27u
#define ICU_SIG_DTC_BROADCAST     28u
#define ICU_SIG_COUNT             29u

/* ====================================================================
 * Com RX PDU IDs (ICU is listen-only, no TX)
 * ==================================================================== */

#define ICU_COM_RX_ESTOP          0u   /* CAN 0x001 */
#define ICU_COM_RX_HB_CVC         1u   /* CAN 0x010 */
#define ICU_COM_RX_HB_FZC         2u   /* CAN 0x011 */
#define ICU_COM_RX_HB_RZC         3u   /* CAN 0x012 */
#define ICU_COM_RX_VEHICLE_STATE  4u   /* CAN 0x100 */
#define ICU_COM_RX_TORQUE_REQ     5u   /* CAN 0x101 */
#define ICU_COM_RX_MOTOR_CURRENT  6u   /* CAN 0x301 */
#define ICU_COM_RX_MOTOR_TEMP     7u   /* CAN 0x302 */
#define ICU_COM_RX_BATTERY        8u   /* CAN 0x303 */
#define ICU_COM_RX_LIGHT_STATUS   9u   /* CAN 0x400 */
#define ICU_COM_RX_INDICATOR     10u   /* CAN 0x401 */
#define ICU_COM_RX_DOOR_LOCK     11u   /* CAN 0x402 */
#define ICU_COM_RX_DTC_BCAST     12u   /* CAN 0x500 */

#define ICU_COM_RX_PDU_COUNT     13u

/* ====================================================================
 * Temperature Zone Thresholds (degrees C)
 * ==================================================================== */

#define ICU_TEMP_GREEN_MAX        59u   /* GREEN: 0-59    */
#define ICU_TEMP_YELLOW_MAX       79u   /* YELLOW: 60-79  */
#define ICU_TEMP_ORANGE_MAX       99u   /* ORANGE: 80-99  */
                                        /* RED: >= 100    */

/* ====================================================================
 * Battery Voltage Zone Thresholds (millivolts)
 * ==================================================================== */

#define ICU_BATT_GREEN_MIN      11000u  /* GREEN: > 11000 */
#define ICU_BATT_YELLOW_MIN     10000u  /* YELLOW: 10000-11000 */
                                        /* RED: < 10000   */

/* ====================================================================
 * Heartbeat Timeout (ticks at 50ms cycle)
 * ==================================================================== */

#define ICU_HB_TIMEOUT_TICKS       4u   /* 200ms at 50ms tick */

/* ====================================================================
 * ncurses Color Pair IDs
 * ==================================================================== */

#define ICU_COLOR_GREEN    1u
#define ICU_COLOR_YELLOW   2u
#define ICU_COLOR_ORANGE   3u
#define ICU_COLOR_RED      4u
#define ICU_COLOR_WHITE    5u

#endif /* ICU_CFG_H */
