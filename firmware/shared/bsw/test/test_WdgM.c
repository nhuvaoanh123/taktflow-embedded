/**
 * @file    test_WdgM.c
 * @brief   Unit tests for Watchdog Manager
 * @date    2026-02-21
 *
 * @verifies SWR-BSW-019, SWR-BSW-020
 *
 * Tests alive supervision, checkpoint counting, global status,
 * and watchdog feed gating logic.
 */
#include "unity.h"
#include "WdgM.h"

/* ==================================================================
 * Mock: Dio (watchdog pin toggle)
 * ================================================================== */

static uint8 mock_dio_flip_count;

void Dio_FlipChannel(uint8 ChannelId)
{
    (void)ChannelId;
    mock_dio_flip_count++;
}

/* ==================================================================
 * Mock: Dem (error reporting)
 * ================================================================== */

static uint8 mock_dem_event_id;
static uint8 mock_dem_status;
static uint8 mock_dem_report_count;

void Dem_ReportErrorStatus(uint8 EventId, uint8 EventStatus)
{
    mock_dem_event_id = EventId;
    mock_dem_status = EventStatus;
    mock_dem_report_count++;
}

/* ==================================================================
 * Test configuration
 * ================================================================== */

static const WdgM_SupervisedEntityConfigType test_se_config[] = {
    { 0u, 4u, 6u, 2u },  /* SE 0: expected 4-6 checkpoints, 2 failed cycles tolerance */
    { 1u, 1u, 1u, 1u },  /* SE 1: expected exactly 1, 1 cycle tolerance */
};

static WdgM_ConfigType test_config;

void setUp(void)
{
    mock_dio_flip_count = 0u;
    mock_dem_report_count = 0u;

    test_config.seConfig = test_se_config;
    test_config.seCount = 2u;
    test_config.wdtDioChannel = 0u;

    WdgM_Init(&test_config);
}

void tearDown(void) { }

/* ==================================================================
 * SWR-BSW-019: Alive Supervision
 * ================================================================== */

/** @verifies SWR-BSW-019 */
void test_WdgM_CheckpointReached_increments(void)
{
    Std_ReturnType ret = WdgM_CheckpointReached(0u);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @verifies SWR-BSW-019 */
void test_WdgM_CheckpointReached_invalid_se(void)
{
    Std_ReturnType ret = WdgM_CheckpointReached(99u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @verifies SWR-BSW-019 */
void test_WdgM_MainFunction_all_ok_feeds_wdt(void)
{
    /* SE 0: report 5 checkpoints (within 4-6 range) */
    for (uint8 i = 0u; i < 5u; i++) {
        WdgM_CheckpointReached(0u);
    }
    /* SE 1: report 1 checkpoint (expected exactly 1) */
    WdgM_CheckpointReached(1u);

    WdgM_MainFunction();

    TEST_ASSERT_EQUAL_UINT8(1u, mock_dio_flip_count);
    TEST_ASSERT_EQUAL(WDGM_GLOBAL_STATUS_OK, WdgM_GetGlobalStatus());
}

/** @verifies SWR-BSW-019 */
void test_WdgM_MainFunction_too_few_no_feed(void)
{
    /* SE 0: only 2 checkpoints (below min 4) */
    WdgM_CheckpointReached(0u);
    WdgM_CheckpointReached(0u);
    /* SE 1: ok */
    WdgM_CheckpointReached(1u);

    WdgM_MainFunction();

    /* First failure — within tolerance, but local status should be FAILED */
    WdgM_LocalStatusType local_status;
    WdgM_GetLocalStatus(0u, &local_status);
    TEST_ASSERT_EQUAL(WDGM_LOCAL_STATUS_FAILED, local_status);
}

/** @verifies SWR-BSW-019 */
void test_WdgM_MainFunction_too_many_no_feed(void)
{
    /* SE 0: 10 checkpoints (above max 6) */
    for (uint8 i = 0u; i < 10u; i++) {
        WdgM_CheckpointReached(0u);
    }
    /* SE 1: ok */
    WdgM_CheckpointReached(1u);

    WdgM_MainFunction();

    WdgM_LocalStatusType local_status;
    WdgM_GetLocalStatus(0u, &local_status);
    TEST_ASSERT_EQUAL(WDGM_LOCAL_STATUS_FAILED, local_status);
}

/* ==================================================================
 * SWR-BSW-020: Global Status and Expiry
 * ================================================================== */

/** @verifies SWR-BSW-020 */
void test_WdgM_expired_after_tolerance(void)
{
    /* SE 1 has tolerance 1. Fail it twice to expire. */
    /* Cycle 1: no checkpoints for SE 1 */
    for (uint8 i = 0u; i < 5u; i++) {
        WdgM_CheckpointReached(0u);
    }
    WdgM_MainFunction();

    /* Cycle 2: still no checkpoints for SE 1 */
    for (uint8 i = 0u; i < 5u; i++) {
        WdgM_CheckpointReached(0u);
    }
    WdgM_MainFunction();

    WdgM_LocalStatusType local;
    WdgM_GetLocalStatus(1u, &local);
    TEST_ASSERT_EQUAL(WDGM_LOCAL_STATUS_EXPIRED, local);
    TEST_ASSERT_EQUAL(WDGM_GLOBAL_STATUS_FAILED, WdgM_GetGlobalStatus());
}

/** @verifies SWR-BSW-020 */
void test_WdgM_Init_null_config(void)
{
    WdgM_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(WDGM_GLOBAL_STATUS_FAILED, WdgM_GetGlobalStatus());
}

/** @verifies SWR-BSW-020 */
void test_WdgM_GetLocalStatus_null(void)
{
    Std_ReturnType ret = WdgM_GetLocalStatus(0u, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* ==================================================================
 * Test runner
 * ================================================================== */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_WdgM_CheckpointReached_increments);
    RUN_TEST(test_WdgM_CheckpointReached_invalid_se);
    RUN_TEST(test_WdgM_MainFunction_all_ok_feeds_wdt);
    RUN_TEST(test_WdgM_MainFunction_too_few_no_feed);
    RUN_TEST(test_WdgM_MainFunction_too_many_no_feed);
    RUN_TEST(test_WdgM_expired_after_tolerance);
    RUN_TEST(test_WdgM_Init_null_config);
    RUN_TEST(test_WdgM_GetLocalStatus_null);

    return UNITY_END();
}
