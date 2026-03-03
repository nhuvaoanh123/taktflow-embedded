/**
 * @file    Det.c
 * @brief   Default Error Tracer implementation
 * @date    2026-03-03
 *
 * @details Ring buffer stores the last DET_LOG_SIZE errors. In POSIX/SIL
 *          builds, every error is also printed to stderr via fprintf
 *          (Docker stdout is fully buffered — stderr is line-buffered).
 *
 * @safety_req SWR-BSW-040
 * @traces_to  TSR-022, TSR-038
 *
 * @standard AUTOSAR_SWS_DefaultErrorTracer, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#include "Det.h"

#if defined(PLATFORM_POSIX) && !defined(UNIT_TEST)
#include <stdio.h>
#endif

/* ---- Internal State ---- */

static Det_ErrorEntryType det_log[DET_LOG_SIZE];
static uint8              det_log_head;     /**< Next write position    */
static uint8              det_log_count;    /**< Entries in buffer      */
static uint16             det_error_count;  /**< Total errors reported  */
static boolean            det_initialized;
static Det_CallbackType   det_callback;

/* ---- Module Name Lookup (for SIL_DIAG output) ---- */

#if defined(PLATFORM_POSIX) && !defined(UNIT_TEST)
static const char* det_module_name(uint16 id)
{
    switch (id) {
    case DET_MODULE_CAN:    return "Can";
    case DET_MODULE_CANIF:  return "CanIf";
    case DET_MODULE_PDUR:   return "PduR";
    case DET_MODULE_COM:    return "Com";
    case DET_MODULE_DCM:    return "Dcm";
    case DET_MODULE_DEM:    return "Dem";
    case DET_MODULE_WDGM:   return "WdgM";
    case DET_MODULE_BSWM:   return "BswM";
    case DET_MODULE_E2E:    return "E2E";
    case DET_MODULE_RTE:    return "Rte";
    case DET_MODULE_SPI:    return "Spi";
    case DET_MODULE_ADC:    return "Adc";
    case DET_MODULE_DIO:    return "Dio";
    case DET_MODULE_GPT:    return "Gpt";
    case DET_MODULE_PWM:    return "Pwm";
    case DET_MODULE_IOHWAB: return "IoHwAb";
    case DET_MODULE_UART:   return "Uart";
    case DET_MODULE_NVM:    return "NvM";
    case DET_MODULE_CANTP:  return "CanTp";
    default:                return "Unknown";
    }
}

static const char* det_error_name(uint8 id)
{
    switch (id) {
    case DET_E_PARAM_POINTER: return "PARAM_POINTER";
    case DET_E_UNINIT:        return "UNINIT";
    case DET_E_PARAM_VALUE:   return "PARAM_VALUE";
    default:                  return "UNKNOWN";
    }
}
#endif

/* ---- API Implementation ---- */

void Det_Init(void)
{
    uint8 i;

    for (i = 0u; i < DET_LOG_SIZE; i++) {
        det_log[i].ModuleId   = 0u;
        det_log[i].InstanceId = 0u;
        det_log[i].ApiId      = 0u;
        det_log[i].ErrorId    = 0u;
    }

    det_log_head    = 0u;
    det_log_count   = 0u;
    det_error_count = 0u;
    det_callback    = NULL_PTR;
    det_initialized = TRUE;
}

void Det_ReportError(uint16 ModuleId, uint8 InstanceId,
                     uint8 ApiId, uint8 ErrorId)
{
    if (det_initialized == FALSE) {
        return;
    }

    /* Store in ring buffer */
    det_log[det_log_head].ModuleId   = ModuleId;
    det_log[det_log_head].InstanceId = InstanceId;
    det_log[det_log_head].ApiId      = ApiId;
    det_log[det_log_head].ErrorId    = ErrorId;

    det_log_head = (det_log_head + 1u) % DET_LOG_SIZE;

    if (det_log_count < DET_LOG_SIZE) {
        det_log_count++;
    }

    /* Saturating total counter */
    if (det_error_count < 0xFFFFu) {
        det_error_count++;
    }

    /* SIL_DIAG output (POSIX only, not in unit tests) */
#if defined(PLATFORM_POSIX) && !defined(UNIT_TEST)
    fprintf(stderr, "[DET] %s(Inst=%u) Api=0x%02X Err=%s(0x%02X)\n",
            det_module_name(ModuleId), InstanceId,
            ApiId, det_error_name(ErrorId), ErrorId);
#endif

    /* User callback */
    if (det_callback != NULL_PTR) {
        det_callback(ModuleId, InstanceId, ApiId, ErrorId);
    }
}

void Det_ReportRuntimeError(uint16 ModuleId, uint8 InstanceId,
                            uint8 ApiId, uint8 ErrorId)
{
    /* Same implementation — AUTOSAR distinguishes development vs runtime
     * errors conceptually but the logging path is identical */
    Det_ReportError(ModuleId, InstanceId, ApiId, ErrorId);
}

uint16 Det_GetErrorCount(void)
{
    return det_error_count;
}

Std_ReturnType Det_GetLogEntry(uint8 Index, Det_ErrorEntryType* EntryPtr)
{
    uint8 actual_index;

    if (EntryPtr == NULL_PTR) {
        return E_NOT_OK;
    }

    if (Index >= det_log_count) {
        return E_NOT_OK;
    }

    /* Ring buffer: oldest entry is at (head - count), wrapped */
    if (det_log_count < DET_LOG_SIZE) {
        actual_index = Index;
    } else {
        actual_index = (det_log_head + Index) % DET_LOG_SIZE;
    }

    EntryPtr->ModuleId   = det_log[actual_index].ModuleId;
    EntryPtr->InstanceId = det_log[actual_index].InstanceId;
    EntryPtr->ApiId      = det_log[actual_index].ApiId;
    EntryPtr->ErrorId    = det_log[actual_index].ErrorId;

    return E_OK;
}

void Det_SetCallback(Det_CallbackType Callback)
{
    det_callback = Callback;
}
