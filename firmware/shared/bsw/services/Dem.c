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

/* ---- Internal State ---- */

typedef struct {
    sint16  debounceCounter;
    uint8   statusByte;
    uint32  occurrenceCounter;
} Dem_EventDataType;

static Dem_EventDataType dem_events[DEM_MAX_EVENTS];

/* ---- API Implementation ---- */

void Dem_Init(const void* ConfigPtr)
{
    uint8 i;
    (void)ConfigPtr;

    for (i = 0u; i < DEM_MAX_EVENTS; i++) {
        dem_events[i].debounceCounter  = 0;
        dem_events[i].statusByte       = 0u;
        dem_events[i].occurrenceCounter = 0u;
    }
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
    }

    return E_OK;
}
