/**
 * @file    test_Swc_DtcStore.c
 * @brief   Unit tests for DTC Store SWC -- in-memory DTC management
 * @date    2026-02-23
 *
 * @verifies SWR-TCU-008, SWR-TCU-009
 *
 * Tests DTC storage initialization, add/update, maximum capacity, freeze-frame
 * capture, aging counter, clear-all, and auto-capture from DTC broadcast.
 *
 * Mocks: Rte_Read, Rte_Write
 */
#include "unity.h"

/* ====================================================================
 * Local type definitions (self-contained test -- no BSW headers)
 * ==================================================================== */

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned long  uint32;
typedef signed short   sint16;
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
#define SWC_DTCSTORE_H
#define TCU_CFG_H

/* ====================================================================
 * Signal IDs (must match Tcu_Cfg.h)
 * ==================================================================== */

#define TCU_SIG_VEHICLE_SPEED     16u
#define TCU_SIG_MOTOR_TEMP        17u
#define TCU_SIG_BATTERY_VOLTAGE   18u
#define TCU_SIG_MOTOR_CURRENT     19u
#define TCU_SIG_TORQUE_PCT        20u
#define TCU_SIG_MOTOR_RPM         21u
#define TCU_SIG_DTC_BROADCAST     22u
#define TCU_SIG_COUNT             23u

/* DTC Store config */
#define DTC_STORE_MAX_ENTRIES   64u

#define DTC_STATUS_TEST_FAILED          0x01u
#define DTC_STATUS_TEST_FAILED_THIS_OP  0x02u
#define DTC_STATUS_PENDING              0x04u
#define DTC_STATUS_CONFIRMED            0x08u

/* DTC aging threshold */
#define TCU_DTC_AGING_CLEAR_CYCLES  40u

/* ====================================================================
 * Mock: Rte_Read / Rte_Write
 * ==================================================================== */

#define MOCK_RTE_MAX_SIGNALS  32u

static uint32 mock_rte_signals[MOCK_RTE_MAX_SIGNALS];
static uint32 mock_tick_counter;

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

static uint16 mock_rte_write_sig;
static uint32 mock_rte_write_val;
static uint8  mock_rte_write_count;

Std_ReturnType Rte_Write(uint16 SignalId, uint32 Data)
{
    mock_rte_write_sig = SignalId;
    mock_rte_write_val = Data;
    mock_rte_write_count++;
    if (SignalId < MOCK_RTE_MAX_SIGNALS) {
        mock_rte_signals[SignalId] = Data;
        return E_OK;
    }
    return E_NOT_OK;
}

/* ====================================================================
 * Include SWC under test (source inclusion for test build)
 * ==================================================================== */

#include "../src/Swc_DtcStore.c"

/* ====================================================================
 * setUp / tearDown
 * ==================================================================== */

void setUp(void)
{
    uint8 i;

    for (i = 0u; i < MOCK_RTE_MAX_SIGNALS; i++) {
        mock_rte_signals[i] = 0u;
    }

    mock_rte_write_sig   = 0u;
    mock_rte_write_val   = 0u;
    mock_rte_write_count = 0u;
    mock_tick_counter    = 0u;

    Swc_DtcStore_Init();
}

void tearDown(void) { }

/* ====================================================================
 * SWR-TCU-008: Init -- store is empty
 * ==================================================================== */

/** @verifies SWR-TCU-008 */
void test_DtcStore_init_empty(void)
{
    TEST_ASSERT_EQUAL_UINT8(0u, Swc_DtcStore_GetCount());
}

/* ====================================================================
 * SWR-TCU-008: Add a DTC
 * ==================================================================== */

/** @verifies SWR-TCU-008 */
void test_DtcStore_add_dtc(void)
{
    /* Set freeze-frame source signals */
    mock_rte_signals[TCU_SIG_VEHICLE_SPEED]   = 55u;
    mock_rte_signals[TCU_SIG_MOTOR_CURRENT]   = 120u;
    mock_rte_signals[TCU_SIG_BATTERY_VOLTAGE] = 48000u;
    mock_rte_signals[TCU_SIG_MOTOR_TEMP]      = 65u;

    Std_ReturnType result = Swc_DtcStore_Add(0x001234u, DTC_STATUS_CONFIRMED);
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    TEST_ASSERT_EQUAL_UINT8(1u, Swc_DtcStore_GetCount());

    const DtcStoreEntry_t* entry = Swc_DtcStore_GetByIndex(0u);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_UINT32(0x001234u, entry->dtcCode);
    TEST_ASSERT_EQUAL_UINT8(DTC_STATUS_CONFIRMED, entry->status);
}

/* ====================================================================
 * SWR-TCU-008: Max 64 entries -- does not exceed capacity
 * ==================================================================== */

/** @verifies SWR-TCU-008 */
void test_DtcStore_max_64_entries(void)
{
    uint8 i;
    for (i = 0u; i < DTC_STORE_MAX_ENTRIES; i++) {
        Swc_DtcStore_Add((uint32)i + 1u, DTC_STATUS_CONFIRMED);
    }
    TEST_ASSERT_EQUAL_UINT8(DTC_STORE_MAX_ENTRIES, Swc_DtcStore_GetCount());

    /* Adding one more should still succeed (wraps, replacing oldest) */
    Std_ReturnType result = Swc_DtcStore_Add(0xFFFFu, DTC_STATUS_CONFIRMED);
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    /* Count should remain at max */
    TEST_ASSERT_EQUAL_UINT8(DTC_STORE_MAX_ENTRIES, Swc_DtcStore_GetCount());
}

/* ====================================================================
 * SWR-TCU-008: Duplicate DTC updates status
 * ==================================================================== */

/** @verifies SWR-TCU-008 */
void test_DtcStore_duplicate_updates_status(void)
{
    Swc_DtcStore_Add(0x001234u, DTC_STATUS_PENDING);
    TEST_ASSERT_EQUAL_UINT8(1u, Swc_DtcStore_GetCount());

    /* Add same DTC code again with different status */
    Swc_DtcStore_Add(0x001234u, DTC_STATUS_CONFIRMED);
    /* Should NOT create a new entry */
    TEST_ASSERT_EQUAL_UINT8(1u, Swc_DtcStore_GetCount());

    const DtcStoreEntry_t* entry = Swc_DtcStore_GetByIndex(0u);
    TEST_ASSERT_NOT_NULL(entry);
    /* Status should be updated (OR'd) */
    TEST_ASSERT_BITS(DTC_STATUS_CONFIRMED, DTC_STATUS_CONFIRMED, entry->status);
}

/* ====================================================================
 * SWR-TCU-009: Freeze frame capture at first detection
 * ==================================================================== */

/** @verifies SWR-TCU-009 */
void test_DtcStore_freeze_frame_capture(void)
{
    mock_rte_signals[TCU_SIG_VEHICLE_SPEED]   = 88u;
    mock_rte_signals[TCU_SIG_MOTOR_CURRENT]   = 200u;
    mock_rte_signals[TCU_SIG_BATTERY_VOLTAGE] = 48500u;
    mock_rte_signals[TCU_SIG_MOTOR_TEMP]      = 72u;

    Swc_DtcStore_Add(0xABCDEFu, DTC_STATUS_TEST_FAILED);

    const DtcStoreEntry_t* entry = Swc_DtcStore_GetByIndex(0u);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_UINT16(88u,    entry->ff_speed);
    TEST_ASSERT_EQUAL_UINT16(200u,   entry->ff_current);
    TEST_ASSERT_EQUAL_UINT16(48500u, entry->ff_voltage);
    TEST_ASSERT_EQUAL_UINT8(72u,     entry->ff_temp);
}

/* ====================================================================
 * SWR-TCU-008: Get count
 * ==================================================================== */

/** @verifies SWR-TCU-008 */
void test_DtcStore_get_count(void)
{
    TEST_ASSERT_EQUAL_UINT8(0u, Swc_DtcStore_GetCount());

    Swc_DtcStore_Add(0x000001u, DTC_STATUS_CONFIRMED);
    TEST_ASSERT_EQUAL_UINT8(1u, Swc_DtcStore_GetCount());

    Swc_DtcStore_Add(0x000002u, DTC_STATUS_CONFIRMED);
    TEST_ASSERT_EQUAL_UINT8(2u, Swc_DtcStore_GetCount());
}

/* ====================================================================
 * SWR-TCU-008: Get by index
 * ==================================================================== */

/** @verifies SWR-TCU-008 */
void test_DtcStore_get_by_index(void)
{
    Swc_DtcStore_Add(0x000AAu, DTC_STATUS_CONFIRMED);
    Swc_DtcStore_Add(0x000BBu, DTC_STATUS_PENDING);

    const DtcStoreEntry_t* e0 = Swc_DtcStore_GetByIndex(0u);
    const DtcStoreEntry_t* e1 = Swc_DtcStore_GetByIndex(1u);
    const DtcStoreEntry_t* eX = Swc_DtcStore_GetByIndex(99u);

    TEST_ASSERT_NOT_NULL(e0);
    TEST_ASSERT_NOT_NULL(e1);
    TEST_ASSERT_NULL(eX);

    TEST_ASSERT_EQUAL_UINT32(0x000AAu, e0->dtcCode);
    TEST_ASSERT_EQUAL_UINT32(0x000BBu, e1->dtcCode);
}

/* ====================================================================
 * SWR-TCU-008: Clear all DTCs
 * ==================================================================== */

/** @verifies SWR-TCU-008 */
void test_DtcStore_clear_all(void)
{
    Swc_DtcStore_Add(0x000001u, DTC_STATUS_CONFIRMED);
    Swc_DtcStore_Add(0x000002u, DTC_STATUS_CONFIRMED);
    TEST_ASSERT_EQUAL_UINT8(2u, Swc_DtcStore_GetCount());

    Swc_DtcStore_Clear();
    TEST_ASSERT_EQUAL_UINT8(0u, Swc_DtcStore_GetCount());
}

/* ====================================================================
 * SWR-TCU-009: Aging counter -- increment when status not failing
 * ==================================================================== */

/** @verifies SWR-TCU-009 */
void test_DtcStore_aging_counter(void)
{
    /* Add a DTC that is confirmed but not currently failing */
    Swc_DtcStore_Add(0x000001u, DTC_STATUS_CONFIRMED);

    /* Get the entry and verify aging counter starts at 0 */
    const DtcStoreEntry_t* entry = Swc_DtcStore_GetByIndex(0u);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_UINT16(0u, entry->agingCounter);

    /* Run 10ms cycles -- DTC broadcast is 0 (no new faults) */
    mock_rte_signals[TCU_SIG_DTC_BROADCAST] = 0u;
    uint16 cycle;
    for (cycle = 0u; cycle < 10u; cycle++) {
        Swc_DtcStore_10ms();
    }

    /* Aging counter should have incremented */
    entry = Swc_DtcStore_GetByIndex(0u);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_TRUE(entry->agingCounter > 0u);
}

/* ====================================================================
 * SWR-TCU-009: Auto-capture from DTC broadcast
 * ==================================================================== */

/** @verifies SWR-TCU-009 */
void test_DtcStore_auto_capture_from_broadcast(void)
{
    /* Simulate a DTC broadcast: non-zero value encodes DTC code */
    mock_rte_signals[TCU_SIG_DTC_BROADCAST]   = 0x005678u;
    mock_rte_signals[TCU_SIG_VEHICLE_SPEED]   = 30u;
    mock_rte_signals[TCU_SIG_MOTOR_CURRENT]   = 100u;
    mock_rte_signals[TCU_SIG_BATTERY_VOLTAGE] = 48000u;
    mock_rte_signals[TCU_SIG_MOTOR_TEMP]      = 55u;

    /* Run one 10ms cycle */
    Swc_DtcStore_10ms();

    /* DTC should have been auto-captured */
    TEST_ASSERT_EQUAL_UINT8(1u, Swc_DtcStore_GetCount());

    const DtcStoreEntry_t* entry = Swc_DtcStore_GetByIndex(0u);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_UINT32(0x005678u, entry->dtcCode);
    TEST_ASSERT_EQUAL_UINT16(30u, entry->ff_speed);
}

/* ====================================================================
 * Test runner
 * ==================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* SWR-TCU-008: DTC storage management */
    RUN_TEST(test_DtcStore_init_empty);
    RUN_TEST(test_DtcStore_add_dtc);
    RUN_TEST(test_DtcStore_max_64_entries);
    RUN_TEST(test_DtcStore_duplicate_updates_status);
    RUN_TEST(test_DtcStore_get_count);
    RUN_TEST(test_DtcStore_get_by_index);
    RUN_TEST(test_DtcStore_clear_all);

    /* SWR-TCU-009: Freeze frames and aging */
    RUN_TEST(test_DtcStore_freeze_frame_capture);
    RUN_TEST(test_DtcStore_aging_counter);
    RUN_TEST(test_DtcStore_auto_capture_from_broadcast);

    return UNITY_END();
}
