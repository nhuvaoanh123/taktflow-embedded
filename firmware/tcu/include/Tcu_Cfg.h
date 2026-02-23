/**
 * @file    Tcu_Cfg.h
 * @brief   TCU configuration -- signal IDs, PDU IDs, timing constants
 * @date    2026-02-23
 *
 * @safety_req SWR-TCU-001
 * @traces_to  TSR-038, TSR-039, TSR-040
 *
 * @copyright Taktflow Systems 2026
 */
#ifndef TCU_CFG_H
#define TCU_CFG_H

/* ---- RTE Signal IDs ---- */

#define TCU_SIG_VEHICLE_SPEED     16u
#define TCU_SIG_MOTOR_TEMP        17u
#define TCU_SIG_BATTERY_VOLTAGE   18u
#define TCU_SIG_MOTOR_CURRENT     19u
#define TCU_SIG_TORQUE_PCT        20u
#define TCU_SIG_MOTOR_RPM         21u
#define TCU_SIG_DTC_BROADCAST     22u
#define TCU_SIG_COUNT             23u

/* ---- Com TX PDU IDs ---- */

#define TCU_COM_TX_UDS_RSP        0u  /**< CAN 0x644 */

/* ---- Com RX PDU IDs ---- */

#define TCU_COM_RX_UDS_FUNC       0u  /**< CAN 0x7DF */
#define TCU_COM_RX_UDS_PHYS       1u  /**< CAN 0x604 */
#define TCU_COM_RX_VEHICLE_STATE  2u  /**< CAN 0x100 */
#define TCU_COM_RX_MOTOR_CURRENT  3u  /**< CAN 0x301 */
#define TCU_COM_RX_MOTOR_TEMP     4u  /**< CAN 0x302 */
#define TCU_COM_RX_BATTERY        5u  /**< CAN 0x303 */
#define TCU_COM_RX_DTC_BCAST      6u  /**< CAN 0x500 */
#define TCU_COM_RX_HB_CVC         7u  /**< CAN 0x010 */
#define TCU_COM_RX_HB_FZC         8u  /**< CAN 0x011 */
#define TCU_COM_RX_HB_RZC         9u  /**< CAN 0x012 */

/* ---- UDS timing ---- */

#define TCU_UDS_SESSION_TIMEOUT_TICKS  500u   /**< 5000ms at 10ms tick */
#define TCU_UDS_SECURITY_LOCKOUT_TICKS 1000u  /**< 10s at 10ms tick   */
#define TCU_UDS_MAX_SECURITY_ATTEMPTS  3u

/* ---- Security keys ---- */

#define TCU_SECURITY_LEVEL1_XOR  0xA5A5A5A5u
#define TCU_SECURITY_LEVEL3_XOR  0x5A5A5A5Au

/* ---- VIN ---- */

#define TCU_VIN_LENGTH  17u
#define TCU_VIN_DEFAULT "TAKTFLOW000000001"

/* ---- DTC aging threshold ---- */

#define TCU_DTC_AGING_CLEAR_CYCLES  40u

#endif /* TCU_CFG_H */
