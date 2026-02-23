/**
 * @file    Com_Cfg_Tcu.c
 * @brief   Com configuration for TCU -- TX/RX PDU and shadow buffer setup
 * @date    2026-02-23
 *
 * @safety_req SWR-TCU-001
 * @traces_to  TSR-022, TSR-023, TSR-024
 *
 * @copyright Taktflow Systems 2026
 */

#include "Com.h"
#include "Tcu_Cfg.h"

/* ---- Shadow Buffers ---- */

/* TX: UDS response (0x644) */
static uint8 tcu_tx_uds_rsp_buf[COM_PDU_SIZE];

/* RX shadow buffers for UDS and heartbeat PDUs — not yet wired to
 * Com signal config (no mapped signals).  Restore when Com routing
 * is extended to cover these PDUs. */
#if 0
static uint8 tcu_rx_uds_func_buf[COM_PDU_SIZE];     /* 0x7DF functional */
static uint8 tcu_rx_uds_phys_buf[COM_PDU_SIZE];     /* 0x604 physical   */
static uint8 tcu_rx_hb_cvc_buf[COM_PDU_SIZE];        /* 0x010            */
static uint8 tcu_rx_hb_fzc_buf[COM_PDU_SIZE];        /* 0x011            */
static uint8 tcu_rx_hb_rzc_buf[COM_PDU_SIZE];        /* 0x012            */
#endif

/* RX shadow buffers (only for PDUs with mapped signals) */
static uint8 tcu_rx_vehicle_state_buf[COM_PDU_SIZE]; /* 0x100            */
static uint8 tcu_rx_motor_current_buf[COM_PDU_SIZE]; /* 0x301            */
static uint8 tcu_rx_motor_temp_buf[COM_PDU_SIZE];    /* 0x302            */
static uint8 tcu_rx_battery_buf[COM_PDU_SIZE];       /* 0x303            */
static uint8 tcu_rx_dtc_bcast_buf[COM_PDU_SIZE];     /* 0x500            */

/* ---- Signal Configuration ---- */

static const Com_SignalConfigType tcu_signal_config[] = {
    /* TX: UDS response data (8 bytes mapped as individual signals) */
    {
        .SignalId     = 0u,
        .BitPosition  = 0u,
        .BitSize      = 64u,
        .Type         = COM_UINT8,
        .PduId        = TCU_COM_TX_UDS_RSP,
        .ShadowBuffer = tcu_tx_uds_rsp_buf,
    },

    /* RX: Vehicle speed from CAN 0x100 (bytes 0-1) */
    {
        .SignalId     = 1u,
        .BitPosition  = 0u,
        .BitSize      = 16u,
        .Type         = COM_UINT16,
        .PduId        = TCU_COM_RX_VEHICLE_STATE,
        .ShadowBuffer = tcu_rx_vehicle_state_buf,
    },

    /* RX: Motor current from CAN 0x301 (bytes 0-1) */
    {
        .SignalId     = 2u,
        .BitPosition  = 0u,
        .BitSize      = 16u,
        .Type         = COM_UINT16,
        .PduId        = TCU_COM_RX_MOTOR_CURRENT,
        .ShadowBuffer = tcu_rx_motor_current_buf,
    },

    /* RX: Motor temperature from CAN 0x302 (byte 0) */
    {
        .SignalId     = 3u,
        .BitPosition  = 0u,
        .BitSize      = 8u,
        .Type         = COM_UINT8,
        .PduId        = TCU_COM_RX_MOTOR_TEMP,
        .ShadowBuffer = tcu_rx_motor_temp_buf,
    },

    /* RX: Battery voltage from CAN 0x303 (bytes 0-1) */
    {
        .SignalId     = 4u,
        .BitPosition  = 0u,
        .BitSize      = 16u,
        .Type         = COM_UINT16,
        .PduId        = TCU_COM_RX_BATTERY,
        .ShadowBuffer = tcu_rx_battery_buf,
    },

    /* RX: DTC broadcast from CAN 0x500 (bytes 0-2, 24-bit DTC code) */
    {
        .SignalId     = 5u,
        .BitPosition  = 0u,
        .BitSize      = 24u,
        .Type         = COM_UINT16,  /* Closest available type */
        .PduId        = TCU_COM_RX_DTC_BCAST,
        .ShadowBuffer = tcu_rx_dtc_bcast_buf,
    },
};

/* ---- TX PDU Configuration ---- */

static const Com_TxPduConfigType tcu_tx_pdu_config[] = {
    /* UDS response -- event-triggered (CycleTimeMs = 0 means event) */
    {
        .PduId       = TCU_COM_TX_UDS_RSP,
        .Dlc         = 8u,
        .CycleTimeMs = 0u,  /* Event-triggered, not periodic */
    },
};

/* ---- RX PDU Configuration ---- */

static const Com_RxPduConfigType tcu_rx_pdu_config[] = {
    /* Functional UDS request (0x7DF) */
    {
        .PduId     = TCU_COM_RX_UDS_FUNC,
        .Dlc       = 8u,
        .TimeoutMs = 0u,  /* No timeout for diagnostic requests */
    },
    /* Physical UDS request (0x604) */
    {
        .PduId     = TCU_COM_RX_UDS_PHYS,
        .Dlc       = 8u,
        .TimeoutMs = 0u,
    },
    /* Vehicle state (0x100) */
    {
        .PduId     = TCU_COM_RX_VEHICLE_STATE,
        .Dlc       = 8u,
        .TimeoutMs = 500u,
    },
    /* Motor current (0x301) */
    {
        .PduId     = TCU_COM_RX_MOTOR_CURRENT,
        .Dlc       = 8u,
        .TimeoutMs = 500u,
    },
    /* Motor temperature (0x302) */
    {
        .PduId     = TCU_COM_RX_MOTOR_TEMP,
        .Dlc       = 8u,
        .TimeoutMs = 500u,
    },
    /* Battery (0x303) */
    {
        .PduId     = TCU_COM_RX_BATTERY,
        .Dlc       = 8u,
        .TimeoutMs = 500u,
    },
    /* DTC broadcast (0x500) */
    {
        .PduId     = TCU_COM_RX_DTC_BCAST,
        .Dlc       = 8u,
        .TimeoutMs = 0u,  /* Event-based */
    },
    /* Heartbeat CVC (0x010) */
    {
        .PduId     = TCU_COM_RX_HB_CVC,
        .Dlc       = 8u,
        .TimeoutMs = 1000u,
    },
    /* Heartbeat FZC (0x011) */
    {
        .PduId     = TCU_COM_RX_HB_FZC,
        .Dlc       = 8u,
        .TimeoutMs = 1000u,
    },
    /* Heartbeat RZC (0x012) */
    {
        .PduId     = TCU_COM_RX_HB_RZC,
        .Dlc       = 8u,
        .TimeoutMs = 1000u,
    },
};

/* ---- Aggregate Com Configuration ---- */

const Com_ConfigType tcu_com_config = {
    .signalConfig = tcu_signal_config,
    .signalCount  = (uint8)(sizeof(tcu_signal_config) / sizeof(tcu_signal_config[0])),
    .txPduConfig  = tcu_tx_pdu_config,
    .txPduCount   = (uint8)(sizeof(tcu_tx_pdu_config) / sizeof(tcu_tx_pdu_config[0])),
    .rxPduConfig  = tcu_rx_pdu_config,
    .rxPduCount   = (uint8)(sizeof(tcu_rx_pdu_config) / sizeof(tcu_rx_pdu_config[0])),
};
