/**
 * @file    test_Swc_DtcDisplay.c
 * @brief   Unit tests for Swc_DtcDisplay — DTC list management for ICU
 * @date    2026-02-23
 *
 * @verifies SWR-ICU-008
 *
 * Tests DTC display initialization, adding DTCs from broadcast, circular
 * buffer overflow (oldest replaced), active/pending status flags, and
 * clear-all operation.
 *
 * Data processing logic ONLY — ncurses rendering is excluded via
 * PLATFORM_POSIX_TEST guard.
 *
 * Mocks: Rte_Read
 */
#include "unity.h"

/* ====================================================================
 * Local type definitions (self-contained test — no BSW headers)
 * ==================================================================== */

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned long  uint32;
typedef uint8          Std_ReturnType;
typedef uint8          boolean;

#define E_OK      0u
#define E_NOT_OK  1u
#define TRUE      1u
#define FALSE     0u
#define NULL_PTR  ((void*)0)

/* Prevent BSW headers from redefining types */
#define PLATFORM_TYPES_H
#define STD_TYPES_H
#define SWC_DTCDISPLAY_H
#define ICU_CFG_H

/* Prevent ncurses inclusion in test builds */
#define PLATFORM_POSIX_TEST

/* ====================================================================
 * Signal IDs (must match Icu_Cfg.h)
 * ==================================================================== */

#define ICU_SIG_DTC_BROADCAST     28u
#define ICU_SIG_COUNT             29u

/* DTC buffer size */
#define DTC_MAX_ENTRIES           16u

/* ====================================================================
 * DTC Entry type (must match Swc_DtcDisplay.h)
 * ==================================================================== */

typedef struct {
    uint32  code;      /* 24-bit DTC code */
    uint8   status;    /* ISO 14229 status byte */
    uint32  timestamp; /* Tick count when received */
} DtcEntry_t;

/* ====================================================================
 * Mock: Rte_Read — inject signal values via array
 * ==================================================================== */

#define MOCK_RTE_MAX_SIGNALS  64u

static uint32 mock_rte_signals[MOCK_RTE_MAX_SIGNALS];

Std_ReturnType Rte_Read(uint16 SignalId, uint32* DataPtr)
{
    if (DataPtr == NULL_PTR) {
        return E_NOT_OK;
    }
    if (SignalId < MOCK_RTE_MAX_SIGNALS) {
        *DataPtr = mock_rte_signals[SignalId];
        return E_OK;
    }
    return E_NOT_OK;
}

/* ====================================================================
 * Include source under test (source inclusion for test build)
 * ==================================================================== */

#include "../src/Swc_DtcDisplay.c"

/* ====================================================================
 * Helper: encode DTC broadcast value
 * Format: bits 31..8 = 24-bit DTC code, bits 7..0 = status byte
 * ==================================================================== */

static uint32 encode_dtc_broadcast(uint32 dtc_code, uint8 status)
{
    return ((dtc_code & 0x00FFFFFFu) << 8u) | (uint32)status;
}

/* ====================================================================
 * setUp / tearDown
 * ==================================================================== */

void setUp(void)
{
    uint8 i;

    for (i = 0u; i < MOCK_RTE_MAX_SIGNALS; i++) {
        mock_rte_signals[i] = 0u;
    }

    Swc_DtcDisplay_Init();
}

void tearDown(void) { }

/* ====================================================================
 * SWR-ICU-008: Init — empty DTC list
 * ==================================================================== */

/** @verifies SWR-ICU-008 */
void test_DtcDisplay_init_empty(void)
{
    TEST_ASSERT_EQUAL_UINT8(0u, Swc_DtcDisplay_GetCount());

    /* All entries should be zeroed */
    TEST_ASSERT_TRUE(Swc_DtcDisplay_GetEntry(0u) == NULL_PTR);
}

/* ====================================================================
 * SWR-ICU-008: Add DTC from broadcast signal
 * ==================================================================== */

/** @verifies SWR-ICU-008 */
void test_DtcDisplay_add_dtc_from_broadcast(void)
{
    const DtcEntry_t* entry;

    /* Broadcast a DTC: code 0xC00100, status = 0x09 (testFailed + confirmedDTC) */
    mock_rte_signals[ICU_SIG_DTC_BROADCAST] = encode_dtc_broadcast(0xC00100u, 0x09u);

    Swc_DtcDisplay_50ms();

    TEST_ASSERT_EQUAL_UINT8(1u, Swc_DtcDisplay_GetCount());

    entry = Swc_DtcDisplay_GetEntry(0u);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_UINT32(0xC00100u, entry->code);
    TEST_ASSERT_EQUAL_UINT8(0x09u, entry->status);
}

/* ====================================================================
 * SWR-ICU-008: Duplicate DTC updates status
 * ==================================================================== */

/** @verifies SWR-ICU-008 */
void test_DtcDisplay_duplicate_updates_status(void)
{
    const DtcEntry_t* entry;

    /* First broadcast: active */
    mock_rte_signals[ICU_SIG_DTC_BROADCAST] = encode_dtc_broadcast(0xC00100u, 0x01u);
    Swc_DtcDisplay_50ms();
    TEST_ASSERT_EQUAL_UINT8(1u, Swc_DtcDisplay_GetCount());

    /* Second broadcast: same DTC, now confirmed */
    mock_rte_signals[ICU_SIG_DTC_BROADCAST] = encode_dtc_broadcast(0xC00100u, 0x09u);
    Swc_DtcDisplay_50ms();

    /* Count should still be 1 (updated, not added) */
    TEST_ASSERT_EQUAL_UINT8(1u, Swc_DtcDisplay_GetCount());

    entry = Swc_DtcDisplay_GetEntry(0u);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_UINT8(0x09u, entry->status);
}

/* ====================================================================
 * SWR-ICU-008: Max 16 DTCs — oldest replaced when full
 * ==================================================================== */

/** @verifies SWR-ICU-008 */
void test_DtcDisplay_max_16_dtcs(void)
{
    uint8 i;
    const DtcEntry_t* entry;

    /* Fill all 16 slots with unique DTCs */
    for (i = 0u; i < DTC_MAX_ENTRIES; i++) {
        mock_rte_signals[ICU_SIG_DTC_BROADCAST] =
            encode_dtc_broadcast((uint32)(0xD00000u + i), 0x01u);
        Swc_DtcDisplay_50ms();

        /* Clear broadcast so next cycle doesn't re-add */
        mock_rte_signals[ICU_SIG_DTC_BROADCAST] = 0u;
        Swc_DtcDisplay_50ms();
    }

    TEST_ASSERT_EQUAL_UINT8(DTC_MAX_ENTRIES, Swc_DtcDisplay_GetCount());

    /* Add 17th DTC — should replace oldest (index 0) */
    mock_rte_signals[ICU_SIG_DTC_BROADCAST] =
        encode_dtc_broadcast(0xE00000u, 0x09u);
    Swc_DtcDisplay_50ms();

    TEST_ASSERT_EQUAL_UINT8(DTC_MAX_ENTRIES, Swc_DtcDisplay_GetCount());

    /* The new DTC should be present somewhere in the list */
    {
        boolean found = FALSE;
        for (i = 0u; i < DTC_MAX_ENTRIES; i++) {
            entry = Swc_DtcDisplay_GetEntry(i);
            if ((entry != NULL_PTR) && (entry->code == 0xE00000u)) {
                found = TRUE;
                TEST_ASSERT_EQUAL_UINT8(0x09u, entry->status);
            }
        }
        TEST_ASSERT_TRUE(found);
    }
}

/* ====================================================================
 * SWR-ICU-008: Active status flag — bit 0 (testFailed)
 * ==================================================================== */

/** @verifies SWR-ICU-008 */
void test_DtcDisplay_active_status_flag(void)
{
    const DtcEntry_t* entry;

    /* Status byte with bit 0 set = testFailed (active) */
    mock_rte_signals[ICU_SIG_DTC_BROADCAST] = encode_dtc_broadcast(0xC10100u, 0x01u);
    Swc_DtcDisplay_50ms();

    entry = Swc_DtcDisplay_GetEntry(0u);
    TEST_ASSERT_NOT_NULL(entry);

    /* Bit 0: testFailed (active) */
    TEST_ASSERT_TRUE((entry->status & 0x01u) != 0u);
}

/* ====================================================================
 * SWR-ICU-008: Pending status flag — bit 2 (pendingDTC)
 * ==================================================================== */

/** @verifies SWR-ICU-008 */
void test_DtcDisplay_pending_status_flag(void)
{
    const DtcEntry_t* entry;

    /* Status byte with bit 2 set = pendingDTC */
    mock_rte_signals[ICU_SIG_DTC_BROADCAST] = encode_dtc_broadcast(0xC20100u, 0x04u);
    Swc_DtcDisplay_50ms();

    entry = Swc_DtcDisplay_GetEntry(0u);
    TEST_ASSERT_NOT_NULL(entry);

    /* Bit 2: pendingDTC */
    TEST_ASSERT_TRUE((entry->status & 0x04u) != 0u);
}

/* ====================================================================
 * SWR-ICU-008: Clear all DTCs
 * ==================================================================== */

/** @verifies SWR-ICU-008 */
void test_DtcDisplay_clear_all(void)
{
    /* Add a few DTCs */
    mock_rte_signals[ICU_SIG_DTC_BROADCAST] = encode_dtc_broadcast(0xC00100u, 0x01u);
    Swc_DtcDisplay_50ms();
    mock_rte_signals[ICU_SIG_DTC_BROADCAST] = encode_dtc_broadcast(0xC00200u, 0x04u);
    Swc_DtcDisplay_50ms();

    TEST_ASSERT_EQUAL_UINT8(2u, Swc_DtcDisplay_GetCount());

    /* Clear all */
    Swc_DtcDisplay_ClearAll();

    TEST_ASSERT_EQUAL_UINT8(0u, Swc_DtcDisplay_GetCount());
    TEST_ASSERT_TRUE(Swc_DtcDisplay_GetEntry(0u) == NULL_PTR);
}

/* ====================================================================
 * Test runner
 * ==================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* SWR-ICU-008: DTC display management */
    RUN_TEST(test_DtcDisplay_init_empty);
    RUN_TEST(test_DtcDisplay_add_dtc_from_broadcast);
    RUN_TEST(test_DtcDisplay_duplicate_updates_status);
    RUN_TEST(test_DtcDisplay_max_16_dtcs);
    RUN_TEST(test_DtcDisplay_active_status_flag);
    RUN_TEST(test_DtcDisplay_pending_status_flag);
    RUN_TEST(test_DtcDisplay_clear_all);

    return UNITY_END();
}
