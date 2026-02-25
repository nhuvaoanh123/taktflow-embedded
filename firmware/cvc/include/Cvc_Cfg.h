/**
 * @file    Cvc_Cfg.h
 * @brief   CVC configuration — all CVC-specific ID definitions
 * @date    2026-02-21
 *
 * @details Unified configuration header for the Central Vehicle Computer.
 *          Contains RTE signal IDs, Com PDU IDs, DTC event IDs, E2E data IDs,
 *          vehicle state enum, and pedal fault enum.
 *
 * @safety_req SWR-CVC-001 to SWR-CVC-035
 * @traces_to  SSR-CVC-001 to SSR-CVC-035, TSR-022, TSR-030, TSR-038, TSR-046
 *
 * @standard AUTOSAR, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#ifndef CVC_CFG_H
#define CVC_CFG_H

/* ====================================================================
 * RTE Signal IDs (extends BSW well-known IDs at offset 16)
 * ==================================================================== */

#define CVC_SIG_PEDAL_RAW_1       16u
#define CVC_SIG_PEDAL_RAW_2       17u
#define CVC_SIG_PEDAL_POSITION    18u
#define CVC_SIG_PEDAL_FAULT       19u
#define CVC_SIG_VEHICLE_STATE     20u
#define CVC_SIG_TORQUE_REQUEST    21u
#define CVC_SIG_ESTOP_ACTIVE      22u
#define CVC_SIG_FZC_COMM_STATUS   23u
#define CVC_SIG_RZC_COMM_STATUS   24u
#define CVC_SIG_MOTOR_SPEED       25u
#define CVC_SIG_FAULT_MASK        26u
#define CVC_SIG_MOTOR_CURRENT     27u
#define CVC_SIG_MOTOR_CUTOFF      28u
#define CVC_SIG_STEERING_FAULT    29u
#define CVC_SIG_BRAKE_FAULT       30u
#define CVC_SIG_COUNT             31u

/* ====================================================================
 * Com TX PDU IDs
 * ==================================================================== */

#define CVC_COM_TX_ESTOP           0u   /* CAN 0x001 */
#define CVC_COM_TX_HEARTBEAT       1u   /* CAN 0x010 */
#define CVC_COM_TX_VEHICLE_STATE   2u   /* CAN 0x100 */
#define CVC_COM_TX_TORQUE_REQ      3u   /* CAN 0x101 */
#define CVC_COM_TX_STEER_CMD       4u   /* CAN 0x102 */
#define CVC_COM_TX_BRAKE_CMD       5u   /* CAN 0x103 */
#define CVC_COM_TX_BODY_CMD        6u   /* CAN 0x350 */
#define CVC_COM_TX_UDS_RSP         7u   /* CAN 0x7E8 */

/* ====================================================================
 * Com RX PDU IDs
 * ==================================================================== */

#define CVC_COM_RX_FZC_HB          0u   /* CAN 0x011 */
#define CVC_COM_RX_RZC_HB          1u   /* CAN 0x012 */
#define CVC_COM_RX_BRAKE_FAULT     2u   /* CAN 0x210 */
#define CVC_COM_RX_MOTOR_CUTOFF    3u   /* CAN 0x211 */
#define CVC_COM_RX_LIDAR           4u   /* CAN 0x220 */
#define CVC_COM_RX_MOTOR_CURRENT   5u   /* CAN 0x301 */

/* ====================================================================
 * DTC Event IDs (Dem_EventIdType)
 * ==================================================================== */

#define CVC_DTC_PEDAL_PLAUSIBILITY   0u   /* 0xC00100 */
#define CVC_DTC_PEDAL_SENSOR1_FAIL   1u   /* 0xC00200 */
#define CVC_DTC_PEDAL_SENSOR2_FAIL   2u   /* 0xC00300 */
#define CVC_DTC_PEDAL_STUCK          3u   /* 0xC00400 */
#define CVC_DTC_CAN_FZC_TIMEOUT      4u   /* 0xC10100 */
#define CVC_DTC_CAN_RZC_TIMEOUT      5u   /* 0xC10200 */
#define CVC_DTC_CAN_BUS_OFF          6u   /* 0xC10300 */
#define CVC_DTC_MOTOR_OVERCURRENT    7u   /* 0xC20100 */
#define CVC_DTC_MOTOR_OVERTEMP       8u   /* 0xC20200 */
#define CVC_DTC_MOTOR_CUTOFF_RX      9u   /* 0xC20300 */
#define CVC_DTC_BRAKE_FAULT_RX      10u   /* 0xC30100 */
#define CVC_DTC_STEERING_FAULT_RX   11u   /* 0xC30200 */
#define CVC_DTC_ESTOP_ACTIVATED     12u   /* 0xC40100 */
#define CVC_DTC_BATT_UNDERVOLT      13u   /* 0xC50100 */
#define CVC_DTC_BATT_OVERVOLT       14u   /* 0xC50200 */
#define CVC_DTC_NVM_CRC_FAIL        15u   /* 0xC60100 */
#define CVC_DTC_SELF_TEST_FAIL      16u   /* 0xC60200 */
#define CVC_DTC_DISPLAY_COMM        17u   /* 0xC70100 */

/* ====================================================================
 * E2E Data IDs
 * ==================================================================== */

#define CVC_E2E_ESTOP_DATA_ID       0x01u
#define CVC_E2E_HEARTBEAT_DATA_ID   0x02u
#define CVC_E2E_VEHSTATE_DATA_ID    0x05u
#define CVC_E2E_TORQUE_DATA_ID      0x06u

/* ====================================================================
 * Vehicle State Enum
 * ==================================================================== */

#define CVC_STATE_INIT             0u
#define CVC_STATE_RUN              1u
#define CVC_STATE_DEGRADED         2u
#define CVC_STATE_LIMP             3u
#define CVC_STATE_SAFE_STOP        4u
#define CVC_STATE_SHUTDOWN         5u
#define CVC_STATE_COUNT            6u

/* ====================================================================
 * Pedal Fault Enum
 * ==================================================================== */

#define CVC_PEDAL_NO_FAULT         0u
#define CVC_PEDAL_PLAUSIBILITY     1u
#define CVC_PEDAL_STUCK            2u
#define CVC_PEDAL_SENSOR1_FAIL     3u
#define CVC_PEDAL_SENSOR2_FAIL     4u

/* ====================================================================
 * Heartbeat Constants
 * ==================================================================== */

#define CVC_HB_TX_PERIOD_MS       50u   /* TX every 50ms */
#define CVC_HB_TIMEOUT_MS        150u   /* 3x TX period */
#define CVC_HB_MAX_MISS            3u   /* Consecutive misses before timeout */
#define CVC_HB_ALIVE_MAX          15u   /* 4-bit alive counter wraps at 15 */

#define CVC_ECU_ID_CVC             0x01u
#define CVC_ECU_ID_FZC             0x02u
#define CVC_ECU_ID_RZC             0x03u

/* ====================================================================
 * Comm Status Values
 * ==================================================================== */

#define CVC_COMM_OK                0u
#define CVC_COMM_TIMEOUT           1u

/* ====================================================================
 * Pedal Thresholds
 * ==================================================================== */

#define CVC_PEDAL_PLAUS_THRESHOLD     819u   /* |S1-S2| >= 5% of 14-bit range */
#define CVC_PEDAL_PLAUS_DEBOUNCE        2u   /* Cycles before fault */
#define CVC_PEDAL_STUCK_THRESHOLD      10u   /* Delta < 10 = stuck */
#define CVC_PEDAL_STUCK_CYCLES        100u   /* Cycles before stuck fault */
#define CVC_PEDAL_LATCH_CLEAR_CYCLES   50u   /* Fault-free cycles to clear latch */
#define CVC_PEDAL_RAMP_LIMIT            5u   /* 0.5%/cycle = 5 units per cycle of 0-1000 */
#define CVC_PEDAL_MAX_RUN            1000u   /* 100% torque in RUN mode */
#define CVC_PEDAL_MAX_DEGRADED        750u   /* 75% torque in DEGRADED */
#define CVC_PEDAL_MAX_LIMP            300u   /* 30% torque in LIMP */

/* ====================================================================
 * Dashboard Constants
 * ==================================================================== */

#define CVC_DASH_REFRESH_MS        200u   /* Display update every 200ms */

/* ====================================================================
 * Self-Test Constants
 * ==================================================================== */

#define CVC_SELF_TEST_PASS           1u
#define CVC_SELF_TEST_FAIL           0u

/* ====================================================================
 * Vehicle State Event IDs (for state machine transitions)
 * ==================================================================== */

#define CVC_EVT_SELF_TEST_PASS       0u
#define CVC_EVT_SELF_TEST_FAIL       1u
#define CVC_EVT_PEDAL_FAULT_SINGLE   2u
#define CVC_EVT_PEDAL_FAULT_DUAL     3u
#define CVC_EVT_CAN_TIMEOUT_SINGLE   4u
#define CVC_EVT_CAN_TIMEOUT_DUAL     5u
#define CVC_EVT_ESTOP                6u
#define CVC_EVT_SC_KILL              7u
#define CVC_EVT_FAULT_CLEARED        8u
#define CVC_EVT_CAN_RESTORED         9u
#define CVC_EVT_VEHICLE_STOPPED     10u
#define CVC_EVT_MOTOR_CUTOFF        11u
#define CVC_EVT_BRAKE_FAULT         12u
#define CVC_EVT_STEERING_FAULT      13u
#define CVC_EVT_COUNT               14u

/* Invalid/no transition sentinel */
#define CVC_STATE_INVALID          0xFFu

#endif /* CVC_CFG_H */
