/**
 * @file    Com_Cfg_Icu.c
 * @brief   Com module configuration for ICU — RX-only PDUs and signal mappings
 * @date    2026-02-23
 *
 * @safety_req SWR-ICU-001 to SWR-ICU-010
 * @traces_to  SSR-ICU-001 to SSR-ICU-010, TSR-022, TSR-023, TSR-024
 *
 * @details  ICU is a listen-only consumer. It subscribes to all CAN
 *           messages on the bus but transmits nothing. Therefore this
 *           configuration has RX PDUs and signals only — no TX config.
 *
 * @standard AUTOSAR Com, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#include "Com.h"
#include "Icu_Cfg.h"

/* ==================================================================
 * Shadow Buffers (static RAM for RX signal read)
 * ICU is listen-only — no TX shadow buffers needed.
 * ================================================================== */

/* E-stop broadcast (CAN 0x001) */
static uint8  sig_rx_estop_active;

/* CVC heartbeat (CAN 0x010) */
static uint8  sig_rx_hb_cvc_alive;
static uint8  sig_rx_hb_cvc_ecu_id;
static uint8  sig_rx_hb_cvc_state;

/* FZC heartbeat (CAN 0x011) */
static uint8  sig_rx_hb_fzc_alive;
static uint8  sig_rx_hb_fzc_ecu_id;

/* RZC heartbeat (CAN 0x012) */
static uint8  sig_rx_hb_rzc_alive;
static uint8  sig_rx_hb_rzc_ecu_id;

/* Vehicle state (CAN 0x100) */
static uint8  sig_rx_vehicle_state;

/* Torque request (CAN 0x101) */
static uint16 sig_rx_torque_request;

/* Motor current / RPM (CAN 0x301) */
static uint16 sig_rx_motor_current;
static uint16 sig_rx_motor_rpm;

/* Motor temperature (CAN 0x302) */
static uint8  sig_rx_motor_temp;

/* Battery voltage (CAN 0x303) */
static uint16 sig_rx_battery_voltage;

/* Light status (CAN 0x400) */
static uint8  sig_rx_light_status;

/* Turn indicator state (CAN 0x401) */
static uint8  sig_rx_indicator_state;

/* Door lock status (CAN 0x402) */
static uint8  sig_rx_door_lock;

/* DTC broadcast (CAN 0x500) */
static uint32 sig_rx_dtc_broadcast;

/* Overcurrent flag (extracted from motor current PDU) */
static uint8  sig_rx_overcurrent_flag;

/* ==================================================================
 * Signal Configuration Table
 * Maps signal ID -> bit position, size, type, parent PDU, shadow buffer
 * All RX signals — ICU transmits nothing.
 * ================================================================== */

static const Com_SignalConfigType icu_signal_config[] = {
    /* id, bitPos, bitSize, type,       pduId,                   shadowBuf */
    {  0u,   16u,     8u, COM_UINT8,  ICU_COM_RX_ESTOP,         &sig_rx_estop_active      },
    {  1u,   16u,     8u, COM_UINT8,  ICU_COM_RX_HB_CVC,        &sig_rx_hb_cvc_alive      },
    {  2u,   24u,     8u, COM_UINT8,  ICU_COM_RX_HB_CVC,        &sig_rx_hb_cvc_ecu_id     },
    {  3u,   32u,     8u, COM_UINT8,  ICU_COM_RX_HB_CVC,        &sig_rx_hb_cvc_state      },
    {  4u,   16u,     8u, COM_UINT8,  ICU_COM_RX_HB_FZC,        &sig_rx_hb_fzc_alive      },
    {  5u,   24u,     8u, COM_UINT8,  ICU_COM_RX_HB_FZC,        &sig_rx_hb_fzc_ecu_id     },
    {  6u,   16u,     8u, COM_UINT8,  ICU_COM_RX_HB_RZC,        &sig_rx_hb_rzc_alive      },
    {  7u,   24u,     8u, COM_UINT8,  ICU_COM_RX_HB_RZC,        &sig_rx_hb_rzc_ecu_id     },
    {  8u,   16u,     8u, COM_UINT8,  ICU_COM_RX_VEHICLE_STATE,  &sig_rx_vehicle_state     },
    {  9u,   16u,    16u, COM_UINT16, ICU_COM_RX_TORQUE_REQ,     &sig_rx_torque_request    },
    { 10u,   16u,    16u, COM_UINT16, ICU_COM_RX_MOTOR_CURRENT,  &sig_rx_motor_current     },
    { 11u,   32u,    16u, COM_UINT16, ICU_COM_RX_MOTOR_CURRENT,  &sig_rx_motor_rpm         },
    { 12u,   48u,     8u, COM_UINT8,  ICU_COM_RX_MOTOR_CURRENT,  &sig_rx_overcurrent_flag  },
    { 13u,   16u,     8u, COM_UINT8,  ICU_COM_RX_MOTOR_TEMP,     &sig_rx_motor_temp        },
    { 14u,   16u,    16u, COM_UINT16, ICU_COM_RX_BATTERY,        &sig_rx_battery_voltage   },
    { 15u,   16u,     8u, COM_UINT8,  ICU_COM_RX_LIGHT_STATUS,   &sig_rx_light_status      },
    { 16u,   16u,     8u, COM_UINT8,  ICU_COM_RX_INDICATOR,      &sig_rx_indicator_state   },
    { 17u,   16u,     8u, COM_UINT8,  ICU_COM_RX_DOOR_LOCK,      &sig_rx_door_lock         },
    { 18u,   16u,    32u, COM_UINT16, ICU_COM_RX_DTC_BCAST,      &sig_rx_dtc_broadcast     },
};

#define ICU_COM_SIGNAL_COUNT  (sizeof(icu_signal_config) / sizeof(icu_signal_config[0]))

/* ==================================================================
 * TX PDU Configuration Table
 * ICU is listen-only — empty TX config.
 * ================================================================== */

/* No TX PDUs for ICU */

/* ==================================================================
 * RX PDU Configuration Table
 * Maps PDU ID -> DLC, timeout
 * ICU subscribes to ALL 13 CAN messages on the bus.
 * ================================================================== */

static const Com_RxPduConfigType icu_rx_pdu_config[] = {
    /* pduId,                    dlc, timeoutMs */
    { ICU_COM_RX_ESTOP,           8u,  500u },   /* E-stop broadcast        */
    { ICU_COM_RX_HB_CVC,          8u,  150u },   /* CVC heartbeat (3x 50ms) */
    { ICU_COM_RX_HB_FZC,          8u,  150u },   /* FZC heartbeat           */
    { ICU_COM_RX_HB_RZC,          8u,  150u },   /* RZC heartbeat           */
    { ICU_COM_RX_VEHICLE_STATE,   8u,  300u },   /* Vehicle state (3x100ms) */
    { ICU_COM_RX_TORQUE_REQ,      8u,   50u },   /* Torque request (5x10ms) */
    { ICU_COM_RX_MOTOR_CURRENT,   8u,  200u },   /* Motor current/RPM       */
    { ICU_COM_RX_MOTOR_TEMP,      8u, 1000u },   /* Motor temp (slow)       */
    { ICU_COM_RX_BATTERY,         8u, 1000u },   /* Battery voltage (slow)  */
    { ICU_COM_RX_LIGHT_STATUS,    8u,  500u },   /* Light status            */
    { ICU_COM_RX_INDICATOR,       8u,  500u },   /* Turn indicator          */
    { ICU_COM_RX_DOOR_LOCK,       8u, 1000u },   /* Door lock status        */
    { ICU_COM_RX_DTC_BCAST,       8u, 5000u },   /* DTC broadcast (event)   */
};

#define ICU_COM_RX_PDU_COUNT_ACTUAL  (sizeof(icu_rx_pdu_config) / sizeof(icu_rx_pdu_config[0]))

/* ==================================================================
 * Aggregate Com Configuration
 * ================================================================== */

const Com_ConfigType icu_com_config = {
    .signalConfig = icu_signal_config,
    .signalCount  = (uint8)ICU_COM_SIGNAL_COUNT,
    .txPduConfig  = NULL_PTR,  /* No TX PDUs — ICU is listen-only */
    .txPduCount   = 0u,
    .rxPduConfig  = icu_rx_pdu_config,
    .rxPduCount   = (uint8)ICU_COM_RX_PDU_COUNT_ACTUAL,
};
