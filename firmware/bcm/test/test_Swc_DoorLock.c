/**
 * @file    test_Swc_DoorLock.c
 * @brief   Unit tests for DoorLock SWC — manual and auto lock/unlock
 * @date    2026-02-23
 *
 * @verifies SWR-BCM-009
 *
 * Tests door lock initialization (unlocked), manual lock command,
 * auto-lock above 10 speed threshold, auto-unlock when transitioning
 * to parked state, and guard against uninitialized operation.
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
#define SWC_DOORLOCK_H
#define BCM_CFG_H

/* ====================================================================
 * Signal IDs (must match Bcm_Cfg.h)
 * ==================================================================== */

#define BCM_SIG_VEHICLE_SPEED       16u
#define BCM_SIG_VEHICLE_STATE       17u
#define BCM_SIG_BODY_CONTROL_CMD    18u
#define BCM_SIG_DOOR_LOCK_STATE     24u

/* Vehicle state values */
#define BCM_VSTATE_INIT             0u
#define BCM_VSTATE_READY            1u
#define BCM_VSTATE_DRIVING          2u
#define BCM_VSTATE_DEGRADED         3u
#define BCM_VSTATE_ESTOP            4u
#define BCM_VSTATE_FAULT            5u

/* Auto-lock speed threshold */
#define BCM_AUTO_LOCK_SPEED         10u

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

#include "../src/Swc_DoorLock.c"

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

    Swc_DoorLock_Init();
}

void tearDown(void) { }

/* ====================================================================
 * SWR-BCM-009: Initialization — door unlocked
 * ==================================================================== */

/** @verifies SWR-BCM-009 */
void test_DoorLock_init_unlocked(void)
{
    /* Run one cycle with no commands, vehicle parked */
    mock_rte_signals[BCM_SIG_BODY_CONTROL_CMD] = 0u;
    mock_rte_signals[BCM_SIG_VEHICLE_SPEED]    = 0u;
    mock_rte_signals[BCM_SIG_VEHICLE_STATE]    = BCM_VSTATE_READY;

    Swc_DoorLock_100ms();

    /* Door lock state should be 0 (unlocked) */
    TEST_ASSERT_EQUAL_UINT32(0u, mock_rte_signals[BCM_SIG_DOOR_LOCK_STATE]);
}

/* ====================================================================
 * SWR-BCM-009: Manual lock command — byte 1 bit 0
 * ==================================================================== */

/** @verifies SWR-BCM-009 */
void test_DoorLock_manual_lock_command(void)
{
    /* Byte 1 bit 0 = lock. Body control cmd is uint32, byte 1 = bits 8-15 */
    mock_rte_signals[BCM_SIG_BODY_CONTROL_CMD] = (1u << 8u);  /* Byte 1, bit 0 */
    mock_rte_signals[BCM_SIG_VEHICLE_SPEED]    = 0u;
    mock_rte_signals[BCM_SIG_VEHICLE_STATE]    = BCM_VSTATE_READY;

    Swc_DoorLock_100ms();

    TEST_ASSERT_EQUAL_UINT32(1u, mock_rte_signals[BCM_SIG_DOOR_LOCK_STATE]);
}

/* ====================================================================
 * SWR-BCM-009: Auto-lock when speed exceeds threshold (>10)
 * ==================================================================== */

/** @verifies SWR-BCM-009 */
void test_DoorLock_auto_lock_above_10_speed(void)
{
    /* No manual lock command, but speed > 10 */
    mock_rte_signals[BCM_SIG_BODY_CONTROL_CMD] = 0u;
    mock_rte_signals[BCM_SIG_VEHICLE_SPEED]    = 11u;
    mock_rte_signals[BCM_SIG_VEHICLE_STATE]    = BCM_VSTATE_DRIVING;

    Swc_DoorLock_100ms();

    TEST_ASSERT_EQUAL_UINT32(1u, mock_rte_signals[BCM_SIG_DOOR_LOCK_STATE]);
}

/* ====================================================================
 * SWR-BCM-009: Auto-unlock when transitioning to parked state
 * ==================================================================== */

/** @verifies SWR-BCM-009 */
void test_DoorLock_auto_unlock_when_parked(void)
{
    /* First: lock the door via auto-lock (driving, speed > 10) */
    mock_rte_signals[BCM_SIG_BODY_CONTROL_CMD] = 0u;
    mock_rte_signals[BCM_SIG_VEHICLE_SPEED]    = 20u;
    mock_rte_signals[BCM_SIG_VEHICLE_STATE]    = BCM_VSTATE_DRIVING;
    Swc_DoorLock_100ms();
    TEST_ASSERT_EQUAL_UINT32(1u, mock_rte_signals[BCM_SIG_DOOR_LOCK_STATE]);

    /* Now transition to parked (READY state, speed 0) */
    mock_rte_signals[BCM_SIG_VEHICLE_SPEED] = 0u;
    mock_rte_signals[BCM_SIG_VEHICLE_STATE] = BCM_VSTATE_READY;
    Swc_DoorLock_100ms();

    /* Door should auto-unlock on transition to parked */
    TEST_ASSERT_EQUAL_UINT32(0u, mock_rte_signals[BCM_SIG_DOOR_LOCK_STATE]);
}

/* ====================================================================
 * SWR-BCM-009: Not initialized — does nothing
 * ==================================================================== */

/** @verifies SWR-BCM-009 */
void test_DoorLock_not_init_does_nothing(void)
{
    initialized = FALSE;

    mock_rte_signals[BCM_SIG_BODY_CONTROL_CMD] = (1u << 8u);
    mock_rte_signals[BCM_SIG_VEHICLE_SPEED]    = 20u;
    mock_rte_write_count = 0u;

    Swc_DoorLock_100ms();

    /* No RTE writes should occur */
    TEST_ASSERT_EQUAL_UINT8(0u, mock_rte_write_count);
}

/* ====================================================================
 * Test runner
 * ==================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* SWR-BCM-009: Door lock control */
    RUN_TEST(test_DoorLock_init_unlocked);
    RUN_TEST(test_DoorLock_manual_lock_command);
    RUN_TEST(test_DoorLock_auto_lock_above_10_speed);
    RUN_TEST(test_DoorLock_auto_unlock_when_parked);
    RUN_TEST(test_DoorLock_not_init_does_nothing);

    return UNITY_END();
}
