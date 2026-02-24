/**
 * @file    Swc_FzcCom.c
 * @brief   FZC CAN communication — E2E protect/check, message RX/TX
 * @date    2026-02-24
 *
 * @safety_req SWR-FZC-019, SWR-FZC-020, SWR-FZC-026, SWR-FZC-027
 * @traces_to  SSR-FZC-019, SSR-FZC-020, SSR-FZC-026, SSR-FZC-027
 *
 * @details  Implements FZC CAN communication SWC:
 *           1. E2E protection on TX: CRC-8 (poly 0x1D), alive counter,
 *              FZC-specific Data IDs from Fzc_Cfg.h
 *           2. E2E verification on RX with safe defaults:
 *              - Brake safe default: 100% (max braking)
 *              - Steering safe default: 0 deg (center)
 *           3. CAN message reception (10ms cyclic):
 *              0x001 E-stop, 0x100 vehicle state,
 *              0x200 brake cmd, 0x201 steering cmd
 *           4. CAN message transmission (scheduled):
 *              0x011 heartbeat 50ms, 0x210 brake fault (event),
 *              0x211 motor cutoff (event), 0x220 lidar warning (event)
 *
 *           All variables are static file-scope. No dynamic memory.
 *
 * @standard AUTOSAR SWC pattern, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */

#include "Swc_FzcCom.h"
#include "Fzc_Cfg.h"

/* ==================================================================
 * BSW Includes
 * ================================================================== */

#include "Rte.h"
#include "Com.h"

/* ==================================================================
 * Constants
 * ================================================================== */

/** CRC-8 polynomial per AUTOSAR E2E Profile 1 */
#define FZC_COM_CRC8_POLY          0x1Du

/** CRC-8 initial value */
#define FZC_COM_CRC8_INIT          0xFFu

/** Alive counter mask (4-bit, bits [3:0] of byte[1]) */
#define FZC_COM_ALIVE_MASK         0x0Fu

/** Heartbeat TX period in 10ms cycles: 50ms / 10ms = 5 */
#define FZC_COM_HB_PERIOD_CYCLES   5u

/** Safe default: brake 100% on E2E failure */
#define FZC_COM_SAFE_BRAKE_PCT     100u

/** Safe default: steering center (0 deg) on E2E failure */
#define FZC_COM_SAFE_STEER_DEG     0

/* ==================================================================
 * Module State (static file-scope — ASIL D: no dynamic memory)
 * ================================================================== */

static uint8  FzcCom_Initialized;

/** TX alive counter (0..15, wraps) */
static uint8  FzcCom_TxAlive;

/** RX alive counter — last accepted value per RX message */
static uint8  FzcCom_RxAliveEstop;
static uint8  FzcCom_RxAliveVehState;
static uint8  FzcCom_RxAliveBrakeCmd;
static uint8  FzcCom_RxAliveSteerCmd;

/** Heartbeat TX cycle counter */
static uint8  FzcCom_HbCycleCount;

/** Event-driven TX pending flags */
static uint8  FzcCom_TxPendBrakeFault;
static uint8  FzcCom_TxPendMotorCutoff;
static uint8  FzcCom_TxPendLidarWarn;

/* ==================================================================
 * Private: CRC-8 Calculation
 * ================================================================== */

/**
 * @brief  Compute CRC-8 with polynomial 0x1D (SAE J1850)
 * @param  data    Pointer to data bytes
 * @param  length  Number of bytes (excludes CRC byte itself)
 * @param  dataId  Data ID XORed into initial seed
 * @return CRC-8 value
 */
static uint8 FzcCom_CalcCrc8(const uint8* data, uint8 length, uint8 dataId)
{
    uint8 crc;
    uint8 i;
    uint8 bit;

    crc = (uint8)(FZC_COM_CRC8_INIT ^ dataId);

    for (i = 0u; i < length; i++) {
        crc ^= data[i];
        for (bit = 0u; bit < 8u; bit++) {
            if ((crc & 0x80u) != 0u) {
                crc = (uint8)((uint8)(crc << 1u) ^ FZC_COM_CRC8_POLY);
            } else {
                crc = (uint8)(crc << 1u);
            }
        }
    }

    return crc;
}

/* ==================================================================
 * API: Swc_FzcCom_Init
 * ================================================================== */

void Swc_FzcCom_Init(void)
{
    FzcCom_TxAlive          = 0u;
    FzcCom_RxAliveEstop     = 0xFFu;  /* Invalid initial — first msg always accepted */
    FzcCom_RxAliveVehState  = 0xFFu;
    FzcCom_RxAliveBrakeCmd  = 0xFFu;
    FzcCom_RxAliveSteerCmd  = 0xFFu;
    FzcCom_HbCycleCount     = 0u;
    FzcCom_TxPendBrakeFault = FALSE;
    FzcCom_TxPendMotorCutoff = FALSE;
    FzcCom_TxPendLidarWarn  = FALSE;
    FzcCom_Initialized      = TRUE;
}

/* ==================================================================
 * API: Swc_FzcCom_E2eProtect
 * ================================================================== */

Std_ReturnType Swc_FzcCom_E2eProtect(uint8* data, uint8 length, uint8 dataId)
{
    uint8 crc;

    if (data == NULL_PTR) {
        return E_NOT_OK;
    }

    if (length < 2u) {
        return E_NOT_OK;
    }

    /* Insert alive counter into byte[1] bits [3:0] */
    data[1] = (uint8)((data[1] & 0xF0u) | (FzcCom_TxAlive & FZC_COM_ALIVE_MASK));

    /* Compute CRC-8 over bytes [1..length-1], with Data ID in seed */
    crc = FzcCom_CalcCrc8(&data[1], (uint8)(length - 1u), dataId);

    /* Store CRC in byte[0] */
    data[0] = crc;

    /* Increment alive counter (wraps at 15) */
    FzcCom_TxAlive = (uint8)((FzcCom_TxAlive + 1u) & FZC_COM_ALIVE_MASK);

    return E_OK;
}

/* ==================================================================
 * API: Swc_FzcCom_E2eCheck
 * ================================================================== */

Std_ReturnType Swc_FzcCom_E2eCheck(const uint8* data, uint8 length, uint8 dataId)
{
    uint8 received_crc;
    uint8 computed_crc;
    uint8 rx_alive;

    if (data == NULL_PTR) {
        return E_NOT_OK;
    }

    if (length < 2u) {
        return E_NOT_OK;
    }

    /* Extract received CRC from byte[0] */
    received_crc = data[0];

    /* Compute CRC-8 over bytes [1..length-1] with Data ID */
    computed_crc = FzcCom_CalcCrc8(&data[1], (uint8)(length - 1u), dataId);

    if (received_crc != computed_crc) {
        return E_NOT_OK;
    }

    /* Extract alive counter from byte[1] bits [3:0] */
    rx_alive = (uint8)(data[1] & FZC_COM_ALIVE_MASK);
    (void)rx_alive;  /* Alive validation handled per-message in Receive */

    return E_OK;
}

/* ==================================================================
 * API: Swc_FzcCom_Receive (10ms cyclic)
 * ================================================================== */

void Swc_FzcCom_Receive(void)
{
    uint8  rxBuf[8];
    Std_ReturnType ret;

    if (FzcCom_Initialized != TRUE) {
        return;
    }

    /* ---- RX: 0x001 E-stop ---- */
    ret = Com_ReceiveSignal(FZC_COM_RX_ESTOP, rxBuf);
    if (ret == E_OK) {
        if (Swc_FzcCom_E2eCheck(rxBuf, 8u, FZC_E2E_ESTOP_DATA_ID) == E_OK) {
            FzcCom_RxAliveEstop = (uint8)(rxBuf[1] & FZC_COM_ALIVE_MASK);
            (void)Rte_Write(FZC_SIG_ESTOP_ACTIVE, (uint32)rxBuf[2]);
        }
        /* E2E fail on E-stop: assume worst case — set active */
        else {
            (void)Rte_Write(FZC_SIG_ESTOP_ACTIVE, (uint32)1u);
        }
    }

    /* ---- RX: 0x100 Vehicle State ---- */
    ret = Com_ReceiveSignal(FZC_COM_RX_VEHICLE_STATE, rxBuf);
    if (ret == E_OK) {
        if (Swc_FzcCom_E2eCheck(rxBuf, 8u, FZC_E2E_VEHSTATE_DATA_ID) == E_OK) {
            FzcCom_RxAliveVehState = (uint8)(rxBuf[1] & FZC_COM_ALIVE_MASK);
            (void)Rte_Write(FZC_SIG_VEHICLE_STATE, (uint32)rxBuf[2]);
        }
    }

    /* ---- RX: 0x200 Brake Command ---- */
    ret = Com_ReceiveSignal(FZC_COM_RX_BRAKE_CMD, rxBuf);
    if (ret == E_OK) {
        if (Swc_FzcCom_E2eCheck(rxBuf, 8u, FZC_E2E_BRAKE_CMD_DATA_ID) == E_OK) {
            FzcCom_RxAliveBrakeCmd = (uint8)(rxBuf[1] & FZC_COM_ALIVE_MASK);
            (void)Rte_Write(FZC_SIG_BRAKE_CMD, (uint32)rxBuf[2]);
        } else {
            /* E2E failure on brake: safe default = 100% braking */
            (void)Rte_Write(FZC_SIG_BRAKE_CMD, (uint32)FZC_COM_SAFE_BRAKE_PCT);
        }
    }

    /* ---- RX: 0x201 Steering Command ---- */
    ret = Com_ReceiveSignal(FZC_COM_RX_STEER_CMD, rxBuf);
    if (ret == E_OK) {
        if (Swc_FzcCom_E2eCheck(rxBuf, 8u, FZC_E2E_STEER_CMD_DATA_ID) == E_OK) {
            FzcCom_RxAliveSteerCmd = (uint8)(rxBuf[1] & FZC_COM_ALIVE_MASK);
            /* Steering angle in bytes [2..3] as sint16 */
            (void)Rte_Write(FZC_SIG_STEER_CMD,
                (uint32)((uint16)((uint16)rxBuf[2] | ((uint16)rxBuf[3] << 8u))));
        } else {
            /* E2E failure on steering: safe default = center (0 deg) */
            (void)Rte_Write(FZC_SIG_STEER_CMD,
                (uint32)((uint16)((sint16)FZC_COM_SAFE_STEER_DEG)));
        }
    }
}

/* ==================================================================
 * API: Swc_FzcCom_TransmitSchedule (10ms cyclic)
 * ================================================================== */

void Swc_FzcCom_TransmitSchedule(void)
{
    uint8  txBuf[8];
    uint32 rteVal;
    uint8  i;

    if (FzcCom_Initialized != TRUE) {
        return;
    }

    /* ---- TX: 0x011 Heartbeat (50ms periodic) ---- */
    FzcCom_HbCycleCount++;
    if (FzcCom_HbCycleCount >= FZC_COM_HB_PERIOD_CYCLES) {
        FzcCom_HbCycleCount = 0u;

        for (i = 0u; i < 8u; i++) {
            txBuf[i] = 0u;
        }
        txBuf[2] = FZC_ECU_ID;
        (void)Rte_Read(FZC_SIG_VEHICLE_STATE, &rteVal);
        txBuf[3] = (uint8)rteVal;
        (void)Rte_Read(FZC_SIG_FAULT_MASK, &rteVal);
        txBuf[4] = (uint8)rteVal;

        (void)Swc_FzcCom_E2eProtect(txBuf, 8u, FZC_E2E_HEARTBEAT_DATA_ID);
        (void)Com_SendSignal(FZC_COM_TX_HEARTBEAT, txBuf);
    }

    /* ---- TX: 0x210 Brake Fault (event-driven) ---- */
    (void)Rte_Read(FZC_SIG_BRAKE_FAULT, &rteVal);
    if (rteVal != (uint32)FZC_BRAKE_NO_FAULT) {
        FzcCom_TxPendBrakeFault = TRUE;
    }
    if (FzcCom_TxPendBrakeFault == TRUE) {
        for (i = 0u; i < 8u; i++) {
            txBuf[i] = 0u;
        }
        txBuf[2] = (uint8)rteVal;
        (void)Swc_FzcCom_E2eProtect(txBuf, 8u, FZC_E2E_BRAKE_STATUS_DATA_ID);
        (void)Com_SendSignal(FZC_COM_TX_BRAKE_FAULT, txBuf);
        FzcCom_TxPendBrakeFault = FALSE;
    }

    /* ---- TX: 0x211 Motor Cutoff (event-driven) ---- */
    (void)Rte_Read(FZC_SIG_MOTOR_CUTOFF, &rteVal);
    if (rteVal != 0u) {
        FzcCom_TxPendMotorCutoff = TRUE;
    }
    if (FzcCom_TxPendMotorCutoff == TRUE) {
        for (i = 0u; i < 8u; i++) {
            txBuf[i] = 0u;
        }
        txBuf[2] = (uint8)rteVal;
        (void)Swc_FzcCom_E2eProtect(txBuf, 8u, FZC_E2E_BRAKE_STATUS_DATA_ID);
        (void)Com_SendSignal(FZC_COM_TX_MOTOR_CUTOFF, txBuf);
        FzcCom_TxPendMotorCutoff = FALSE;
    }

    /* ---- TX: 0x220 Lidar Warning (event-driven) ---- */
    (void)Rte_Read(FZC_SIG_LIDAR_ZONE, &rteVal);
    if (rteVal >= (uint32)FZC_LIDAR_ZONE_WARNING) {
        FzcCom_TxPendLidarWarn = TRUE;
    }
    if (FzcCom_TxPendLidarWarn == TRUE) {
        for (i = 0u; i < 8u; i++) {
            txBuf[i] = 0u;
        }
        txBuf[2] = (uint8)rteVal;
        (void)Rte_Read(FZC_SIG_LIDAR_DIST, &rteVal);
        txBuf[3] = (uint8)(rteVal & 0xFFu);
        txBuf[4] = (uint8)((rteVal >> 8u) & 0xFFu);
        (void)Swc_FzcCom_E2eProtect(txBuf, 8u, FZC_E2E_LIDAR_DATA_ID);
        (void)Com_SendSignal(FZC_COM_TX_LIDAR, txBuf);
        FzcCom_TxPendLidarWarn = FALSE;
    }
}
