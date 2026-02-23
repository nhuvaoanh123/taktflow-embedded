/**
 * @file    test_Swc_Indicators.c
 * @brief   Unit tests for Indicators SWC — turn signals and hazard lights
 * @date    2026-02-23
 *
 * @verifies SWR-BCM-006, SWR-BCM-007, SWR-BCM-008
 *
 * Tests indicator initialization (all off), left/right turn signal activation,
 * flash toggle at ~1.5Hz (33 ticks on, 33 ticks off at 10ms), hazard override
 * of turn signals, hazard from E-stop, off command, and guard against
 * uninitialized operation.
 *
 * Mocks: Rte_Read, Rte_Write, Dem_ReportErrorStatus
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
#define SWC_INDICATORS_H
#define BCM_CFG_H

/* ====================================================================
 * Signal IDs (must match Bcm_Cfg.h)
 * ==================================================================== */

#define BCM_SIG_VEHICLE_SPEED       16u
#define BCM_SIG_VEHICLE_STATE       17u
#define BCM_SIG_BODY_CONTROL_CMD    18u
#define BCM_SIG_INDICATOR_LEFT      21u
#define BCM_SIG_INDICATOR_RIGHT     22u
#define BCM_SIG_HAZARD_ACTIVE       23u
#define BCM_SIG_ESTOP_ACTIVE        25u

/* Indicator flash period (ticks at 10ms) */
#define BCM_INDICATOR_FLASH_ON      33u
#define BCM_INDICATOR_FLASH_OFF     33u

/* ====================================================================
 * Mock: Rte_Read — store values in array, return when SWC reads
 * ==================================================================== */

#define MOCK_RTE_MAX_SIGNALS  32u

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
 * Mock: Rte_Write — capture what SWC writes
 * ==================================================================== */

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
 * Mock: Dem_ReportErrorStatus
 * ==================================================================== */

static uint8 mock_dem_event_id;
static uint8 mock_dem_event_status;
static uint8 mock_dem_report_count;

void Dem_ReportErrorStatus(uint8 EventId, uint8 EventStatus)
{
    mock_dem_event_id = EventId;
    mock_dem_event_status = EventStatus;
    mock_dem_report_count++;
}

/* ====================================================================
 * Include SWC under test (source inclusion for test build)
 * ==================================================================== */

#include "../src/Swc_Indicators.c"

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

    mock_dem_event_id     = 0xFFu;
    mock_dem_event_status = 0xFFu;
    mock_dem_report_count = 0u;

    Swc_Indicators_Init();
}

void tearDown(void) { }

/* ====================================================================
 * Helper: run N cycles with current mock state
 * ==================================================================== */

static void run_cycles(uint16 count)
{
    uint16 i;
    for (i = 0u; i < count; i++) {
        Swc_Indicators_10ms();
    }
}

/* ====================================================================
 * SWR-BCM-006: Initialization — all indicators off
 * ==================================================================== */

/** @verifies SWR-BCM-006 */
void test_Indicators_init_all_off(void)
{
    /* Run one cycle with no commands */
    mock_rte_signals[BCM_SIG_BODY_CONTROL_CMD] = 0u;
    mock_rte_signals[BCM_SIG_ESTOP_ACTIVE]     = 0u;

    Swc_Indicators_10ms();

    TEST_ASSERT_EQUAL_UINT32(0u, mock_rte_signals[BCM_SIG_INDICATOR_LEFT]);
    TEST_ASSERT_EQUAL_UINT32(0u, mock_rte_signals[BCM_SIG_INDICATOR_RIGHT]);
    TEST_ASSERT_EQUAL_UINT32(0u, mock_rte_signals[BCM_SIG_HAZARD_ACTIVE]);
}

/* ====================================================================
 * SWR-BCM-006: Left turn signal activation
 * ==================================================================== */

/** @verifies SWR-BCM-006 */
void test_Indicators_left_turn_signal(void)
{
    /* Body control cmd bits 1-2: 01 = left */
    mock_rte_signals[BCM_SIG_BODY_CONTROL_CMD] = (1u << 1u);  /* bit 1 set = left */
    mock_rte_signals[BCM_SIG_ESTOP_ACTIVE]     = 0u;

    /* First tick — flash should be ON (start of flash cycle) */
    Swc_Indicators_10ms();

    TEST_ASSERT_EQUAL_UINT32(1u, mock_rte_signals[BCM_SIG_INDICATOR_LEFT]);
    TEST_ASSERT_EQUAL_UINT32(0u, mock_rte_signals[BCM_SIG_INDICATOR_RIGHT]);
}

/* ====================================================================
 * SWR-BCM-006: Right turn signal activation
 * ==================================================================== */

/** @verifies SWR-BCM-006 */
void test_Indicators_right_turn_signal(void)
{
    /* Body control cmd bits 1-2: 10 = right */
    mock_rte_signals[BCM_SIG_BODY_CONTROL_CMD] = (2u << 1u);  /* bit 2 set = right */
    mock_rte_signals[BCM_SIG_ESTOP_ACTIVE]     = 0u;

    /* First tick — flash should be ON */
    Swc_Indicators_10ms();

    TEST_ASSERT_EQUAL_UINT32(0u, mock_rte_signals[BCM_SIG_INDICATOR_LEFT]);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_rte_signals[BCM_SIG_INDICATOR_RIGHT]);
}

/* ====================================================================
 * SWR-BCM-007: Flash toggles at ~1.5Hz (33 ticks ON, 33 ticks OFF)
 * ==================================================================== */

/** @verifies SWR-BCM-007 */
void test_Indicators_flash_toggles_at_1_5hz(void)
{
    /* Left turn signal active */
    mock_rte_signals[BCM_SIG_BODY_CONTROL_CMD] = (1u << 1u);
    mock_rte_signals[BCM_SIG_ESTOP_ACTIVE]     = 0u;

    /* Run 1 tick — should be ON */
    Swc_Indicators_10ms();
    TEST_ASSERT_EQUAL_UINT32(1u, mock_rte_signals[BCM_SIG_INDICATOR_LEFT]);

    /* Run to tick 32 (32 more ticks, total 33) — still ON at tick 33 */
    run_cycles(32u);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_rte_signals[BCM_SIG_INDICATOR_LEFT]);

    /* Tick 34 — should toggle to OFF */
    Swc_Indicators_10ms();
    TEST_ASSERT_EQUAL_UINT32(0u, mock_rte_signals[BCM_SIG_INDICATOR_LEFT]);

    /* Run 32 more ticks (total in OFF phase = 33) — still OFF */
    run_cycles(32u);
    TEST_ASSERT_EQUAL_UINT32(0u, mock_rte_signals[BCM_SIG_INDICATOR_LEFT]);

    /* Next tick — should toggle back to ON */
    Swc_Indicators_10ms();
    TEST_ASSERT_EQUAL_UINT32(1u, mock_rte_signals[BCM_SIG_INDICATOR_LEFT]);
}

/* ====================================================================
 * SWR-BCM-008: Hazard overrides turn signal (both flash)
 * ==================================================================== */

/** @verifies SWR-BCM-008 */
void test_Indicators_hazard_overrides_turn(void)
{
    /* Left turn active + hazard bit set (bit 3) */
    mock_rte_signals[BCM_SIG_BODY_CONTROL_CMD] = (1u << 1u) | (1u << 3u);
    mock_rte_signals[BCM_SIG_ESTOP_ACTIVE]     = 0u;

    Swc_Indicators_10ms();

    /* Both left and right should flash when hazard is active */
    TEST_ASSERT_EQUAL_UINT32(1u, mock_rte_signals[BCM_SIG_INDICATOR_LEFT]);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_rte_signals[BCM_SIG_INDICATOR_RIGHT]);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_rte_signals[BCM_SIG_HAZARD_ACTIVE]);
}

/* ====================================================================
 * SWR-BCM-008: Hazard from E-stop signal
 * ==================================================================== */

/** @verifies SWR-BCM-008 */
void test_Indicators_hazard_from_estop(void)
{
    /* No body control cmd, but E-stop active */
    mock_rte_signals[BCM_SIG_BODY_CONTROL_CMD] = 0u;
    mock_rte_signals[BCM_SIG_ESTOP_ACTIVE]     = 1u;

    Swc_Indicators_10ms();

    /* Both left and right should flash, hazard active */
    TEST_ASSERT_EQUAL_UINT32(1u, mock_rte_signals[BCM_SIG_INDICATOR_LEFT]);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_rte_signals[BCM_SIG_INDICATOR_RIGHT]);
    TEST_ASSERT_EQUAL_UINT32(1u, mock_rte_signals[BCM_SIG_HAZARD_ACTIVE]);
}

/* ====================================================================
 * SWR-BCM-006: Off command — indicators off
 * ==================================================================== */

/** @verifies SWR-BCM-006 */
void test_Indicators_off_command(void)
{
    /* First activate left turn */
    mock_rte_signals[BCM_SIG_BODY_CONTROL_CMD] = (1u << 1u);
    mock_rte_signals[BCM_SIG_ESTOP_ACTIVE]     = 0u;
    Swc_Indicators_10ms();
    TEST_ASSERT_EQUAL_UINT32(1u, mock_rte_signals[BCM_SIG_INDICATOR_LEFT]);

    /* Now send off command (bits 1-2 = 0, no hazard) */
    mock_rte_signals[BCM_SIG_BODY_CONTROL_CMD] = 0u;
    Swc_Indicators_10ms();

    TEST_ASSERT_EQUAL_UINT32(0u, mock_rte_signals[BCM_SIG_INDICATOR_LEFT]);
    TEST_ASSERT_EQUAL_UINT32(0u, mock_rte_signals[BCM_SIG_INDICATOR_RIGHT]);
    TEST_ASSERT_EQUAL_UINT32(0u, mock_rte_signals[BCM_SIG_HAZARD_ACTIVE]);
}

/* ====================================================================
 * SWR-BCM-006: Not initialized — does nothing
 * ==================================================================== */

/** @verifies SWR-BCM-006 */
void test_Indicators_not_init_does_nothing(void)
{
    initialized = FALSE;

    mock_rte_signals[BCM_SIG_BODY_CONTROL_CMD] = (1u << 1u);
    mock_rte_write_count = 0u;

    Swc_Indicators_10ms();

    /* No RTE writes should occur */
    TEST_ASSERT_EQUAL_UINT8(0u, mock_rte_write_count);
}

/* ====================================================================
 * Test runner
 * ==================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* SWR-BCM-006: Initialization and basic activation */
    RUN_TEST(test_Indicators_init_all_off);
    RUN_TEST(test_Indicators_left_turn_signal);
    RUN_TEST(test_Indicators_right_turn_signal);
    RUN_TEST(test_Indicators_off_command);
    RUN_TEST(test_Indicators_not_init_does_nothing);

    /* SWR-BCM-007: Flash timing */
    RUN_TEST(test_Indicators_flash_toggles_at_1_5hz);

    /* SWR-BCM-008: Hazard logic */
    RUN_TEST(test_Indicators_hazard_overrides_turn);
    RUN_TEST(test_Indicators_hazard_from_estop);

    return UNITY_END();
}
