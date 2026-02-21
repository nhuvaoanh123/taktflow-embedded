/**
 * @file    CanIf.c
 * @brief   CAN Interface implementation
 * @date    2026-02-21
 *
 * @safety_req SWR-BSW-011, SWR-BSW-012
 * @traces_to  TSR-022, TSR-023, TSR-024
 *
 * @standard AUTOSAR_SWS_CANInterface, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#include "CanIf.h"

/* ---- Internal State ---- */

static const CanIf_ConfigType* canif_config = NULL_PTR;
static boolean canif_initialized = FALSE;

/* ---- API Implementation ---- */

void CanIf_Init(const CanIf_ConfigType* ConfigPtr)
{
    if (ConfigPtr == NULL_PTR) {
        canif_initialized = FALSE;
        canif_config = NULL_PTR;
        return;
    }

    canif_config = ConfigPtr;
    canif_initialized = TRUE;
}

Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    Can_PduType can_pdu;

    if ((canif_initialized == FALSE) || (canif_config == NULL_PTR)) {
        return E_NOT_OK;
    }

    if (PduInfoPtr == NULL_PTR) {
        return E_NOT_OK;
    }

    if (TxPduId >= canif_config->txPduCount) {
        return E_NOT_OK;
    }

    /* Map PDU ID to CAN ID */
    const CanIf_TxPduConfigType* tx_cfg = &canif_config->txPduConfig[TxPduId];

    can_pdu.id     = tx_cfg->CanId;
    can_pdu.length = PduInfoPtr->SduLength;
    can_pdu.sdu    = PduInfoPtr->SduDataPtr;

    Can_ReturnType result = Can_Write(tx_cfg->Hth, &can_pdu);

    return (result == CAN_OK) ? E_OK : E_NOT_OK;
}

void CanIf_RxIndication(Can_IdType CanId, const uint8* SduPtr, uint8 Dlc)
{
    PduInfoType pdu_info;
    uint8 i;

    if ((canif_initialized == FALSE) || (canif_config == NULL_PTR)) {
        return;
    }

    if (SduPtr == NULL_PTR) {
        return;
    }

    /* Look up CAN ID in RX routing table */
    for (i = 0u; i < canif_config->rxPduCount; i++) {
        if (canif_config->rxPduConfig[i].CanId == CanId) {
            /* Found — route to PduR */
            pdu_info.SduDataPtr = (uint8*)SduPtr; /* const-cast for AUTOSAR API */
            pdu_info.SduLength  = Dlc;

            PduR_CanIfRxIndication(canif_config->rxPduConfig[i].UpperPduId,
                                   &pdu_info);
            return;
        }
    }

    /* Unknown CAN ID — silently discard per SWR-BSW-011 */
}
