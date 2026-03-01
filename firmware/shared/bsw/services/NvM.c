/**
 * @file    NvM.c
 * @brief   NVRAM Manager — non-volatile memory block read/write
 * @date    2026-02-25
 *
 * @safety_req SWR-BSW-031
 * @traces_to  TSR-050
 *
 * @details  Dual-mode implementation:
 *           - POSIX builds (SIL/MIL): file-backed storage in /tmp/nvm_block_{id}.bin
 *           - Target builds: no-op stub (real flash driver to be integrated)
 *
 *           Reusable across all ECUs. Block size fixed at NVM_BLOCK_SIZE bytes.
 *
 * @standard AUTOSAR_SWS_NVRAMManager, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */

#include "NvM.h"

/* ---- POSIX file-backed implementation (SIL / MIL / PIL host) ---- */
#if defined(__unix__) || defined(__linux__) || defined(__APPLE__) || \
    defined(__POSIX__) || defined(POSIX_BUILD)

#include <stdio.h>
#include <string.h>

#define NVM_BLOCK_SIZE  1024u   /* Max bytes per NvM block */

Std_ReturnType NvM_ReadBlock(NvM_BlockIdType BlockId, void* NvM_DstPtr)
{
    char path[64];
    FILE* fp;
    size_t n;

    if (NvM_DstPtr == NULL_PTR) {
        return E_NOT_OK;
    }

    (void)snprintf(path, sizeof(path), "/tmp/nvm_block_%u.bin",
                   (unsigned)BlockId);

    fp = fopen(path, "rb");
    if (fp == NULL) {
        /* File not found — first boot, no stored data */
        return E_OK;
    }

    n = fread(NvM_DstPtr, 1u, NVM_BLOCK_SIZE, fp);
    (void)fclose(fp);
    (void)n;

    return E_OK;
}

Std_ReturnType NvM_WriteBlock(NvM_BlockIdType BlockId, const void* NvM_SrcPtr)
{
    char path[64];
    FILE* fp;

    if (NvM_SrcPtr == NULL_PTR) {
        return E_NOT_OK;
    }

    (void)snprintf(path, sizeof(path), "/tmp/nvm_block_%u.bin",
                   (unsigned)BlockId);

    fp = fopen(path, "wb");
    if (fp == NULL) {
        return E_NOT_OK;
    }

    (void)fwrite(NvM_SrcPtr, 1u, NVM_BLOCK_SIZE, fp);
    (void)fclose(fp);

    return E_OK;
}

/* ---- Target stub (real flash driver integration point) ---- */
#else

Std_ReturnType NvM_ReadBlock(NvM_BlockIdType BlockId, void* NvM_DstPtr)
{
    (void)BlockId;
    (void)NvM_DstPtr;
    /* TODO:HARDWARE — integrate with flash/EEPROM driver */
    return E_OK;
}

Std_ReturnType NvM_WriteBlock(NvM_BlockIdType BlockId, const void* NvM_SrcPtr)
{
    (void)BlockId;
    (void)NvM_SrcPtr;
    /* TODO:HARDWARE — integrate with flash/EEPROM driver */
    return E_OK;
}

#endif
