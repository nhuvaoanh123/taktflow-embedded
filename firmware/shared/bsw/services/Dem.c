/**
 * @file    Dem.c
 * @brief   Diagnostic Event Manager implementation
 * @date    2026-02-21
 *
 * @safety_req SWR-BSW-017, SWR-BSW-018
 * @traces_to  TSR-038, TSR-039
 *
 * @standard AUTOSAR_SWS_DiagnosticEventManager, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#include "Dem.h"
#include "ComStack_Types.h"
#include "NvM.h"

/* ---- Forward declaration for PduR_Transmit (avoids circular include) ---- */
extern Std_ReturnType PduR_Transmit(PduIdType TxPduId,
                                     const PduInfoType* PduInfoPtr);

/* ---- NvM block ID for DTC persistence ---- */
#define DEM_NVM_BLOCK_ID            1u

/* ---- DTC-to-UDS code mapping (configurable per ECU via Dem_SetDtcCode) ---- */
static uint32 dem_dtc_codes[DEM_MAX_EVENTS] = {
    0xC00100u, /* 0:  Pedal plausibility */
    0xC00200u, /* 1:  Pedal sensor 1 fail */
    0xC00300u, /* 2:  Pedal sensor 2 fail */
    0xC00400u, /* 3:  Pedal stuck */
    0xC10100u, /* 4:  CAN FZC timeout */
    0xC10200u, /* 5:  CAN RZC timeout */
    0xC10300u, /* 6:  CAN bus-off */
    0xC20100u, /* 7:  Motor overcurrent */
    0xC20200u, /* 8:  Motor overtemp */
    0xC20300u, /* 9:  Motor cutoff RX */
    0xC30100u, /* 10: Brake fault RX */
    0xC30200u, /* 11: Steering fault RX */
    0xC40100u, /* 12: E-stop activated */
    0xC50100u, /* 13: Battery undervolt */
    0xC50200u, /* 14: Battery overvolt */
    0xC60100u, /* 15: NVM CRC fail */
    0xC60200u, /* 16: Self-test fail */
    0xC70100u, /* 17: Display comm */
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u  /* 18-31: reserved */
};

/* Track which DTCs have been broadcast (avoid re-broadcasting same DTC) */
static uint8 dem_broadcast_sent[DEM_MAX_EVENTS];

/* ---- Internal State ---- */

typedef struct {
    sint16  debounceCounter;
    uint8   statusByte;
    uint32  occurrenceCounter;
} Dem_EventDataType;

static Dem_EventDataType dem_events[DEM_MAX_EVENTS];

/* ECU source ID for DTC broadcast (set via Dem_SetEcuId, default 0x00) */
static uint8 dem_ecu_id;

/* ---- API Implementation ---- */

void Dem_Init(const void* ConfigPtr)
{
    uint8 i;
    (void)ConfigPtr;

    for (i = 0u; i < DEM_MAX_EVENTS; i++) {
        dem_events[i].debounceCounter   = 0;
        dem_events[i].statusByte        = 0u;
        dem_events[i].occurrenceCounter = 0u;
        dem_broadcast_sent[i]           = 0u;
    }
    dem_ecu_id = 0u;

    /* Attempt to restore DTCs from NvM */
    (void)NvM_ReadBlock(DEM_NVM_BLOCK_ID, (void*)dem_events);
}

void Dem_ReportErrorStatus(Dem_EventIdType EventId,
                           Dem_EventStatusType EventStatus)
{
    if (EventId >= DEM_MAX_EVENTS) {
        return;
    }

    Dem_EventDataType* ev = &dem_events[EventId];

    if (EventStatus == DEM_EVENT_STATUS_FAILED) {
        /* Increment debounce counter toward fail threshold */
        if (ev->debounceCounter < DEM_DEBOUNCE_FAIL_THRESHOLD) {
            ev->debounceCounter++;
        }

        /* Set testFailed and pendingDTC on first failure (AUTOSAR DEM) */
        ev->statusByte |= DEM_STATUS_TEST_FAILED;
        ev->statusByte |= DEM_STATUS_PENDING_DTC;

        /* Confirm DTC when threshold reached */
        if (ev->debounceCounter >= DEM_DEBOUNCE_FAIL_THRESHOLD) {
            ev->statusByte |= DEM_STATUS_CONFIRMED_DTC;
            ev->occurrenceCounter++;
        }
    } else {
        /* Decrement debounce counter toward pass threshold */
        if (ev->debounceCounter > DEM_DEBOUNCE_PASS_THRESHOLD) {
            ev->debounceCounter--;
        }

        /* Clear testFailed when counter reaches zero (healed) */
        if (ev->debounceCounter <= 0) {
            ev->statusByte &= (uint8)(~DEM_STATUS_TEST_FAILED);
        }

        /* Clamp at pass threshold */
        if (ev->debounceCounter <= DEM_DEBOUNCE_PASS_THRESHOLD) {
            ev->debounceCounter = DEM_DEBOUNCE_PASS_THRESHOLD;
        }
    }
}

Std_ReturnType Dem_GetEventStatus(Dem_EventIdType EventId, uint8* StatusPtr)
{
    if ((EventId >= DEM_MAX_EVENTS) || (StatusPtr == NULL_PTR)) {
        return E_NOT_OK;
    }

    *StatusPtr = dem_events[EventId].statusByte;
    return E_OK;
}

Std_ReturnType Dem_GetOccurrenceCounter(Dem_EventIdType EventId, uint32* CountPtr)
{
    if ((EventId >= DEM_MAX_EVENTS) || (CountPtr == NULL_PTR)) {
        return E_NOT_OK;
    }

    *CountPtr = dem_events[EventId].occurrenceCounter;
    return E_OK;
}

Std_ReturnType Dem_ClearAllDTCs(void)
{
    uint8 i;

    for (i = 0u; i < DEM_MAX_EVENTS; i++) {
        dem_events[i].debounceCounter   = 0;
        dem_events[i].statusByte        = 0u;
        dem_events[i].occurrenceCounter = 0u;
        dem_broadcast_sent[i]           = 0u;
    }

    return E_OK;
}

/* ==================================================================
 * API: Dem_SetEcuId / Dem_SetDtcCode
 * ================================================================== */

void Dem_SetEcuId(uint8 EcuId)
{
    dem_ecu_id = EcuId;
}

void Dem_SetDtcCode(Dem_EventIdType EventId, uint32 DtcCode)
{
    if (EventId < DEM_MAX_EVENTS) {
        dem_dtc_codes[EventId] = DtcCode;
    }
}

/* ==================================================================
 * API: Dem_MainFunction — periodic DTC broadcast on CAN 0x500
 *
 * DTC_Broadcast frame format (8 bytes):
 *   Byte 0-2: DTC code (24-bit UDS DTC number)
 *   Byte 3:   DTC status byte (ISO 14229)
 *   Byte 4:   ECU source ID (0x10 = CVC)
 *   Byte 5:   Occurrence counter (low byte)
 *   Byte 6-7: Reserved (0x00)
 *
 * @safety_req SWR-BSW-017, SWR-BSW-018
 * ================================================================== */

void Dem_MainFunction(void)
{
    uint8 i;
    uint8 pdu_data[8];
    PduInfoType pdu_info;
    uint32 dtc_code;

    pdu_info.SduDataPtr = pdu_data;
    pdu_info.SduLength  = 8u;

    for (i = 0u; i < DEM_MAX_EVENTS; i++) {
        /* Only broadcast newly confirmed DTCs that haven't been sent yet */
        if (((dem_events[i].statusByte & DEM_STATUS_CONFIRMED_DTC) != 0u) &&
            (dem_broadcast_sent[i] == 0u))
        {
            dtc_code = dem_dtc_codes[i];

            if (dtc_code == 0u) {
                continue;  /* Skip unmapped event IDs */
            }

            /* Pack DTC_Broadcast frame */
            pdu_data[0] = (uint8)((dtc_code >> 16u) & 0xFFu);  /* DTC high */
            pdu_data[1] = (uint8)((dtc_code >> 8u) & 0xFFu);   /* DTC mid */
            pdu_data[2] = (uint8)(dtc_code & 0xFFu);            /* DTC low */
            pdu_data[3] = dem_events[i].statusByte;              /* Status */
            pdu_data[4] = dem_ecu_id;                               /* ECU source */
            pdu_data[5] = (uint8)(dem_events[i].occurrenceCounter & 0xFFu);
            pdu_data[6] = 0x00u;
            pdu_data[7] = 0x00u;

            /* Transmit via PduR -> CanIf -> CAN 0x500 */
            (void)PduR_Transmit(0x500u, &pdu_info);

            /* Mark as broadcast — don't re-send until cleared */
            dem_broadcast_sent[i] = 1u;

            /* Persist to NvM */
            (void)NvM_WriteBlock(DEM_NVM_BLOCK_ID,
                                  (const void*)dem_events);
        }
    }
}
