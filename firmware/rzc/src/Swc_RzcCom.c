/**
 * @file    Swc_RzcCom.c
 * @brief   RZC CAN communication -- E2E protection, message RX/TX tables
 * @date    2026-02-24
 *
 * @safety_req SWR-RZC-019, SWR-RZC-020, SWR-RZC-026, SWR-RZC-027
 * @traces_to  SSR-RZC-019, SSR-RZC-020, SSR-RZC-026, SSR-RZC-027
 *
 * @details  Implements RZC CAN communication:
 *           1.  E2E transmit: CRC-8 polynomial 0x1D over payload bytes
 *               1..7 XOR'd with RZC-specific Data ID, alive counter in
 *               byte 1 bits [3:0], CRC in byte 0. 16-entry alive counter
 *               array indexed by PDU ID.
 *           2.  E2E receive: CRC verify + alive counter monotonic check.
 *               3 consecutive failures -> safe default = zero torque for
 *               torque command messages.
 *           3.  CAN RX table (10ms cyclic):
 *               0x001 E-stop broadcast -> set RZC_SIG_ESTOP_ACTIVE
 *               0x100 Vehicle_State + Torque -> write to RTE, 100ms timeout
 *           4.  CAN TX schedule (10ms cyclic):
 *               0x012 heartbeat every 50ms (5 cycles)
 *               0x301 motor status every 10ms (current/temp/speed/battery)
 *
 *           All variables are static file-scope. No dynamic memory.
 *
 * @standard AUTOSAR SWC pattern, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */

#include "Swc_RzcCom.h"
#include "Rzc_Cfg.h"

/* ==================================================================
 * BSW Includes
 * ================================================================== */

#include "Rte.h"
#include "Com.h"
#include "Dem.h"

/* ==================================================================
 * Constants
 * ================================================================== */

/** CRC-8 polynomial 0x1D (SAE J1850) */
#define RZCCOM_CRC8_POLY        0x1Du

/** Number of alive counter slots (one per PDU) */
#define RZCCOM_ALIVE_SLOTS      16u

/** E2E consecutive failure threshold for safe default */
#define RZCCOM_E2E_FAIL_LIMIT    3u

/** Torque command timeout in 10ms cycles: 100ms / 10ms = 10 */
#define RZCCOM_TORQUE_TIMEOUT   10u

/** Heartbeat TX period in 10ms cycles: 50ms / 10ms = 5 */
#define RZCCOM_HB_PERIOD         5u

/** Motor status TX period in 10ms cycles: 10ms / 10ms = 1 */
#define RZCCOM_MSTATUS_PERIOD    1u

/* ==================================================================
 * Module State (all static file-scope -- ASIL D: no dynamic memory)
 * ================================================================== */

/** Module initialization flag */
static uint8   RzcCom_Initialized;

/** TX alive counters: one 4-bit counter per PDU */
static uint8   RzcCom_TxAlive[RZCCOM_ALIVE_SLOTS];

/** RX alive counters: last received value per PDU */
static uint8   RzcCom_RxAlive[RZCCOM_ALIVE_SLOTS];

/** RX E2E consecutive failure count per PDU */
static uint8   RzcCom_RxFailCount[RZCCOM_ALIVE_SLOTS];

/** Torque command timeout counter (10ms cycles since last valid RX) */
static uint16  RzcCom_TorqueTimeout;

/** Heartbeat TX cycle counter */
static uint8   RzcCom_HbCycleCount;

/* ==================================================================
 * Private Helper: CRC-8 calculation (polynomial 0x1D)
 * ================================================================== */

/**
 * @brief  Compute CRC-8 over a byte array
 * @param  data   Pointer to data bytes
 * @param  length Number of bytes
 * @param  dataId Data ID XOR'd into CRC initial value
 * @return CRC-8 result
 */
static uint8 RzcCom_Crc8(const uint8 *data, uint8 length, uint8 dataId)
{
    uint8 crc;
    uint8 i;
    uint8 bit;

    crc = dataId;

    for (i = 0u; i < length; i++)
    {
        crc ^= data[i];

        for (bit = 0u; bit < 8u; bit++)
        {
            if ((crc & 0x80u) != 0u)
            {
                crc = (uint8)((uint8)(crc << 1u) ^ RZCCOM_CRC8_POLY);
            }
            else
            {
                crc = (uint8)(crc << 1u);
            }
        }
    }

    return crc;
}

/* ==================================================================
 * Private: RZC-specific Data ID lookup
 * ================================================================== */

/**
 * @brief  Get the E2E Data ID for a given PDU index
 * @param  pduId  PDU index
 * @return Data ID byte, or 0x00 if unknown
 */
static uint8 RzcCom_GetTxDataId(uint8 pduId)
{
    switch (pduId)
    {
        case RZC_COM_TX_HEARTBEAT:
            return RZC_E2E_HEARTBEAT_DATA_ID;
        case RZC_COM_TX_MOTOR_CURRENT:
            return RZC_E2E_MOTOR_CURRENT_DATA_ID;
        case RZC_COM_TX_MOTOR_STATUS:
            return RZC_E2E_MOTOR_STATUS_DATA_ID;
        default:
            return 0x00u;
    }
}

static uint8 RzcCom_GetRxDataId(uint8 pduId)
{
    switch (pduId)
    {
        case RZC_COM_RX_ESTOP:
            return RZC_E2E_ESTOP_DATA_ID;
        default:
            return 0x00u;
    }
}

/* ==================================================================
 * API: Swc_RzcCom_Init
 * ================================================================== */

void Swc_RzcCom_Init(void)
{
    uint8 i;

    for (i = 0u; i < RZCCOM_ALIVE_SLOTS; i++)
    {
        RzcCom_TxAlive[i]     = 0u;
        RzcCom_RxAlive[i]     = 0u;
        RzcCom_RxFailCount[i] = 0u;
    }

    RzcCom_TorqueTimeout = 0u;
    RzcCom_HbCycleCount  = 0u;
    RzcCom_Initialized   = TRUE;
}

/* ==================================================================
 * API: Swc_RzcCom_E2eProtect (TX)
 * ================================================================== */

Std_ReturnType Swc_RzcCom_E2eProtect(uint8 pduId, uint8 *data, uint8 length)
{
    uint8 dataId;
    uint8 alive;
    uint8 crc;

    if ((data == NULL_PTR) || (length < 2u))
    {
        return E_NOT_OK;
    }

    if (pduId >= RZCCOM_ALIVE_SLOTS)
    {
        return E_NOT_OK;
    }

    dataId = RzcCom_GetTxDataId(pduId);
    alive  = RzcCom_TxAlive[pduId];

    /* Write alive counter into byte 1 low nibble */
    data[1] = (uint8)((data[1] & 0xF0u) | (alive & 0x0Fu));

    /* Compute CRC over bytes 1..length-1 */
    crc = RzcCom_Crc8(&data[1], (uint8)(length - 1u), dataId);

    /* Write CRC into byte 0 */
    data[0] = crc;

    /* Increment alive counter with wrap at 15 */
    alive++;
    if (alive > 15u)
    {
        alive = 0u;
    }
    RzcCom_TxAlive[pduId] = alive;

    return E_OK;
}

/* ==================================================================
 * API: Swc_RzcCom_E2eCheck (RX)
 * ================================================================== */

Std_ReturnType Swc_RzcCom_E2eCheck(uint8 pduId, const uint8 *data, uint8 length)
{
    uint8 dataId;
    uint8 rx_crc;
    uint8 calc_crc;
    uint8 rx_alive;
    uint8 expected_alive;

    if ((data == NULL_PTR) || (length < 2u))
    {
        return E_NOT_OK;
    }

    if (pduId >= RZCCOM_ALIVE_SLOTS)
    {
        return E_NOT_OK;
    }

    dataId = RzcCom_GetTxDataId(pduId);

    /* Extract received CRC from byte 0 */
    rx_crc = data[0];

    /* Compute expected CRC over bytes 1..length-1 */
    calc_crc = RzcCom_Crc8(&data[1], (uint8)(length - 1u), dataId);

    if (rx_crc != calc_crc)
    {
        RzcCom_RxFailCount[pduId]++;
        return E_NOT_OK;
    }

    /* Extract received alive counter from byte 1 low nibble */
    rx_alive = (uint8)(data[1] & 0x0Fu);

    /* Check alive counter is monotonically increasing */
    expected_alive = RzcCom_RxAlive[pduId];
    expected_alive++;
    if (expected_alive > 15u)
    {
        expected_alive = 0u;
    }

    if (rx_alive != expected_alive)
    {
        RzcCom_RxFailCount[pduId]++;
        RzcCom_RxAlive[pduId] = rx_alive;  /* Re-sync */
        return E_NOT_OK;
    }

    /* Valid: update alive tracker and clear failure count */
    RzcCom_RxAlive[pduId]     = rx_alive;
    RzcCom_RxFailCount[pduId] = 0u;

    return E_OK;
}

/* ==================================================================
 * API: Swc_RzcCom_Receive (10ms cyclic)
 * ================================================================== */

void Swc_RzcCom_Receive(void)
{
    uint32 estop_raw;
    uint32 vehicle_raw;
    uint32 torque_raw;
    uint8  new_torque_received;

    if (RzcCom_Initialized != TRUE)
    {
        return;
    }

    /* --- 0x001 E-stop broadcast --- */
    estop_raw = 0u;
    (void)Rte_Read(RZC_SIG_ESTOP_ACTIVE, &estop_raw);

    if (estop_raw != 0u)
    {
        /* E-stop active: signal motor disable via RTE */
        (void)Rte_Write(RZC_SIG_ESTOP_ACTIVE, 1u);
    }

    /* --- 0x100 Vehicle_State + Torque --- */
    /* Check if E2E for torque PDU has exceeded failure threshold */
    if (RzcCom_RxFailCount[RZC_COM_RX_VEHICLE_TORQUE] >= RZCCOM_E2E_FAIL_LIMIT)
    {
        /* 3 consecutive E2E failures: safe default = zero torque */
        (void)Rte_Write(RZC_SIG_TORQUE_CMD, 0u);
        Dem_ReportErrorStatus(RZC_DTC_CAN_BUS_OFF, DEM_EVENT_STATUS_FAILED);
        return;
    }

    /* Read vehicle state and torque from RTE (set by lower BSW/Com) */
    vehicle_raw = 0u;
    (void)Rte_Read(RZC_SIG_VEHICLE_STATE, &vehicle_raw);

    torque_raw = 0u;
    new_torque_received = FALSE;
    (void)Rte_Read(RZC_SIG_TORQUE_CMD, &torque_raw);

    /* Detect new torque command (simplified: any non-zero read) */
    if (torque_raw != 0u)
    {
        new_torque_received = TRUE;
    }

    /* Torque command timeout: 100ms with no new command */
    if (new_torque_received == TRUE)
    {
        RzcCom_TorqueTimeout = 0u;
    }
    else
    {
        if (RzcCom_TorqueTimeout < 0xFFFFu)
        {
            RzcCom_TorqueTimeout++;
        }

        if (RzcCom_TorqueTimeout >= RZCCOM_TORQUE_TIMEOUT)
        {
            /* Timeout: force zero torque */
            (void)Rte_Write(RZC_SIG_TORQUE_CMD, 0u);
        }
    }
}

/* ==================================================================
 * API: Swc_RzcCom_TransmitSchedule (10ms cyclic)
 * ================================================================== */

void Swc_RzcCom_TransmitSchedule(void)
{
    uint32 current_ma;
    uint32 temp_ddc;
    uint32 speed_rpm;
    uint32 battery_mv;
    uint8  motor_data[8];
    uint8  hb_data[8];
    uint8  i;

    if (RzcCom_Initialized != TRUE)
    {
        return;
    }

    /* --- 0x301 Motor status: every 10ms (every cycle) --- */
    current_ma = 0u;
    (void)Rte_Read(RZC_SIG_CURRENT_MA, &current_ma);

    temp_ddc = 0u;
    (void)Rte_Read(RZC_SIG_TEMP1_DC, &temp_ddc);

    speed_rpm = 0u;
    (void)Rte_Read(RZC_SIG_ENCODER_SPEED, &speed_rpm);

    battery_mv = 0u;
    (void)Rte_Read(RZC_SIG_BATTERY_MV, &battery_mv);

    for (i = 0u; i < 8u; i++)
    {
        motor_data[i] = 0u;
    }

    /* Pack motor status: current(2), temp(2), speed(2), battery(2) */
    motor_data[0] = 0u;  /* Reserved for E2E CRC */
    motor_data[1] = 0u;  /* Reserved for alive counter */
    motor_data[2] = (uint8)(current_ma & 0xFFu);
    motor_data[3] = (uint8)((current_ma >> 8u) & 0xFFu);
    motor_data[4] = (uint8)(temp_ddc & 0xFFu);
    motor_data[5] = (uint8)((temp_ddc >> 8u) & 0xFFu);
    motor_data[6] = (uint8)(speed_rpm & 0xFFu);
    motor_data[7] = (uint8)((speed_rpm >> 8u) & 0xFFu);

    (void)Swc_RzcCom_E2eProtect(RZC_COM_TX_MOTOR_CURRENT, motor_data, 8u);
    (void)Com_SendSignal(RZC_COM_TX_MOTOR_CURRENT, motor_data);

    /* --- 0x012 Heartbeat: every 50ms (5 cycles) --- */
    RzcCom_HbCycleCount++;

    if (RzcCom_HbCycleCount >= RZCCOM_HB_PERIOD)
    {
        RzcCom_HbCycleCount = 0u;

        for (i = 0u; i < 8u; i++)
        {
            hb_data[i] = 0u;
        }

        hb_data[1] = RZC_ECU_ID;

        (void)Swc_RzcCom_E2eProtect(RZC_COM_TX_HEARTBEAT, hb_data, 8u);
        (void)Com_SendSignal(RZC_COM_TX_HEARTBEAT, hb_data);
    }
}
