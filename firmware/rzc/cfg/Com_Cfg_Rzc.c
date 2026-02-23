/**
 * @file    Com_Cfg_Rzc.c
 * @brief   Com module configuration for RZC — TX/RX PDUs and signal mappings
 * @date    2026-02-23
 *
 * @safety_req SWR-RZC-001 to SWR-RZC-030
 * @traces_to  SSR-RZC-001 to SSR-RZC-017, TSR-022, TSR-023, TSR-024
 *
 * @standard AUTOSAR Com, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#include "Com.h"
#include "Rzc_Cfg.h"

/* ==================================================================
 * Shadow Buffers (static RAM for signal read/write)
 * ================================================================== */

/* TX signal buffers — Heartbeat */
static uint8  sig_tx_hb_alive;
static uint8  sig_tx_hb_ecu_id;
static uint8  sig_tx_hb_fault_mask;

/* TX signal buffers — Motor status */
static uint8  sig_tx_torque_echo;
static uint16 sig_tx_motor_speed;
static uint8  sig_tx_motor_dir;
static uint8  sig_tx_motor_enable;
static uint8  sig_tx_motor_fault;

/* TX signal buffers — Motor current */
static uint16 sig_tx_current_mA;
static uint8  sig_tx_overcurrent;

/* TX signal buffers — Motor temp */
static sint16 sig_tx_temp1;
static sint16 sig_tx_temp2;
static uint8  sig_tx_derating_pct;

/* TX signal buffers — Battery */
static uint16 sig_tx_battery_mV;
static uint8  sig_tx_battery_status;

/* RX signal buffers — E-stop */
static uint8  sig_rx_estop_active;

/* RX signal buffers — Vehicle + Torque */
static uint8  sig_rx_vehicle_state;
static sint16 sig_rx_torque_cmd;

/* ==================================================================
 * Signal Configuration Table
 * Maps signal ID -> bit position, size, type, parent PDU, shadow buffer
 * ================================================================== */

static const Com_SignalConfigType rzc_signal_config[] = {
    /* TX signals — Heartbeat PDU */
    /* id, bitPos, bitSize, type,       pduId,                        shadowBuf */
    {  0u,   16u,     8u, COM_UINT8,  RZC_COM_TX_HEARTBEAT,          &sig_tx_hb_alive       },
    {  1u,   24u,     8u, COM_UINT8,  RZC_COM_TX_HEARTBEAT,          &sig_tx_hb_ecu_id      },
    {  2u,   32u,     8u, COM_UINT8,  RZC_COM_TX_HEARTBEAT,          &sig_tx_hb_fault_mask  },

    /* TX signals — Motor status PDU */
    {  3u,   16u,     8u, COM_UINT8,  RZC_COM_TX_MOTOR_STATUS,       &sig_tx_torque_echo    },
    {  4u,   24u,    16u, COM_UINT16, RZC_COM_TX_MOTOR_STATUS,       &sig_tx_motor_speed    },
    {  5u,   40u,     8u, COM_UINT8,  RZC_COM_TX_MOTOR_STATUS,       &sig_tx_motor_dir      },
    {  6u,   48u,     8u, COM_UINT8,  RZC_COM_TX_MOTOR_STATUS,       &sig_tx_motor_enable   },
    {  7u,   56u,     8u, COM_UINT8,  RZC_COM_TX_MOTOR_STATUS,       &sig_tx_motor_fault    },

    /* TX signals — Motor current PDU */
    {  8u,   16u,    16u, COM_UINT16, RZC_COM_TX_MOTOR_CURRENT,      &sig_tx_current_mA     },
    {  9u,   32u,     8u, COM_UINT8,  RZC_COM_TX_MOTOR_CURRENT,      &sig_tx_overcurrent    },

    /* TX signals — Motor temp PDU */
    { 10u,   16u,    16u, COM_SINT16, RZC_COM_TX_MOTOR_TEMP,         &sig_tx_temp1          },
    { 11u,   32u,    16u, COM_SINT16, RZC_COM_TX_MOTOR_TEMP,         &sig_tx_temp2          },
    { 12u,   48u,     8u, COM_UINT8,  RZC_COM_TX_MOTOR_TEMP,         &sig_tx_derating_pct   },

    /* TX signals — Battery status PDU */
    { 13u,   16u,    16u, COM_UINT16, RZC_COM_TX_BATTERY_STATUS,     &sig_tx_battery_mV     },
    { 14u,   32u,     8u, COM_UINT8,  RZC_COM_TX_BATTERY_STATUS,     &sig_tx_battery_status },

    /* RX signals — E-stop PDU */
    { 15u,   16u,     8u, COM_UINT8,  RZC_COM_RX_ESTOP,              &sig_rx_estop_active   },

    /* RX signals — Vehicle + Torque PDU */
    { 16u,   16u,     8u, COM_UINT8,  RZC_COM_RX_VEHICLE_TORQUE,     &sig_rx_vehicle_state  },
    { 17u,   24u,    16u, COM_SINT16, RZC_COM_RX_VEHICLE_TORQUE,     &sig_rx_torque_cmd     },
};

#define RZC_COM_SIGNAL_COUNT  (sizeof(rzc_signal_config) / sizeof(rzc_signal_config[0]))

/* ==================================================================
 * TX PDU Configuration Table
 * Maps PDU ID -> DLC, cycle time
 * PDU ID to CAN ID routing is handled by CanIf
 * ================================================================== */

static const Com_TxPduConfigType rzc_tx_pdu_config[] = {
    /* pduId,                        dlc, cycleMs */
    { RZC_COM_TX_HEARTBEAT,           8u,   50u },   /* 50ms cyclic heartbeat      */
    { RZC_COM_TX_MOTOR_STATUS,        8u,   20u },   /* 20ms motor status          */
    { RZC_COM_TX_MOTOR_CURRENT,       8u,   10u },   /* 10ms motor current         */
    { RZC_COM_TX_MOTOR_TEMP,          8u,  100u },   /* 100ms motor temperature    */
    { RZC_COM_TX_BATTERY_STATUS,      8u, 1000u },   /* 1000ms battery status      */
};

#define RZC_COM_TX_PDU_COUNT  (sizeof(rzc_tx_pdu_config) / sizeof(rzc_tx_pdu_config[0]))

/* ==================================================================
 * RX PDU Configuration Table
 * ================================================================== */

static const Com_RxPduConfigType rzc_rx_pdu_config[] = {
    /* pduId,                       dlc, timeoutMs */
    { RZC_COM_RX_ESTOP,               8u,  150u },   /* 3x heartbeat period        */
    { RZC_COM_RX_VEHICLE_TORQUE,      8u,  100u },   /* 100ms vehicle/torque timeout*/
};

#define RZC_COM_RX_PDU_COUNT  (sizeof(rzc_rx_pdu_config) / sizeof(rzc_rx_pdu_config[0]))

/* ==================================================================
 * Aggregate Com Configuration
 * ================================================================== */

const Com_ConfigType rzc_com_config = {
    .signalConfig = rzc_signal_config,
    .signalCount  = (uint8)RZC_COM_SIGNAL_COUNT,
    .txPduConfig  = rzc_tx_pdu_config,
    .txPduCount   = (uint8)RZC_COM_TX_PDU_COUNT,
    .rxPduConfig  = rzc_rx_pdu_config,
    .rxPduCount   = (uint8)RZC_COM_RX_PDU_COUNT,
};
