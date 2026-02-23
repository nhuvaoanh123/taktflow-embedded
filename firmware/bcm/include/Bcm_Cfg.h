/**
 * @file    Bcm_Cfg.h
 * @brief   BCM configuration — all BCM-specific ID definitions
 * @date    2026-02-23
 *
 * @details Unified configuration header for the Body Control Module.
 *          Contains RTE signal IDs, Com PDU IDs, vehicle state values,
 *          indicator flash parameters, and auto-lock thresholds.
 *
 * @safety_req SWR-BCM-001 to SWR-BCM-012
 * @traces_to  SSR-BCM-001 to SSR-BCM-012, TSR-022, TSR-030
 *
 * @standard AUTOSAR, ISO 26262 Part 6 (QM)
 * @copyright Taktflow Systems 2026
 */
#ifndef BCM_CFG_H
#define BCM_CFG_H

/* ====================================================================
 * RTE Signal IDs (offset 16 from BSW well-known)
 * ==================================================================== */

#define BCM_SIG_VEHICLE_SPEED       16u
#define BCM_SIG_VEHICLE_STATE       17u
#define BCM_SIG_BODY_CONTROL_CMD    18u
#define BCM_SIG_LIGHT_HEADLAMP      19u
#define BCM_SIG_LIGHT_TAIL          20u
#define BCM_SIG_INDICATOR_LEFT      21u
#define BCM_SIG_INDICATOR_RIGHT     22u
#define BCM_SIG_HAZARD_ACTIVE       23u
#define BCM_SIG_DOOR_LOCK_STATE     24u
#define BCM_SIG_ESTOP_ACTIVE        25u
#define BCM_SIG_COUNT               26u

/* ====================================================================
 * Com TX PDU IDs
 * ==================================================================== */

#define BCM_COM_TX_LIGHT_STATUS     0u  /* CAN 0x400 */
#define BCM_COM_TX_INDICATOR_STATE  1u  /* CAN 0x401 */
#define BCM_COM_TX_DOOR_LOCK        2u  /* CAN 0x402 */

/* ====================================================================
 * Com RX PDU IDs
 * ==================================================================== */

#define BCM_COM_RX_VEHICLE_STATE    0u  /* CAN 0x100 */
#define BCM_COM_RX_MOTOR_CURRENT    1u  /* CAN 0x301 */
#define BCM_COM_RX_BODY_CMD         2u  /* CAN 0x350 */

/* ====================================================================
 * Vehicle state values
 * ==================================================================== */

#define BCM_VSTATE_INIT             0u
#define BCM_VSTATE_READY            1u
#define BCM_VSTATE_DRIVING          2u
#define BCM_VSTATE_DEGRADED         3u
#define BCM_VSTATE_ESTOP            4u
#define BCM_VSTATE_FAULT            5u

/* ====================================================================
 * Indicator flash period (ticks at 10ms)
 * ==================================================================== */

#define BCM_INDICATOR_FLASH_ON      33u  /* 330ms on  */
#define BCM_INDICATOR_FLASH_OFF     33u  /* 330ms off = ~1.5Hz */

/* ====================================================================
 * Auto-lock speed threshold
 * ==================================================================== */

#define BCM_AUTO_LOCK_SPEED         10u

#endif /* BCM_CFG_H */
