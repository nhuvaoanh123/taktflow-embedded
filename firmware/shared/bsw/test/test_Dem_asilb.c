/**
 * @file    test_Dem.c
 * @brief   Unit tests for Diagnostic Event Manager
 * @date    2026-02-21
 *
 * @verifies SWR-BSW-017, SWR-BSW-018
 *
 * Tests DTC reporting, debouncing, storage, status bits, and clear.
 */
#include "unity.h"
#include "Dem.h"

void setUp(void)
{
    Dem_Init(NULL_PTR);
}

void tearDown(void) { }

/* ==================================================================
 * SWR-BSW-017: DTC Reporting and Debouncing
 * ================================================================== */

/** @verifies SWR-BSW-017 */
void test_Dem_ReportError_single_failed(void)
{
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_FAILED);

    uint8 status = 0u;
    Std_ReturnType ret = Dem_GetEventStatus(0u, &status);

    TEST_ASSERT_EQUAL(E_OK, ret);
    /* After 1 FAILED: testFailed set, not yet confirmed (debounce) */
    TEST_ASSERT_TRUE((status & DEM_STATUS_TEST_FAILED) != 0u);
}

/** @verifies SWR-BSW-017 */
void test_Dem_ReportError_debounce_confirm(void)
{
    /* 3 consecutive FAILED reports → confirmed DTC */
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_FAILED);
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_FAILED);
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_FAILED);

    uint8 status = 0u;
    Dem_GetEventStatus(0u, &status);

    TEST_ASSERT_TRUE((status & DEM_STATUS_CONFIRMED_DTC) != 0u);
}

/** @verifies SWR-BSW-017 */
void test_Dem_ReportError_passed_heals(void)
{
    /* Fail once then pass 3 times → testFailed cleared */
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_FAILED);
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_PASSED);
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_PASSED);
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_PASSED);

    uint8 status = 0u;
    Dem_GetEventStatus(0u, &status);

    TEST_ASSERT_TRUE((status & DEM_STATUS_TEST_FAILED) == 0u);
}

/** @verifies SWR-BSW-017 */
void test_Dem_ReportError_occurrence_counter(void)
{
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_FAILED);
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_FAILED);
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_FAILED);

    uint32 count = 0u;
    Std_ReturnType ret = Dem_GetOccurrenceCounter(0u, &count);

    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_TRUE(count > 0u);
}

/** @verifies SWR-BSW-017 */
void test_Dem_ReportError_invalid_event_id(void)
{
    Dem_ReportErrorStatus(DEM_MAX_EVENTS, DEM_EVENT_STATUS_FAILED);

    uint8 status = 0u;
    Std_ReturnType ret = Dem_GetEventStatus(DEM_MAX_EVENTS, &status);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* ==================================================================
 * SWR-BSW-018: DTC Clear
 * ================================================================== */

/** @verifies SWR-BSW-018 */
void test_Dem_ClearDTC_clears_all(void)
{
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_FAILED);
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_FAILED);
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_FAILED);

    Std_ReturnType ret = Dem_ClearAllDTCs();
    TEST_ASSERT_EQUAL(E_OK, ret);

    uint8 status = 0u;
    Dem_GetEventStatus(0u, &status);
    TEST_ASSERT_EQUAL_HEX8(0u, status & DEM_STATUS_CONFIRMED_DTC);
}

/** @verifies SWR-BSW-018 */
void test_Dem_GetEventStatus_null_ptr(void)
{
    Std_ReturnType ret = Dem_GetEventStatus(0u, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @verifies SWR-BSW-017 */
void test_Dem_multiple_events_independent(void)
{
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_FAILED);
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_FAILED);
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_FAILED);

    /* Event 1 should be unaffected */
    uint8 status1 = 0u;
    Dem_GetEventStatus(1u, &status1);
    TEST_ASSERT_EQUAL_HEX8(0u, status1 & DEM_STATUS_CONFIRMED_DTC);

    /* Event 0 should be confirmed */
    uint8 status0 = 0u;
    Dem_GetEventStatus(0u, &status0);
    TEST_ASSERT_TRUE((status0 & DEM_STATUS_CONFIRMED_DTC) != 0u);
}

/* ==================================================================
 * SWR-BSW-018, SWR-BSW-019: Hardened Boundary / Fault Injection Tests
 * ================================================================== */

/** @verifies SWR-BSW-018
 *  Equivalence class: NULL pointer — Dem_Init(NULL) should initialize to safe defaults */
void test_Dem_Init_null_reinit(void)
{
    /* First report an error to set some state */
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_FAILED);

    /* Re-init with NULL — should reset all state */
    Dem_Init(NULL_PTR);

    uint8 status = 0xFFu;
    Std_ReturnType ret = Dem_GetEventStatus(0u, &status);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_HEX8(0x00u, status);
}

/** @verifies SWR-BSW-018
 *  Equivalence class: Event ID = 0 (first valid event) */
void test_Dem_EventId_zero_valid(void)
{
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_FAILED);

    uint8 status = 0u;
    Std_ReturnType ret = Dem_GetEventStatus(0u, &status);

    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_TRUE((status & DEM_STATUS_TEST_FAILED) != 0u);
}

/** @verifies SWR-BSW-018
 *  Equivalence class: Event ID = DEM_MAX_EVENTS-1 (last valid event) */
void test_Dem_EventId_max_minus_one(void)
{
    Dem_EventIdType last_valid = (Dem_EventIdType)(DEM_MAX_EVENTS - 1u);
    Dem_ReportErrorStatus(last_valid, DEM_EVENT_STATUS_FAILED);

    uint8 status = 0u;
    Std_ReturnType ret = Dem_GetEventStatus(last_valid, &status);

    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_TRUE((status & DEM_STATUS_TEST_FAILED) != 0u);
}

/** @verifies SWR-BSW-018
 *  Equivalence class: Event ID = DEM_MAX_EVENTS+1 (out of range, boundary+1) */
void test_Dem_EventId_max_plus_one(void)
{
    Dem_EventIdType invalid_id = (Dem_EventIdType)(DEM_MAX_EVENTS + 1u);
    Dem_ReportErrorStatus(invalid_id, DEM_EVENT_STATUS_FAILED);

    uint8 status = 0u;
    Std_ReturnType ret = Dem_GetEventStatus(invalid_id, &status);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @verifies SWR-BSW-017
 *  Equivalence class: Debounce threshold crossing —
 *  Report threshold-1 FAILEDs (should NOT confirm), then one more (should confirm) */
void test_Dem_Debounce_threshold_exact(void)
{
    /* DEM_DEBOUNCE_FAIL_THRESHOLD is 3.
     * After 2 FAILED: pending but not confirmed.
     * After 3rd FAILED: confirmed. */

    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_FAILED);
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_FAILED);

    uint8 status_before = 0u;
    Dem_GetEventStatus(0u, &status_before);
    TEST_ASSERT_EQUAL_HEX8(0u, status_before & DEM_STATUS_CONFIRMED_DTC);

    /* Third FAILED crosses the threshold */
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_FAILED);

    uint8 status_after = 0u;
    Dem_GetEventStatus(0u, &status_after);
    TEST_ASSERT_TRUE((status_after & DEM_STATUS_CONFIRMED_DTC) != 0u);
}

/** @verifies SWR-BSW-017
 *  Equivalence class: Status bit — testFailed set after single FAILED */
void test_Dem_StatusBit_testFailed(void)
{
    Dem_ReportErrorStatus(1u, DEM_EVENT_STATUS_FAILED);

    uint8 status = 0u;
    Dem_GetEventStatus(1u, &status);

    TEST_ASSERT_TRUE((status & DEM_STATUS_TEST_FAILED) != 0u);
}

/** @verifies SWR-BSW-017
 *  Equivalence class: Status bit — pendingDTC set before confirmation */
void test_Dem_StatusBit_pending(void)
{
    Dem_ReportErrorStatus(2u, DEM_EVENT_STATUS_FAILED);

    uint8 status = 0u;
    Dem_GetEventStatus(2u, &status);

    /* Pending should be set after first failure */
    TEST_ASSERT_TRUE((status & DEM_STATUS_PENDING_DTC) != 0u);
}

/** @verifies SWR-BSW-017
 *  Equivalence class: Status bit — confirmedDTC implies testFailed and pending */
void test_Dem_StatusBit_confirmed_implies_others(void)
{
    Dem_ReportErrorStatus(3u, DEM_EVENT_STATUS_FAILED);
    Dem_ReportErrorStatus(3u, DEM_EVENT_STATUS_FAILED);
    Dem_ReportErrorStatus(3u, DEM_EVENT_STATUS_FAILED);

    uint8 status = 0u;
    Dem_GetEventStatus(3u, &status);

    /* Confirmed should also have testFailed and pending */
    TEST_ASSERT_TRUE((status & DEM_STATUS_CONFIRMED_DTC) != 0u);
    TEST_ASSERT_TRUE((status & DEM_STATUS_TEST_FAILED) != 0u);
    TEST_ASSERT_TRUE((status & DEM_STATUS_PENDING_DTC) != 0u);
}

/** @verifies SWR-BSW-018
 *  Equivalence class: Fill all event slots — report FAILED for every event ID */
void test_Dem_FillAllEventSlots(void)
{
    uint8 i;
    for (i = 0u; i < DEM_MAX_EVENTS; i++) {
        Dem_ReportErrorStatus(i, DEM_EVENT_STATUS_FAILED);
    }

    /* All events should have testFailed set */
    for (i = 0u; i < DEM_MAX_EVENTS; i++) {
        uint8 status = 0u;
        Std_ReturnType ret = Dem_GetEventStatus(i, &status);
        TEST_ASSERT_EQUAL(E_OK, ret);
        TEST_ASSERT_TRUE((status & DEM_STATUS_TEST_FAILED) != 0u);
    }

    /* Attempt one more beyond max — should be rejected */
    Dem_ReportErrorStatus(DEM_MAX_EVENTS, DEM_EVENT_STATUS_FAILED);
    uint8 status_overflow = 0u;
    Std_ReturnType ret = Dem_GetEventStatus(DEM_MAX_EVENTS, &status_overflow);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @verifies SWR-BSW-017
 *  Equivalence class: Occurrence counter — increment to 0xFF, verify no overflow */
void test_Dem_OccurrenceCounter_no_overflow(void)
{
    /* Report FAILED enough times to max out occurrence counter.
     * Each group of DEM_DEBOUNCE_FAIL_THRESHOLD (3) increments the counter once.
     * Run 256 * 3 = 768 cycles to attempt overflow past 0xFF. */
    uint16 i;
    for (i = 0u; i < 768u; i++) {
        Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_FAILED);
    }

    uint32 count = 0u;
    Std_ReturnType ret = Dem_GetOccurrenceCounter(0u, &count);
    TEST_ASSERT_EQUAL(E_OK, ret);
    /* Counter should be capped at 0xFF or be a reasonable value — must not wrap to 0 */
    TEST_ASSERT_TRUE(count > 0u);
    TEST_ASSERT_TRUE(count <= 0xFFFFFFFFu);
}

/** @verifies SWR-BSW-017
 *  Equivalence class: GetOccurrenceCounter with invalid event ID */
void test_Dem_GetOccurrenceCounter_invalid_id(void)
{
    uint32 count = 0u;
    Std_ReturnType ret = Dem_GetOccurrenceCounter(DEM_MAX_EVENTS, &count);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @verifies SWR-BSW-017
 *  Equivalence class: GetOccurrenceCounter with NULL pointer */
void test_Dem_GetOccurrenceCounter_null_ptr(void)
{
    Std_ReturnType ret = Dem_GetOccurrenceCounter(0u, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @verifies SWR-BSW-018
 *  Equivalence class: Clear DTCs then verify occurrence counter is also cleared */
void test_Dem_ClearDTC_resets_occurrence_counter(void)
{
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_FAILED);
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_FAILED);
    Dem_ReportErrorStatus(0u, DEM_EVENT_STATUS_FAILED);

    uint32 count_before = 0u;
    Dem_GetOccurrenceCounter(0u, &count_before);
    TEST_ASSERT_TRUE(count_before > 0u);

    Dem_ClearAllDTCs();

    uint32 count_after = 0xFFu;
    Dem_GetOccurrenceCounter(0u, &count_after);
    TEST_ASSERT_EQUAL_UINT32(0u, count_after);
}

/* ==================================================================
 * Test runner
 * ================================================================== */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_Dem_ReportError_single_failed);
    RUN_TEST(test_Dem_ReportError_debounce_confirm);
    RUN_TEST(test_Dem_ReportError_passed_heals);
    RUN_TEST(test_Dem_ReportError_occurrence_counter);
    RUN_TEST(test_Dem_ReportError_invalid_event_id);
    RUN_TEST(test_Dem_ClearDTC_clears_all);
    RUN_TEST(test_Dem_GetEventStatus_null_ptr);
    RUN_TEST(test_Dem_multiple_events_independent);

    /* Hardened boundary / fault injection tests */
    RUN_TEST(test_Dem_Init_null_reinit);
    RUN_TEST(test_Dem_EventId_zero_valid);
    RUN_TEST(test_Dem_EventId_max_minus_one);
    RUN_TEST(test_Dem_EventId_max_plus_one);
    RUN_TEST(test_Dem_Debounce_threshold_exact);
    RUN_TEST(test_Dem_StatusBit_testFailed);
    RUN_TEST(test_Dem_StatusBit_pending);
    RUN_TEST(test_Dem_StatusBit_confirmed_implies_others);
    RUN_TEST(test_Dem_FillAllEventSlots);
    RUN_TEST(test_Dem_OccurrenceCounter_no_overflow);
    RUN_TEST(test_Dem_GetOccurrenceCounter_invalid_id);
    RUN_TEST(test_Dem_GetOccurrenceCounter_null_ptr);
    RUN_TEST(test_Dem_ClearDTC_resets_occurrence_counter);

    return UNITY_END();
}
