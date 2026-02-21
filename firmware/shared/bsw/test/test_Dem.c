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

    return UNITY_END();
}
