/**
 * @file    test_Rte.c
 * @brief   Unit tests for Runtime Environment module
 * @date    2026-02-21
 *
 * @verifies SWR-BSW-026, SWR-BSW-027
 *
 * Tests signal read/write (port-based communication), runnable
 * scheduling, WdgM checkpoint integration, and defensive checks.
 */
#include "unity.h"
#include "Rte.h"

/* ==================================================================
 * Mock: WdgM (checkpoint reached after runnable cycle)
 * ================================================================== */

static uint8  mock_wdgm_se_id;
static uint8  mock_wdgm_call_count;

Std_ReturnType WdgM_CheckpointReached(uint8 SEId)
{
    mock_wdgm_se_id = SEId;
    mock_wdgm_call_count++;
    return E_OK;
}

/* ==================================================================
 * Test runnables (set flags when called)
 * ================================================================== */

static uint8  runnable_10ms_call_count;
static uint8  runnable_100ms_call_count;
static uint8  runnable_10ms_b_call_count;

static void TestRunnable_10ms(void)
{
    runnable_10ms_call_count++;
}

static void TestRunnable_100ms(void)
{
    runnable_100ms_call_count++;
}

static void TestRunnable_10ms_B(void)
{
    runnable_10ms_b_call_count++;
}

/* ==================================================================
 * Test Configuration
 * ================================================================== */

/* Signal config: 3 test signals */
static const Rte_SignalConfigType test_signals[] = {
    { RTE_SIG_TORQUE_REQUEST,  0u },   /* signal 0, initial value 0 */
    { RTE_SIG_STEERING_ANGLE,  0u },   /* signal 1, initial value 0 */
    { RTE_SIG_VEHICLE_SPEED,   0u },   /* signal 2, initial value 0 */
};

/* Runnable config: 3 runnables */
static const Rte_RunnableConfigType test_runnables[] = {
    { TestRunnable_10ms,   10u, 2u, 0u },  /* 10ms, priority 2, SE 0 */
    { TestRunnable_10ms_B, 10u, 1u, 0u },  /* 10ms, priority 1, SE 0 */
    { TestRunnable_100ms, 100u, 1u, 1u },  /* 100ms, priority 1, SE 1 */
};

static Rte_ConfigType test_config;

void setUp(void)
{
    mock_wdgm_call_count     = 0u;
    mock_wdgm_se_id          = 0xFFu;
    runnable_10ms_call_count  = 0u;
    runnable_100ms_call_count = 0u;
    runnable_10ms_b_call_count = 0u;

    test_config.signalConfig   = test_signals;
    test_config.signalCount    = 3u;
    test_config.runnableConfig = test_runnables;
    test_config.runnableCount  = 3u;

    Rte_Init(&test_config);
}

void tearDown(void) { }

/* ==================================================================
 * SWR-BSW-026: Rte Port-Based Communication
 * ================================================================== */

/** @verifies SWR-BSW-026 */
void test_Rte_Init_valid_config(void)
{
    /* Init already called in setUp — verify we can write/read */
    uint32 val = 42u;
    Std_ReturnType ret = Rte_Write(RTE_SIG_TORQUE_REQUEST, val);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @verifies SWR-BSW-026 */
void test_Rte_Init_null_config(void)
{
    Rte_Init(NULL_PTR);

    /* After null init, all operations should fail */
    uint32 val = 0u;
    Std_ReturnType ret = Rte_Write(RTE_SIG_TORQUE_REQUEST, 10u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);

    ret = Rte_Read(RTE_SIG_TORQUE_REQUEST, &val);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @verifies SWR-BSW-026 */
void test_Rte_Write_stores_signal_value(void)
{
    Std_ReturnType ret = Rte_Write(RTE_SIG_TORQUE_REQUEST, 255u);
    TEST_ASSERT_EQUAL(E_OK, ret);

    uint32 readback = 0u;
    ret = Rte_Read(RTE_SIG_TORQUE_REQUEST, &readback);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(255u, readback);
}

/** @verifies SWR-BSW-026 */
void test_Rte_Read_returns_initial_value_before_write(void)
{
    uint32 val = 0xDEADu;
    Std_ReturnType ret = Rte_Read(RTE_SIG_STEERING_ANGLE, &val);

    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0u, val);  /* initial value from config */
}

/** @verifies SWR-BSW-026 */
void test_Rte_Write_invalid_signal_id(void)
{
    Std_ReturnType ret = Rte_Write(RTE_MAX_SIGNALS + 1u, 100u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @verifies SWR-BSW-026 */
void test_Rte_Read_invalid_signal_id(void)
{
    uint32 val = 0u;
    Std_ReturnType ret = Rte_Read(RTE_MAX_SIGNALS + 1u, &val);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @verifies SWR-BSW-026 */
void test_Rte_Read_null_data_pointer(void)
{
    Std_ReturnType ret = Rte_Read(RTE_SIG_TORQUE_REQUEST, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @verifies SWR-BSW-026 */
void test_Rte_Write_Read_copy_semantics(void)
{
    /* Write a value, then overwrite — read should get latest */
    Rte_Write(RTE_SIG_VEHICLE_SPEED, 50u);
    Rte_Write(RTE_SIG_VEHICLE_SPEED, 120u);

    uint32 val = 0u;
    Rte_Read(RTE_SIG_VEHICLE_SPEED, &val);
    TEST_ASSERT_EQUAL_UINT32(120u, val);

    /* Write to one signal should not affect another */
    uint32 other = 0u;
    Rte_Read(RTE_SIG_TORQUE_REQUEST, &other);
    TEST_ASSERT_EQUAL_UINT32(0u, other);
}

/* ==================================================================
 * SWR-BSW-027: Rte Runnable Scheduling
 * ================================================================== */

/** @verifies SWR-BSW-027 */
void test_Rte_MainFunction_fires_10ms_runnable_every_10_ticks(void)
{
    /* Tick 10 times — 10ms runnables should fire once at tick 10 */
    for (uint32 i = 0u; i < 10u; i++) {
        Rte_MainFunction();
    }

    TEST_ASSERT_EQUAL_UINT8(1u, runnable_10ms_call_count);
}

/** @verifies SWR-BSW-027 */
void test_Rte_MainFunction_fires_100ms_runnable_every_100_ticks(void)
{
    /* Tick 100 times — 100ms runnable should fire once */
    for (uint32 i = 0u; i < 100u; i++) {
        Rte_MainFunction();
    }

    TEST_ASSERT_EQUAL_UINT8(1u, runnable_100ms_call_count);
    /* 10ms runnables should have fired 10 times */
    TEST_ASSERT_EQUAL_UINT8(10u, runnable_10ms_call_count);
}

/** @verifies SWR-BSW-027 */
void test_Rte_MainFunction_no_fire_before_period(void)
{
    /* Tick 9 times — 10ms runnable should NOT have fired yet */
    for (uint32 i = 0u; i < 9u; i++) {
        Rte_MainFunction();
    }

    TEST_ASSERT_EQUAL_UINT8(0u, runnable_10ms_call_count);
    TEST_ASSERT_EQUAL_UINT8(0u, runnable_100ms_call_count);
}

/** @verifies SWR-BSW-027 */
void test_Rte_MainFunction_multiple_runnables_same_period(void)
{
    /* Tick 10 times — both 10ms runnables should fire */
    for (uint32 i = 0u; i < 10u; i++) {
        Rte_MainFunction();
    }

    TEST_ASSERT_EQUAL_UINT8(1u, runnable_10ms_call_count);
    TEST_ASSERT_EQUAL_UINT8(1u, runnable_10ms_b_call_count);
}

/** @verifies SWR-BSW-027 */
void test_Rte_MainFunction_calls_WdgM_checkpoint(void)
{
    /* After 10 ticks, runnables with SE 0 fire — WdgM checkpoint for SE 0 */
    for (uint32 i = 0u; i < 10u; i++) {
        Rte_MainFunction();
    }

    TEST_ASSERT_TRUE(mock_wdgm_call_count > 0u);
}

/** @verifies SWR-BSW-027 */
void test_Rte_MainFunction_does_nothing_before_init(void)
{
    /* Re-init with null to simulate uninitialized state */
    Rte_Init(NULL_PTR);

    Rte_MainFunction();

    TEST_ASSERT_EQUAL_UINT8(0u, runnable_10ms_call_count);
    TEST_ASSERT_EQUAL_UINT8(0u, runnable_100ms_call_count);
    TEST_ASSERT_EQUAL_UINT8(0u, mock_wdgm_call_count);
}

/* ==================================================================
 * Test runner
 * ================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* SWR-BSW-026: Port-Based Communication */
    RUN_TEST(test_Rte_Init_valid_config);
    RUN_TEST(test_Rte_Init_null_config);
    RUN_TEST(test_Rte_Write_stores_signal_value);
    RUN_TEST(test_Rte_Read_returns_initial_value_before_write);
    RUN_TEST(test_Rte_Write_invalid_signal_id);
    RUN_TEST(test_Rte_Read_invalid_signal_id);
    RUN_TEST(test_Rte_Read_null_data_pointer);
    RUN_TEST(test_Rte_Write_Read_copy_semantics);

    /* SWR-BSW-027: Runnable Scheduling */
    RUN_TEST(test_Rte_MainFunction_fires_10ms_runnable_every_10_ticks);
    RUN_TEST(test_Rte_MainFunction_fires_100ms_runnable_every_100_ticks);
    RUN_TEST(test_Rte_MainFunction_no_fire_before_period);
    RUN_TEST(test_Rte_MainFunction_multiple_runnables_same_period);
    RUN_TEST(test_Rte_MainFunction_calls_WdgM_checkpoint);
    RUN_TEST(test_Rte_MainFunction_does_nothing_before_init);

    return UNITY_END();
}
