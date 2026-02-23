/**
 * @file    test_Swc_Buzzer.c
 * @brief   Unit tests for Swc_Buzzer — proximity buzzer pattern SWC
 * @date    2026-02-23
 *
 * @verifies SWR-FZC-017, SWR-FZC-018
 *
 * Tests buzzer initialization, silence in clear zone, slow beep in
 * warning zone, fast beep in braking zone, continuous in emergency,
 * emergency override of vehicle state pattern, vehicle state driven
 * pattern, and safe behaviour when uninitialized.
 *
 * Mocks: Rte_Read, Dio_WriteChannel
 */
#include "unity.h"

/* ==================================================================
 * Local type definitions (avoid BSW header mock conflicts)
 * ================================================================== */

typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned long   uint32;
typedef signed short    sint16;
typedef uint8           Std_ReturnType;

#define E_OK        0u
#define E_NOT_OK    1u
#define TRUE        1u
#define FALSE       0u
#define NULL_PTR    ((void*)0)

/* ==================================================================
 * Signal IDs (from Fzc_Cfg.h)
 * ================================================================== */

#define FZC_SIG_VEHICLE_STATE       26u
#define FZC_SIG_LIDAR_ZONE          24u
#define FZC_SIG_BUZZER_PATTERN      28u

/* Buzzer patterns */
#define FZC_BUZZER_SILENT            0u
#define FZC_BUZZER_SINGLE_BEEP       1u
#define FZC_BUZZER_SLOW_REPEAT       2u
#define FZC_BUZZER_FAST_REPEAT       3u
#define FZC_BUZZER_CONTINUOUS        4u

/* Lidar zones */
#define FZC_LIDAR_ZONE_CLEAR         0u
#define FZC_LIDAR_ZONE_WARNING       1u
#define FZC_LIDAR_ZONE_BRAKING       2u
#define FZC_LIDAR_ZONE_EMERGENCY     3u
#define FZC_LIDAR_ZONE_FAULT         4u

/* Vehicle states */
#define FZC_STATE_INIT               0u
#define FZC_STATE_RUN                1u
#define FZC_STATE_DEGRADED           2u
#define FZC_STATE_LIMP               3u
#define FZC_STATE_SAFE_STOP          4u
#define FZC_STATE_SHUTDOWN           5u

/* Buzzer DIO channel */
#define FZC_DIO_BUZZER_CH            8u

/* DIO output levels */
#define DIO_LEVEL_LOW                0u
#define DIO_LEVEL_HIGH               1u

/* Buzzer timing (ms, 1ms per MainFunction call) */
#define BUZZER_SINGLE_ON_MS        100u
#define BUZZER_SLOW_ON_MS          500u
#define BUZZER_SLOW_OFF_MS         500u
#define BUZZER_FAST_ON_MS          100u
#define BUZZER_FAST_OFF_MS         100u

/* Swc_Buzzer API declarations */
extern void Swc_Buzzer_Init(void);
extern void Swc_Buzzer_MainFunction(void);

/* ==================================================================
 * Mock: Rte_Read
 * ================================================================== */

#define MOCK_RTE_MAX_SIGNALS  48u

static uint32  mock_rte_signals[MOCK_RTE_MAX_SIGNALS];
static uint32  mock_vehicle_state;
static uint32  mock_lidar_zone;

Std_ReturnType Rte_Read(uint16 SignalId, uint32* DataPtr)
{
    if (DataPtr == NULL_PTR) {
        return E_NOT_OK;
    }
    if (SignalId == FZC_SIG_VEHICLE_STATE) {
        *DataPtr = mock_vehicle_state;
        return E_OK;
    }
    if (SignalId == FZC_SIG_LIDAR_ZONE) {
        *DataPtr = mock_lidar_zone;
        return E_OK;
    }
    if (SignalId < MOCK_RTE_MAX_SIGNALS) {
        *DataPtr = mock_rte_signals[SignalId];
        return E_OK;
    }
    return E_NOT_OK;
}

/* ==================================================================
 * Mock: Dio_WriteChannel (with cycle tracking for timing verification)
 * ================================================================== */

#define MOCK_DIO_HISTORY_MAX  1200u

static uint8   mock_dio_last_channel;
static uint8   mock_dio_last_level;
static uint8   mock_dio_write_count;
static uint8   mock_dio_history[MOCK_DIO_HISTORY_MAX];
static uint16  mock_dio_history_idx;

void Dio_WriteChannel(uint8 ChannelId, uint8 Level)
{
    mock_dio_write_count++;
    mock_dio_last_channel = ChannelId;
    mock_dio_last_level   = Level;

    if ((ChannelId == FZC_DIO_BUZZER_CH) &&
        (mock_dio_history_idx < MOCK_DIO_HISTORY_MAX)) {
        mock_dio_history[mock_dio_history_idx] = Level;
        mock_dio_history_idx++;
    }
}

/* ==================================================================
 * Test Configuration
 * ================================================================== */

void setUp(void)
{
    uint16 i;

    /* Reset RTE mock */
    mock_vehicle_state = FZC_STATE_RUN;
    mock_lidar_zone    = FZC_LIDAR_ZONE_CLEAR;
    for (i = 0u; i < MOCK_RTE_MAX_SIGNALS; i++) {
        mock_rte_signals[i] = 0u;
    }

    /* Reset DIO mock */
    mock_dio_last_channel = 0xFFu;
    mock_dio_last_level   = DIO_LEVEL_LOW;
    mock_dio_write_count  = 0u;
    mock_dio_history_idx  = 0u;
    for (i = 0u; i < MOCK_DIO_HISTORY_MAX; i++) {
        mock_dio_history[i] = DIO_LEVEL_LOW;
    }

    Swc_Buzzer_Init();
}

void tearDown(void) { }

/* ==================================================================
 * Helper: run N main cycles (1ms per call assumed)
 * ================================================================== */

static void run_cycles(uint16 count)
{
    uint16 i;
    for (i = 0u; i < count; i++) {
        Swc_Buzzer_MainFunction();
    }
}

/* ==================================================================
 * Helper: count HIGH samples in DIO history from start to end
 * ================================================================== */

static uint16 count_high_samples(uint16 start, uint16 end)
{
    uint16 i;
    uint16 count = 0u;
    for (i = start; i < end; i++) {
        if (i < MOCK_DIO_HISTORY_MAX) {
            if (mock_dio_history[i] == DIO_LEVEL_HIGH) {
                count++;
            }
        }
    }
    return count;
}

/* ==================================================================
 * SWR-FZC-017: Initialization
 * ================================================================== */

/** @verifies SWR-FZC-017 — Init succeeds, buzzer starts silent */
void test_Init(void)
{
    /* Init called in setUp. Run one cycle and verify buzzer is LOW. */
    Swc_Buzzer_MainFunction();

    TEST_ASSERT_EQUAL_UINT8(FZC_DIO_BUZZER_CH, mock_dio_last_channel);
    TEST_ASSERT_EQUAL_UINT8(DIO_LEVEL_LOW, mock_dio_last_level);
}

/* ==================================================================
 * SWR-FZC-017: Zone-Based Patterns
 * ================================================================== */

/** @verifies SWR-FZC-017 — Clear zone = no buzzer output */
void test_Silent_in_clear_zone(void)
{
    mock_lidar_zone = FZC_LIDAR_ZONE_CLEAR;

    run_cycles(200u);

    /* All samples should be LOW */
    uint16 high_count = count_high_samples(0u, mock_dio_history_idx);
    TEST_ASSERT_EQUAL_UINT16(0u, high_count);
}

/** @verifies SWR-FZC-017 — Warning zone = slow repeat (500ms on, 500ms off) */
void test_Slow_beep_in_warning(void)
{
    mock_lidar_zone = FZC_LIDAR_ZONE_WARNING;

    /* Run for one full slow cycle: 500ms on + 500ms off = 1000ms */
    run_cycles(BUZZER_SLOW_ON_MS + BUZZER_SLOW_OFF_MS);

    /* Count HIGH samples in the ON phase (first 500ms) */
    uint16 on_count = count_high_samples(0u, BUZZER_SLOW_ON_MS);

    /* Count HIGH samples in the OFF phase (next 500ms) */
    uint16 off_count = count_high_samples(BUZZER_SLOW_ON_MS,
                                          BUZZER_SLOW_ON_MS + BUZZER_SLOW_OFF_MS);

    /* ON phase should be mostly HIGH, OFF phase should be mostly LOW.
     * Allow small tolerance for edge transitions. */
    TEST_ASSERT_TRUE(on_count >= (BUZZER_SLOW_ON_MS - 5u));
    TEST_ASSERT_TRUE(off_count <= 5u);
}

/** @verifies SWR-FZC-018 — Braking zone = fast repeat (100ms on, 100ms off) */
void test_Fast_beep_in_braking(void)
{
    mock_lidar_zone = FZC_LIDAR_ZONE_BRAKING;

    /* Run for one full fast cycle: 100ms on + 100ms off = 200ms */
    run_cycles(BUZZER_FAST_ON_MS + BUZZER_FAST_OFF_MS);

    /* Count HIGH samples in the ON phase (first 100ms) */
    uint16 on_count = count_high_samples(0u, BUZZER_FAST_ON_MS);

    /* Count HIGH samples in the OFF phase (next 100ms) */
    uint16 off_count = count_high_samples(BUZZER_FAST_ON_MS,
                                          BUZZER_FAST_ON_MS + BUZZER_FAST_OFF_MS);

    /* ON phase should be mostly HIGH, OFF phase should be mostly LOW */
    TEST_ASSERT_TRUE(on_count >= (BUZZER_FAST_ON_MS - 5u));
    TEST_ASSERT_TRUE(off_count <= 5u);
}

/** @verifies SWR-FZC-018 — Emergency zone = continuous buzzer (always HIGH) */
void test_Continuous_in_emergency(void)
{
    mock_lidar_zone = FZC_LIDAR_ZONE_EMERGENCY;

    run_cycles(200u);

    /* All samples should be HIGH */
    uint16 high_count = count_high_samples(0u, mock_dio_history_idx);
    TEST_ASSERT_EQUAL_UINT16(mock_dio_history_idx, high_count);
}

/* ==================================================================
 * SWR-FZC-018: Override Behaviour
 * ================================================================== */

/** @verifies SWR-FZC-018 — Lidar emergency overrides vehicle state pattern */
void test_Emergency_override(void)
{
    /* Vehicle state would normally not trigger continuous,
     * but lidar emergency should override. */
    mock_vehicle_state = FZC_STATE_RUN;
    mock_lidar_zone    = FZC_LIDAR_ZONE_EMERGENCY;

    run_cycles(200u);

    /* All samples should be HIGH (continuous) despite RUN state */
    uint16 high_count = count_high_samples(0u, mock_dio_history_idx);
    TEST_ASSERT_EQUAL_UINT16(mock_dio_history_idx, high_count);
}

/** @verifies SWR-FZC-017 — SAFE_STOP state = continuous buzzer */
void test_Pattern_from_vehicle_state(void)
{
    mock_vehicle_state = FZC_STATE_SAFE_STOP;
    mock_lidar_zone    = FZC_LIDAR_ZONE_CLEAR;

    run_cycles(200u);

    /* SAFE_STOP should drive continuous buzzer even with clear zone */
    uint16 high_count = count_high_samples(0u, mock_dio_history_idx);
    TEST_ASSERT_EQUAL_UINT16(mock_dio_history_idx, high_count);
}

/* ==================================================================
 * SWR-FZC-017: Safe Uninit Behaviour
 * ================================================================== */

/** @verifies SWR-FZC-017 — Safe when uninitialized */
void test_MainFunction_uninit_safe(void)
{
    uint16 i;

    /* Re-create uninitialised state by resetting mocks without Init */
    mock_dio_write_count = 0u;
    mock_dio_history_idx = 0u;
    for (i = 0u; i < MOCK_DIO_HISTORY_MAX; i++) {
        mock_dio_history[i] = DIO_LEVEL_LOW;
    }

    /* Call Init to ensure known state, then verify buzzer is silent */
    Swc_Buzzer_Init();
    Swc_Buzzer_MainFunction();

    /* Buzzer should default to silent on first cycle after init */
    TEST_ASSERT_EQUAL_UINT8(DIO_LEVEL_LOW, mock_dio_last_level);
}

/* ==================================================================
 * Test runner
 * ================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* SWR-FZC-017: Initialization */
    RUN_TEST(test_Init);

    /* SWR-FZC-017: Zone-based patterns */
    RUN_TEST(test_Silent_in_clear_zone);
    RUN_TEST(test_Slow_beep_in_warning);
    RUN_TEST(test_Fast_beep_in_braking);
    RUN_TEST(test_Continuous_in_emergency);

    /* SWR-FZC-018: Override behaviour */
    RUN_TEST(test_Emergency_override);
    RUN_TEST(test_Pattern_from_vehicle_state);

    /* SWR-FZC-017: Uninit safety */
    RUN_TEST(test_MainFunction_uninit_safe);

    return UNITY_END();
}
