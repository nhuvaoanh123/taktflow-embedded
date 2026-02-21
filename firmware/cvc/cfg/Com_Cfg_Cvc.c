/**
 * @file    Com_Cfg_Cvc.c
 * @brief   Com module configuration for CVC — TX/RX PDUs and signal mappings
 * @date    2026-02-21
 *
 * @safety_req SWR-CVC-001 to SWR-CVC-035
 * @traces_to  SSR-CVC-001 to SSR-CVC-035, TSR-022, TSR-023, TSR-024
 *
 * @standard AUTOSAR Com, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#include "Com.h"
#include "Cvc_Cfg.h"

/* ==================================================================
 * Shadow Buffers (static RAM for signal read/write)
 * ================================================================== */

/* TX signal buffers */
static uint8  sig_tx_estop_active;
static uint8  sig_tx_hb_alive;
static uint8  sig_tx_hb_ecu_id;
static uint8  sig_tx_hb_state;
static uint8  sig_tx_vehicle_state;
static uint16 sig_tx_torque_request;
static sint16 sig_tx_steer_angle;
static uint8  sig_tx_brake_pressure;
static uint8  sig_tx_body_cmd;

/* RX signal buffers */
static uint8  sig_rx_fzc_hb_alive;
static uint8  sig_rx_fzc_hb_ecu_id;
static uint8  sig_rx_rzc_hb_alive;
static uint8  sig_rx_rzc_hb_ecu_id;
static uint8  sig_rx_brake_fault;
static uint8  sig_rx_motor_cutoff;
static uint16 sig_rx_lidar_dist;
static uint16 sig_rx_motor_current;

/* ==================================================================
 * Signal Configuration Table
 * Maps signal ID → bit position, size, type, parent PDU, shadow buffer
 * ================================================================== */

static const Com_SignalConfigType cvc_signal_config[] = {
    /* TX signals */
    /* id, bitPos, bitSize, type,       pduId,                   shadowBuf */
    {  0u,   16u,     8u, COM_UINT8,  CVC_COM_TX_ESTOP,         &sig_tx_estop_active   },
    {  1u,   16u,     8u, COM_UINT8,  CVC_COM_TX_HEARTBEAT,     &sig_tx_hb_alive       },
    {  2u,   24u,     8u, COM_UINT8,  CVC_COM_TX_HEARTBEAT,     &sig_tx_hb_ecu_id      },
    {  3u,   32u,     8u, COM_UINT8,  CVC_COM_TX_HEARTBEAT,     &sig_tx_hb_state       },
    {  4u,   16u,     8u, COM_UINT8,  CVC_COM_TX_VEHICLE_STATE, &sig_tx_vehicle_state  },
    {  5u,   16u,    16u, COM_UINT16, CVC_COM_TX_TORQUE_REQ,    &sig_tx_torque_request },
    {  6u,   16u,    16u, COM_SINT16, CVC_COM_TX_STEER_CMD,     &sig_tx_steer_angle    },
    {  7u,   16u,     8u, COM_UINT8,  CVC_COM_TX_BRAKE_CMD,     &sig_tx_brake_pressure },
    {  8u,   16u,     8u, COM_UINT8,  CVC_COM_TX_BODY_CMD,      &sig_tx_body_cmd       },

    /* RX signals */
    {  9u,   16u,     8u, COM_UINT8,  CVC_COM_RX_FZC_HB,        &sig_rx_fzc_hb_alive   },
    { 10u,   24u,     8u, COM_UINT8,  CVC_COM_RX_FZC_HB,        &sig_rx_fzc_hb_ecu_id  },
    { 11u,   16u,     8u, COM_UINT8,  CVC_COM_RX_RZC_HB,        &sig_rx_rzc_hb_alive   },
    { 12u,   24u,     8u, COM_UINT8,  CVC_COM_RX_RZC_HB,        &sig_rx_rzc_hb_ecu_id  },
    { 13u,   16u,     8u, COM_UINT8,  CVC_COM_RX_BRAKE_FAULT,   &sig_rx_brake_fault    },
    { 14u,   16u,     8u, COM_UINT8,  CVC_COM_RX_MOTOR_CUTOFF,  &sig_rx_motor_cutoff   },
    { 15u,   16u,    16u, COM_UINT16, CVC_COM_RX_LIDAR,         &sig_rx_lidar_dist     },
    { 16u,   16u,    16u, COM_UINT16, CVC_COM_RX_MOTOR_CURRENT, &sig_rx_motor_current  },
};

#define CVC_COM_SIGNAL_COUNT  (sizeof(cvc_signal_config) / sizeof(cvc_signal_config[0]))

/* ==================================================================
 * TX PDU Configuration Table
 * Maps PDU ID → DLC, cycle time
 * PDU ID to CAN ID routing is handled by CanIf
 * ================================================================== */

static const Com_TxPduConfigType cvc_tx_pdu_config[] = {
    /* pduId,                     dlc, cycleMs */
    { CVC_COM_TX_ESTOP,            8u,   0u },   /* Event-triggered, not cyclic */
    { CVC_COM_TX_HEARTBEAT,        8u,  50u },   /* 50ms cyclic heartbeat      */
    { CVC_COM_TX_VEHICLE_STATE,    8u, 100u },   /* 100ms state broadcast      */
    { CVC_COM_TX_TORQUE_REQ,       8u,  10u },   /* 10ms torque request        */
    { CVC_COM_TX_STEER_CMD,        8u,  10u },   /* 10ms steering command      */
    { CVC_COM_TX_BRAKE_CMD,        8u,  10u },   /* 10ms brake command         */
    { CVC_COM_TX_BODY_CMD,         8u, 100u },   /* 100ms body control         */
    { CVC_COM_TX_UDS_RSP,          8u,   0u },   /* Event-triggered UDS resp   */
};

#define CVC_COM_TX_PDU_COUNT  (sizeof(cvc_tx_pdu_config) / sizeof(cvc_tx_pdu_config[0]))

/* ==================================================================
 * RX PDU Configuration Table
 * ================================================================== */

static const Com_RxPduConfigType cvc_rx_pdu_config[] = {
    /* pduId,                    dlc, timeoutMs */
    { CVC_COM_RX_FZC_HB,          8u,  150u },   /* 3x heartbeat period  */
    { CVC_COM_RX_RZC_HB,          8u,  150u },   /* 3x heartbeat period  */
    { CVC_COM_RX_BRAKE_FAULT,     8u, 1000u },   /* Event — long timeout */
    { CVC_COM_RX_MOTOR_CUTOFF,    8u, 1000u },   /* Event — long timeout */
    { CVC_COM_RX_LIDAR,           8u,  200u },   /* 5 Hz lidar updates   */
    { CVC_COM_RX_MOTOR_CURRENT,   8u,  200u },   /* 5 Hz motor feedback  */
};

#define CVC_COM_RX_PDU_COUNT  (sizeof(cvc_rx_pdu_config) / sizeof(cvc_rx_pdu_config[0]))

/* ==================================================================
 * Aggregate Com Configuration
 * ================================================================== */

const Com_ConfigType cvc_com_config = {
    .signalConfig = cvc_signal_config,
    .signalCount  = (uint8)CVC_COM_SIGNAL_COUNT,
    .txPduConfig  = cvc_tx_pdu_config,
    .txPduCount   = (uint8)CVC_COM_TX_PDU_COUNT,
    .rxPduConfig  = cvc_rx_pdu_config,
    .rxPduCount   = (uint8)CVC_COM_RX_PDU_COUNT,
};
