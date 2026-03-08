/**
 * @file    Fzc_Cfg.h
 * @brief   FZC configuration — all FZC-specific ID definitions
 * @date    2026-02-23
 *
 * @details Unified configuration header for the Front Zone Controller.
 *          Contains RTE signal IDs, Com PDU IDs, DTC event IDs, E2E data IDs,
 *          steering constants, brake constants, lidar constants, buzzer patterns,
 *          heartbeat constants, and self-test constants.
 *
 * @safety_req SWR-FZC-001 to SWR-FZC-032
 * @traces_to  SSR-FZC-001 to SSR-FZC-024, TSR-022, TSR-030, TSR-038, TSR-046
 *
 * @standard AUTOSAR, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#ifndef FZC_CFG_H
#define FZC_CFG_H

/* ====================================================================
 * RTE Signal IDs (extends BSW well-known IDs at offset 16)
 * ==================================================================== */

#define FZC_SIG_STEER_CMD          16u
#define FZC_SIG_STEER_ANGLE        17u
#define FZC_SIG_STEER_FAULT        18u
#define FZC_SIG_BRAKE_CMD          19u
#define FZC_SIG_BRAKE_POS          20u
#define FZC_SIG_BRAKE_FAULT        21u
#define FZC_SIG_LIDAR_DIST         22u
#define FZC_SIG_LIDAR_SIGNAL       23u
#define FZC_SIG_LIDAR_ZONE         24u
#define FZC_SIG_LIDAR_FAULT        25u
#define FZC_SIG_VEHICLE_STATE      26u
#define FZC_SIG_ESTOP_ACTIVE       27u
#define FZC_SIG_BUZZER_PATTERN     28u
#define FZC_SIG_MOTOR_CUTOFF       29u
#define FZC_SIG_FAULT_MASK         30u
#define FZC_SIG_STEER_PWM_DISABLE  31u
#define FZC_SIG_BRAKE_PWM_DISABLE  32u
#define FZC_SIG_SELF_TEST_RESULT   33u
#define FZC_SIG_HEARTBEAT_ALIVE    34u
#define FZC_SIG_SAFETY_STATUS      35u
#define FZC_SIG_COUNT              36u

/* ====================================================================
 * Com TX PDU IDs
 * ==================================================================== */

#define FZC_COM_TX_HEARTBEAT       0u   /* CAN 0x011 */
#define FZC_COM_TX_STEER_STATUS    1u   /* CAN 0x200 */
#define FZC_COM_TX_BRAKE_STATUS    2u   /* CAN 0x201 */
#define FZC_COM_TX_BRAKE_FAULT     3u   /* CAN 0x210 */
#define FZC_COM_TX_MOTOR_CUTOFF    4u   /* CAN 0x211 */
#define FZC_COM_TX_LIDAR           5u   /* CAN 0x220 */
#define FZC_COM_TX_DTC_BROADCAST   6u   /* CAN 0x500 — DTC broadcast */

/* ====================================================================
 * Com TX Signal IDs (index into Com signal config table)
 * NOTE: These are SIGNAL IDs, not PDU IDs. Com_SendSignal() takes a
 * signal ID.  The PDU IDs above are for PduR_Transmit() / CanIf.
 * ==================================================================== */

#define FZC_COM_SIG_TX_BRAKE_FAULT     6u   /* sig_tx_brake_fault  in Com_Cfg_Fzc.c */
#define FZC_COM_SIG_TX_MOTOR_CUTOFF    7u   /* sig_tx_motor_cutoff in Com_Cfg_Fzc.c */

/* ====================================================================
 * Com RX PDU IDs
 * ==================================================================== */

#define FZC_COM_RX_ESTOP           0u   /* CAN 0x001 */
#define FZC_COM_RX_VEHICLE_STATE   1u   /* CAN 0x100 */
#define FZC_COM_RX_STEER_CMD       2u   /* CAN 0x102 */
#define FZC_COM_RX_BRAKE_CMD       3u   /* CAN 0x103 */
#define FZC_COM_RX_VIRT_SENSORS    4u   /* CAN 0x600 — virtual sensors from plant-sim */

/* ====================================================================
 * Com Signal IDs for Virtual Sensors (RX from plant-sim, SIL only)
 * ==================================================================== */

#define FZC_COM_SIG_RX_VIRT_STEER_ANGLE   14u  /* uint16 LE, 14-bit SPI format */
#define FZC_COM_SIG_RX_VIRT_BRAKE_POS     15u  /* uint16 LE, 0-1000 ADC counts */
#define FZC_COM_SIG_RX_VIRT_BRAKE_CURRENT 16u  /* uint16 LE, mA               */

/* ADC group/channel for brake position injection (SIL) */
#define FZC_BRAKE_ADC_GROUP    3u   /* Must match iohwab_config.BrakePositionAdcGroup */
#define FZC_BRAKE_ADC_CHANNEL  0u   /* Channel 0 within the group */

/* ====================================================================
 * DTC Event IDs (Dem_EventIdType)
 * ==================================================================== */

#define FZC_DTC_STEER_PLAUSIBILITY    0u   /* 0xD00100 */
#define FZC_DTC_STEER_RANGE           1u   /* 0xD00200 */
#define FZC_DTC_STEER_RATE            2u   /* 0xD00300 */
#define FZC_DTC_STEER_TIMEOUT         3u   /* 0xD00400 */
#define FZC_DTC_STEER_SPI_FAIL        4u   /* 0xD00500 */
#define FZC_DTC_BRAKE_FAULT           5u   /* 0xD10100 */
#define FZC_DTC_BRAKE_TIMEOUT         6u   /* 0xD10200 */
#define FZC_DTC_BRAKE_PWM_FAIL        7u   /* 0xD10300 */
#define FZC_DTC_LIDAR_TIMEOUT         8u   /* 0xD20100 */
#define FZC_DTC_LIDAR_CHECKSUM        9u   /* 0xD20200 */
#define FZC_DTC_LIDAR_STUCK          10u   /* 0xD20300 */
#define FZC_DTC_LIDAR_SIGNAL_LOW     11u   /* 0xD20400 */
#define FZC_DTC_CAN_BUS_OFF          12u   /* 0xD30100 */
#define FZC_DTC_SELF_TEST_FAIL       13u   /* 0xD40100 */
#define FZC_DTC_WATCHDOG_FAIL        14u   /* 0xD40200 */
#define FZC_DTC_BRAKE_OSCILLATION    15u   /* 0xD10400 */

/* ====================================================================
 * E2E Data IDs
 * ==================================================================== */

#define FZC_E2E_HEARTBEAT_DATA_ID    0x03u  /* per CAN message matrix spec */
#define FZC_E2E_STEER_STATUS_DATA_ID 0x20u
#define FZC_E2E_BRAKE_STATUS_DATA_ID 0x21u
#define FZC_E2E_LIDAR_DATA_ID        0x22u
#define FZC_E2E_ESTOP_DATA_ID        0x01u
#define FZC_E2E_VEHSTATE_DATA_ID     0x10u
#define FZC_E2E_STEER_CMD_DATA_ID    0x12u
#define FZC_E2E_BRAKE_CMD_DATA_ID    0x13u

/* ====================================================================
 * Steering Constants (ASIL D)
 * ==================================================================== */

#define FZC_STEER_PLAUS_THRESHOLD_DEG    5u    /* 5 degree command vs feedback */
#define FZC_STEER_PLAUS_DEBOUNCE         5u    /* 5-cycle debounce */
#define FZC_STEER_RATE_LIMIT_DEG_10MS    3u    /* 0.3 deg per 10ms = 3 tenths */
#define FZC_STEER_CMD_TIMEOUT_MS       100u    /* 100ms command timeout */
#define FZC_STEER_RTC_RATE_DEG_S        30u    /* 30 deg/s return-to-center rate */
#define FZC_STEER_ANGLE_MIN           (-45)    /* Minimum steering angle (degrees) */
#define FZC_STEER_ANGLE_MAX             45     /* Maximum steering angle (degrees) */
#define FZC_STEER_PWM_CENTER_US       1500u    /* 1.5ms center = neutral */
#define FZC_STEER_PWM_MIN_US          1000u    /* 1.0ms full left */
#define FZC_STEER_PWM_MAX_US          2000u    /* 2.0ms full right */
#define FZC_STEER_LATCH_CLEAR_CYCLES    50u    /* Fault-free cycles to clear latch */

/* ====================================================================
 * Brake Constants (ASIL D)
 * ==================================================================== */

#define FZC_BRAKE_AUTO_TIMEOUT_MS      100u    /* 100ms auto-brake timeout */
#define FZC_BRAKE_PWM_FAULT_THRESH       2u    /* 2% PWM fault threshold */
#define FZC_BRAKE_FAULT_DEBOUNCE         3u    /* 3-cycle debounce */
#define FZC_BRAKE_CUTOFF_REPEAT_MS      10u    /* Motor cutoff CAN repeat period */
#define FZC_BRAKE_CUTOFF_REPEAT_COUNT   10u    /* Number of cutoff CAN repeats */
#define FZC_BRAKE_PWM_MIN                0u    /* 0% = no brake */
#define FZC_BRAKE_PWM_MAX              100u    /* 100% = full brake */
#define FZC_BRAKE_LATCH_CLEAR_CYCLES    50u    /* Fault-free cycles to clear latch */

/* Brake oscillation detection (ASIL D — command plausibility) */
#define FZC_BRAKE_OSCILLATION_DELTA_THRESH  30u   /* 30% min jump per cycle */
#define FZC_BRAKE_OSCILLATION_DEBOUNCE       4u   /* 4 consecutive = fault (40ms) */

/* ====================================================================
 * Lidar Constants (ASIL C)
 * ==================================================================== */

#define FZC_LIDAR_WARN_CM              100u    /* Warning zone: <= 100cm */
#define FZC_LIDAR_BRAKE_CM              50u    /* Braking zone: <= 50cm */
#define FZC_LIDAR_EMERGENCY_CM          20u    /* Emergency zone: <= 20cm */
#define FZC_LIDAR_TIMEOUT_MS           100u    /* 100ms frame timeout */
#define FZC_LIDAR_STUCK_CYCLES          50u    /* 50 identical readings = stuck */
#define FZC_LIDAR_RANGE_MIN_CM           2u    /* TFMini-S minimum range */
#define FZC_LIDAR_RANGE_MAX_CM        1200u    /* TFMini-S maximum range */
#define FZC_LIDAR_SIGNAL_MIN           100u    /* Minimum signal strength */
#define FZC_LIDAR_FRAME_SIZE             9u    /* TFMini-S frame size bytes */
#define FZC_LIDAR_HEADER_BYTE         0x59u    /* TFMini-S frame header */
#define FZC_LIDAR_DEGRADE_CYCLES       200u    /* Persistent fault cycles before degradation request */

/* Lidar zone enum */
#define FZC_LIDAR_ZONE_CLEAR            0u
#define FZC_LIDAR_ZONE_WARNING          1u
#define FZC_LIDAR_ZONE_BRAKING          2u
#define FZC_LIDAR_ZONE_EMERGENCY        3u
#define FZC_LIDAR_ZONE_FAULT            4u

/* ====================================================================
 * Buzzer Pattern Enum
 * ==================================================================== */

#define FZC_BUZZER_SILENT               0u
#define FZC_BUZZER_SINGLE_BEEP          1u
#define FZC_BUZZER_SLOW_REPEAT          2u
#define FZC_BUZZER_FAST_REPEAT          3u
#define FZC_BUZZER_CONTINUOUS           4u
#define FZC_BUZZER_PATTERN_COUNT        5u

/* ====================================================================
 * RTE Period
 * ==================================================================== */

#define FZC_RTE_PERIOD_MS              10u    /* 10ms cyclic task rate */

/* ====================================================================
 * Heartbeat Constants
 * ==================================================================== */

#define FZC_HB_TX_PERIOD_MS            50u    /* TX every 50ms */
#define FZC_HB_TIMEOUT_MS             150u    /* 3x TX period */
#define FZC_HB_MAX_MISS                 3u    /* Consecutive misses before timeout */
#define FZC_HB_ALIVE_MAX               15u    /* 4-bit alive counter wraps at 15 */

#define FZC_ECU_ID                   0x02u    /* FZC ECU identifier */

/* ====================================================================
 * Vehicle State (received from CVC)
 * ==================================================================== */

#define FZC_STATE_INIT                  0u
#define FZC_STATE_RUN                   1u
#define FZC_STATE_DEGRADED              2u
#define FZC_STATE_LIMP                  3u
#define FZC_STATE_SAFE_STOP             4u
#define FZC_STATE_SHUTDOWN              5u
#define FZC_STATE_COUNT                 6u

/* ====================================================================
 * Self-Test Constants
 * ==================================================================== */

#define FZC_SELF_TEST_PASS              1u
#define FZC_SELF_TEST_FAIL              0u

/* Number of self-test items */
#define FZC_SELF_TEST_ITEMS             7u

/* ====================================================================
 * FZC Safety Startup Grace Period (SIL only)
 * Suppresses motor cutoff assertion for N cycles after boot to absorb
 * startup transients (SC E-Stop, lidar timeout, brake stabilization).
 * On bare metal the value stays at 0 (transparent — guards always pass).
 * ==================================================================== */

#ifndef FZC_POST_INIT_GRACE_CYCLES
#define FZC_POST_INIT_GRACE_CYCLES   500u   /* 5 seconds at 10ms cycle */
#endif

/* ====================================================================
 * Fault Bitmask Positions
 * ==================================================================== */

#define FZC_FAULT_NONE               0x00u
#define FZC_FAULT_STEER            0x01u
#define FZC_FAULT_BRAKE            0x02u
#define FZC_FAULT_LIDAR            0x04u
#define FZC_FAULT_CAN              0x08u
#define FZC_FAULT_WATCHDOG         0x10u
#define FZC_FAULT_SELF_TEST        0x20u
#define FZC_FAULT_CAN_BUS_OFF     0x0100u  /* bit 8: transport-layer fault, outside 8-bit payload range */

/* ====================================================================
 * Steering Fault Enum
 * ==================================================================== */

#define FZC_STEER_NO_FAULT              0u
#define FZC_STEER_PLAUSIBILITY          1u
#define FZC_STEER_OUT_OF_RANGE          2u
#define FZC_STEER_RATE_EXCEEDED         3u
#define FZC_STEER_CMD_TIMEOUT           4u
#define FZC_STEER_SPI_FAIL              5u

/* ====================================================================
 * Brake Fault Enum
 * ==================================================================== */

#define FZC_BRAKE_NO_FAULT              0u
#define FZC_BRAKE_PWM_DEVIATION         1u
#define FZC_BRAKE_CMD_TIMEOUT           2u
#define FZC_BRAKE_LATCHED               3u
#define FZC_BRAKE_CMD_OSCILLATION       4u

#endif /* FZC_CFG_H */
