/**
 * @file    NvM.c
 * @brief   NVRAM Manager — non-volatile memory block read/write (stub)
 * @date    2026-02-25
 *
 * @safety_req SWR-BSW-031
 * @traces_to  TSR-050
 *
 * @details  Stub implementation for SIL simulation. Read/Write are no-ops
 *           that return E_OK. No persistent storage is performed.
 *
 * @standard AUTOSAR_SWS_NVRAMManager, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */

#include "NvM.h"

Std_ReturnType NvM_ReadBlock(NvM_BlockIdType BlockId, void* NvM_DstPtr)
{
    (void)BlockId;
    (void)NvM_DstPtr;
    return E_OK;
}

Std_ReturnType NvM_WriteBlock(NvM_BlockIdType BlockId, const void* NvM_SrcPtr)
{
    (void)BlockId;
    (void)NvM_SrcPtr;
    return E_OK;
}
