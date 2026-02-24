/**
 * @file    Swc_CvcCom.c
 * @brief   CVC CAN communication — E2E protect/check, RX routing, TX scheduling
 * @date    2026-02-24
 *
 * @safety_req SWR-CVC-014, SWR-CVC-015, SWR-CVC-016, SWR-CVC-017
 * @traces_to  SSR-CVC-014, SSR-CVC-015, SSR-CVC-016, SSR-CVC-017,
 *             TSR-022, TSR-023, TSR-024
 *
 * @details  E2E Protect: CRC-8 0x1D, alive counter, Data ID per message
 *           E2E Check: CRC verify, alive counter verify, 3-fail safe default
 *           RX Routing: CAN ID table from FZC/RZC with E2E check
 *           TX Scheduling: CAN ID table with configurable periods
 *
 *           All variables are static file-scope. No dynamic memory.
 *
 * @standard AUTOSAR Com/E2E integration, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */

#include "Swc_CvcCom.h"
#include "Cvc_Cfg.h"
#include "E2E.h"

/* ==================================================================
 * TX Message Schedule Table (SWR-CVC-017)
 * ================================================================== */

static const Swc_CvcCom_TxEntryType CvcCom_TxTable[5] = {
    { 0x001u,  10u, CVC_E2E_ESTOP_DATA_ID,     8u },  /* E-stop: 10ms      */
    { 0x010u,  50u, CVC_E2E_HEARTBEAT_DATA_ID,  8u },  /* Heartbeat: 50ms   */
    { 0x100u,  20u, CVC_E2E_VEHSTATE_DATA_ID,   8u },  /* Vehicle state:20ms*/
    { 0x200u,  10u, CVC_E2E_TORQUE_DATA_ID,     8u },  /* Torque req: 10ms  */
    { 0x201u,  20u, 0x07u,                       8u },  /* Steer cmd: 20ms   */
};

#define CVCCOM_TX_TABLE_SIZE  5u

/* ==================================================================
 * RX Message Routing Table (SWR-CVC-016)
 * ================================================================== */

static const Swc_CvcCom_RxEntryType CvcCom_RxTable[4] = {
    { 0x011u, 0x02u, 8u },  /* FZC heartbeat  */
    { 0x012u, 0x03u, 8u },  /* RZC heartbeat  */
    { 0x210u, 0x08u, 8u },  /* Brake fault    */
    { 0x301u, 0x09u, 8u },  /* Motor current  */
};

#define CVCCOM_RX_TABLE_SIZE  4u

/* ==================================================================
 * Module State (all static file-scope — ASIL D: no dynamic memory)
 * ================================================================== */

static uint8   CvcCom_Initialized;

/* E2E TX state (per TX message) */
static E2E_StateType     CvcCom_TxE2eState[CVCCOM_TX_TABLE_SIZE];
static E2E_ConfigType    CvcCom_TxE2eConfig[CVCCOM_TX_TABLE_SIZE];

/* E2E RX state (per RX message) */
static E2E_StateType     CvcCom_RxE2eState[CVCCOM_RX_TABLE_SIZE];
static E2E_ConfigType    CvcCom_RxE2eConfig[CVCCOM_RX_TABLE_SIZE];

/* RX status tracking */
static Swc_CvcCom_RxStatusType CvcCom_RxStatus[CVCCOM_RX_TABLE_SIZE];

/* TX last-transmit timestamps */
static uint32  CvcCom_TxLastTimeMs[CVCCOM_TX_TABLE_SIZE];

/* ==================================================================
 * API: Swc_CvcCom_Init
 * ================================================================== */

void Swc_CvcCom_Init(void)
{
    uint8 i;

    /* Initialize TX E2E state and config */
    for (i = 0u; i < CVCCOM_TX_TABLE_SIZE; i++)
    {
        CvcCom_TxE2eState[i].Counter     = 0u;
        CvcCom_TxE2eConfig[i].DataId     = CvcCom_TxTable[i].dataId;
        CvcCom_TxE2eConfig[i].MaxDeltaCounter = 3u;
        CvcCom_TxE2eConfig[i].DataLength = CvcCom_TxTable[i].dlc;
        CvcCom_TxLastTimeMs[i]           = 0u;
    }

    /* Initialize RX E2E state and config */
    for (i = 0u; i < CVCCOM_RX_TABLE_SIZE; i++)
    {
        CvcCom_RxE2eState[i].Counter     = 0u;
        CvcCom_RxE2eConfig[i].DataId     = CvcCom_RxTable[i].dataId;
        CvcCom_RxE2eConfig[i].MaxDeltaCounter = 3u;
        CvcCom_RxE2eConfig[i].DataLength = CvcCom_RxTable[i].dlc;

        CvcCom_RxStatus[i].failCount     = 0u;
        CvcCom_RxStatus[i].useSafeDefault = FALSE;
    }

    CvcCom_Initialized = TRUE;
}

/* ==================================================================
 * API: Swc_CvcCom_E2eProtect
 * ================================================================== */

/**
 * @safety_req SWR-CVC-014
 */
Std_ReturnType Swc_CvcCom_E2eProtect(uint8 txIndex, uint8* payload,
                                      uint8 length)
{
    if (CvcCom_Initialized != TRUE)
    {
        return E_NOT_OK;
    }

    if (txIndex >= CVCCOM_TX_TABLE_SIZE)
    {
        return E_NOT_OK;
    }

    if (payload == NULL_PTR)
    {
        return E_NOT_OK;
    }

    return E2E_Protect(&CvcCom_TxE2eConfig[txIndex],
                        &CvcCom_TxE2eState[txIndex],
                        payload,
                        (uint16)length);
}

/* ==================================================================
 * API: Swc_CvcCom_E2eCheck
 * ================================================================== */

/**
 * @safety_req SWR-CVC-015
 */
Std_ReturnType Swc_CvcCom_E2eCheck(uint8 rxIndex, const uint8* payload,
                                    uint8 length)
{
    E2E_CheckStatusType e2eResult;

    if (CvcCom_Initialized != TRUE)
    {
        return E_NOT_OK;
    }

    if (rxIndex >= CVCCOM_RX_TABLE_SIZE)
    {
        return E_NOT_OK;
    }

    if (payload == NULL_PTR)
    {
        return E_NOT_OK;
    }

    e2eResult = E2E_Check(&CvcCom_RxE2eConfig[rxIndex],
                           &CvcCom_RxE2eState[rxIndex],
                           payload,
                           (uint16)length);

    if (e2eResult == E2E_STATUS_OK)
    {
        /* Valid message — reset failure counter */
        CvcCom_RxStatus[rxIndex].failCount     = 0u;
        CvcCom_RxStatus[rxIndex].useSafeDefault = FALSE;
        return E_OK;
    }

    /* E2E check failed — increment failure counter */
    CvcCom_RxStatus[rxIndex].failCount++;

    if (CvcCom_RxStatus[rxIndex].failCount >= CVCCOM_E2E_FAIL_THRESHOLD)
    {
        /* 3 consecutive failures — use safe default values */
        CvcCom_RxStatus[rxIndex].useSafeDefault = TRUE;
    }

    return E_NOT_OK;
}

/* ==================================================================
 * API: Swc_CvcCom_Receive
 * ================================================================== */

/**
 * @safety_req SWR-CVC-016
 */
Std_ReturnType Swc_CvcCom_Receive(uint16 canId, const uint8* payload,
                                   uint8 length)
{
    uint8 i;

    if (CvcCom_Initialized != TRUE)
    {
        return E_NOT_OK;
    }

    if (payload == NULL_PTR)
    {
        return E_NOT_OK;
    }

    /* Look up CAN ID in routing table */
    for (i = 0u; i < CVCCOM_RX_TABLE_SIZE; i++)
    {
        if (CvcCom_RxTable[i].canId == canId)
        {
            /* Found: apply E2E check */
            return Swc_CvcCom_E2eCheck(i, payload, length);
        }
    }

    /* Unknown CAN ID — not in routing table */
    return E_NOT_OK;
}

/* ==================================================================
 * API: Swc_CvcCom_TransmitSchedule
 * ================================================================== */

/**
 * @safety_req SWR-CVC-017
 */
void Swc_CvcCom_TransmitSchedule(uint32 currentTimeMs)
{
    uint8  i;
    uint32 elapsed;

    if (CvcCom_Initialized != TRUE)
    {
        return;
    }

    for (i = 0u; i < CVCCOM_TX_TABLE_SIZE; i++)
    {
        elapsed = currentTimeMs - CvcCom_TxLastTimeMs[i];

        if (elapsed >= (uint32)CvcCom_TxTable[i].periodMs)
        {
            CvcCom_TxLastTimeMs[i] = currentTimeMs;

            /* Transmit is handled by the caller after E2E protect.
             * Mark the slot as due for transmission. */
        }
    }
}

/* ==================================================================
 * API: Swc_CvcCom_GetRxStatus
 * ================================================================== */

Std_ReturnType Swc_CvcCom_GetRxStatus(uint8 rxIndex,
                                       Swc_CvcCom_RxStatusType* status)
{
    if (CvcCom_Initialized != TRUE)
    {
        return E_NOT_OK;
    }

    if (rxIndex >= CVCCOM_RX_TABLE_SIZE)
    {
        return E_NOT_OK;
    }

    if (status == NULL_PTR)
    {
        return E_NOT_OK;
    }

    *status = CvcCom_RxStatus[rxIndex];
    return E_OK;
}
