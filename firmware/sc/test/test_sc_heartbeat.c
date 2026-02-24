/**
 * @file    test_sc_heartbeat.c
 * @brief   Unit tests for sc_heartbeat — per-ECU heartbeat monitoring
 * @date    2026-02-23
 *
 * @verifies SWR-SC-004, SWR-SC-005, SWR-SC-006
 *
 * Tests 3-ECU independent timeout counters, 150ms timeout detection,
 * 50ms confirmation window, heartbeat resume during confirmation,
 * and per-ECU fault LED control.
 *
 * Mocks: GIO registers.
 */
#include "unity.h"

/* ==================================================================
 * Local type definitions
 * ================================================================== */

typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned long       uint32;
typedef signed short        sint16;
typedef signed long         sint32;
typedef uint8               boolean;
typedef uint8               Std_ReturnType;

#define TRUE                1u
#define FALSE               0u
#define E_OK                0u
#define E_NOT_OK            1u
#define NULL_PTR            ((void*)0)

/* ==================================================================
 * SC Configuration Constants
 * ================================================================== */

#define SC_ECU_CVC                  0u
#define SC_ECU_FZC                  1u
#define SC_ECU_RZC                  2u
#define SC_ECU_COUNT                3u

#define SC_HB_TIMEOUT_TICKS         15u
#define SC_HB_CONFIRM_TICKS         5u

#define SC_GIO_PORT_A               0u
#define SC_PIN_LED_CVC              1u
#define SC_PIN_LED_FZC              2u
#define SC_PIN_LED_RZC              3u

/* ==================================================================
 * Mock: GIO Register Access
 * ================================================================== */

static uint8 mock_gio_a_state[8];

void gioSetBit(uint8 port, uint8 pin, uint8 value)
{
    (void)port;
    if (pin < 8u) {
        mock_gio_a_state[pin] = value;
    }
}

uint8 gioGetBit(uint8 port, uint8 pin)
{
    (void)port;
    if (pin < 8u) {
        return mock_gio_a_state[pin];
    }
    return 0u;
}

/* ==================================================================
 * Include source under test
 * ================================================================== */

#include "../src/sc_heartbeat.c"

/* ==================================================================
 * Test setup / teardown
 * ================================================================== */

void setUp(void)
{
    uint8 i;
    for (i = 0u; i < 8u; i++) {
        mock_gio_a_state[i] = 0u;
    }
    SC_Heartbeat_Init();
}

void tearDown(void) { }

/* ==================================================================
 * SWR-SC-004: Individual Timeout Counters
 * ================================================================== */

/** @verifies SWR-SC-004 -- Init resets all counters to zero */
void test_HB_Init_resets_all(void)
{
    TEST_ASSERT_FALSE(SC_Heartbeat_IsTimedOut(SC_ECU_CVC));
    TEST_ASSERT_FALSE(SC_Heartbeat_IsTimedOut(SC_ECU_FZC));
    TEST_ASSERT_FALSE(SC_Heartbeat_IsTimedOut(SC_ECU_RZC));
    TEST_ASSERT_FALSE(SC_Heartbeat_IsAnyConfirmed());
}

/** @verifies SWR-SC-004 -- Counter increments each 10ms tick */
void test_HB_counter_increments(void)
{
    uint8 i;

    /* Run 14 ticks — just under timeout */
    for (i = 0u; i < 14u; i++) {
        SC_Heartbeat_Monitor();
    }

    /* Should not be timed out yet (15 ticks = threshold) */
    TEST_ASSERT_FALSE(SC_Heartbeat_IsTimedOut(SC_ECU_CVC));
}

/** @verifies SWR-SC-004 -- Counter resets on valid heartbeat reception */
void test_HB_counter_resets_on_rx(void)
{
    uint8 i;

    /* Run 10 ticks toward timeout */
    for (i = 0u; i < 10u; i++) {
        SC_Heartbeat_Monitor();
    }

    /* Receive heartbeat */
    SC_Heartbeat_NotifyRx(SC_ECU_CVC);

    /* Run 14 more ticks — still should not timeout because counter reset */
    for (i = 0u; i < 14u; i++) {
        SC_Heartbeat_Monitor();
    }

    TEST_ASSERT_FALSE(SC_Heartbeat_IsTimedOut(SC_ECU_CVC));
}

/* ==================================================================
 * SWR-SC-005: 150ms Timeout Detection
 * ================================================================== */

/** @verifies SWR-SC-005 -- Timeout detected at exactly 15 ticks */
void test_HB_timeout_at_15_ticks(void)
{
    uint8 i;
    for (i = 0u; i < SC_HB_TIMEOUT_TICKS; i++) {
        SC_Heartbeat_Monitor();
    }

    TEST_ASSERT_TRUE(SC_Heartbeat_IsTimedOut(SC_ECU_CVC));
    TEST_ASSERT_TRUE(SC_Heartbeat_IsTimedOut(SC_ECU_FZC));
    TEST_ASSERT_TRUE(SC_Heartbeat_IsTimedOut(SC_ECU_RZC));
}

/** @verifies SWR-SC-005 -- Only one ECU times out when others receive HB */
void test_HB_independent_ecus(void)
{
    uint8 i;

    for (i = 0u; i < SC_HB_TIMEOUT_TICKS; i++) {
        /* FZC and RZC receive heartbeats, CVC does not */
        SC_Heartbeat_NotifyRx(SC_ECU_FZC);
        SC_Heartbeat_NotifyRx(SC_ECU_RZC);
        SC_Heartbeat_Monitor();
    }

    TEST_ASSERT_TRUE(SC_Heartbeat_IsTimedOut(SC_ECU_CVC));
    TEST_ASSERT_FALSE(SC_Heartbeat_IsTimedOut(SC_ECU_FZC));
    TEST_ASSERT_FALSE(SC_Heartbeat_IsTimedOut(SC_ECU_RZC));
}

/** @verifies SWR-SC-005 -- Fault LED set HIGH on timeout */
void test_HB_fault_led_on_timeout(void)
{
    uint8 i;
    for (i = 0u; i < SC_HB_TIMEOUT_TICKS; i++) {
        SC_Heartbeat_Monitor();
    }

    TEST_ASSERT_EQUAL_UINT8(1u, mock_gio_a_state[SC_PIN_LED_CVC]);
    TEST_ASSERT_EQUAL_UINT8(1u, mock_gio_a_state[SC_PIN_LED_FZC]);
    TEST_ASSERT_EQUAL_UINT8(1u, mock_gio_a_state[SC_PIN_LED_RZC]);
}

/* ==================================================================
 * SWR-SC-006: 50ms Confirmation Window
 * ================================================================== */

/** @verifies SWR-SC-006 -- Timeout not confirmed until 200ms total */
void test_HB_confirmation_window(void)
{
    uint8 i;

    /* 15 ticks = timeout detected (150ms) */
    for (i = 0u; i < SC_HB_TIMEOUT_TICKS; i++) {
        SC_Heartbeat_Monitor();
    }
    TEST_ASSERT_TRUE(SC_Heartbeat_IsTimedOut(SC_ECU_CVC));
    TEST_ASSERT_FALSE(SC_Heartbeat_IsAnyConfirmed());

    /* 4 more ticks = still in confirmation window */
    for (i = 0u; i < (SC_HB_CONFIRM_TICKS - 1u); i++) {
        SC_Heartbeat_Monitor();
    }
    TEST_ASSERT_FALSE(SC_Heartbeat_IsAnyConfirmed());

    /* 5th tick = confirmed */
    SC_Heartbeat_Monitor();
    TEST_ASSERT_TRUE(SC_Heartbeat_IsAnyConfirmed());
}

/** @verifies SWR-SC-006 -- Heartbeat resume during confirmation cancels */
void test_HB_resume_cancels_confirmation(void)
{
    uint8 i;

    /* Enter timeout state */
    for (i = 0u; i < SC_HB_TIMEOUT_TICKS; i++) {
        SC_Heartbeat_Monitor();
    }
    TEST_ASSERT_TRUE(SC_Heartbeat_IsTimedOut(SC_ECU_CVC));

    /* Heartbeat resumes for all ECUs during confirmation window */
    SC_Heartbeat_NotifyRx(SC_ECU_CVC);
    SC_Heartbeat_NotifyRx(SC_ECU_FZC);
    SC_Heartbeat_NotifyRx(SC_ECU_RZC);

    /* Continue monitoring — should not reach confirmed */
    for (i = 0u; i < SC_HB_CONFIRM_TICKS; i++) {
        SC_Heartbeat_Monitor();
    }

    /* Not confirmed because heartbeats resumed */
    TEST_ASSERT_FALSE(SC_Heartbeat_IsAnyConfirmed());
}

/** @verifies SWR-SC-006 -- Confirmation requires ALL to be timed out for any one confirmed */
void test_HB_partial_resume(void)
{
    uint8 i;

    /* All ECUs time out */
    for (i = 0u; i < SC_HB_TIMEOUT_TICKS; i++) {
        SC_Heartbeat_Monitor();
    }

    /* Only CVC resumes — FZC and RZC still timed out */
    SC_Heartbeat_NotifyRx(SC_ECU_CVC);

    /* FZC and RZC continue through confirmation window */
    for (i = 0u; i < SC_HB_CONFIRM_TICKS; i++) {
        SC_Heartbeat_Monitor();
    }

    /* FZC or RZC should be confirmed */
    TEST_ASSERT_TRUE(SC_Heartbeat_IsAnyConfirmed());
    TEST_ASSERT_FALSE(SC_Heartbeat_IsTimedOut(SC_ECU_CVC));
}

/** @verifies SWR-SC-004 -- Invalid ECU index handled safely */
void test_HB_invalid_ecu_index(void)
{
    TEST_ASSERT_FALSE(SC_Heartbeat_IsTimedOut(SC_ECU_COUNT));
    TEST_ASSERT_FALSE(SC_Heartbeat_IsTimedOut(0xFFu));
}

/** @verifies SWR-SC-004 -- NotifyRx with invalid index is safe no-op */
void test_HB_notify_invalid_index(void)
{
    /* Should not crash */
    SC_Heartbeat_NotifyRx(SC_ECU_COUNT);
    SC_Heartbeat_NotifyRx(0xFFu);

    TEST_ASSERT_FALSE(SC_Heartbeat_IsAnyConfirmed());
}

/* ==================================================================
 * HARDENED TESTS — Boundary Values, Fault Injection
 * ================================================================== */

/** @verifies SWR-SC-005
 *  Equivalence class: Boundary — timeout at exactly 14 ticks (one under) */
void test_hb_timeout_at_14_no_fault(void)
{
    uint8 i;
    for (i = 0u; i < (SC_HB_TIMEOUT_TICKS - 1u); i++) {
        SC_Heartbeat_Monitor();
    }

    TEST_ASSERT_FALSE(SC_Heartbeat_IsTimedOut(SC_ECU_CVC));
    TEST_ASSERT_FALSE(SC_Heartbeat_IsTimedOut(SC_ECU_FZC));
    TEST_ASSERT_FALSE(SC_Heartbeat_IsTimedOut(SC_ECU_RZC));
}

/** @verifies SWR-SC-006
 *  Equivalence class: Boundary — confirmation at exactly 4 ticks (one under) */
void test_hb_confirmation_at_4_not_confirmed(void)
{
    uint8 i;
    for (i = 0u; i < SC_HB_TIMEOUT_TICKS; i++) {
        SC_Heartbeat_Monitor();
    }
    for (i = 0u; i < (SC_HB_CONFIRM_TICKS - 1u); i++) {
        SC_Heartbeat_Monitor();
    }

    TEST_ASSERT_TRUE(SC_Heartbeat_IsTimedOut(SC_ECU_CVC));
    TEST_ASSERT_FALSE(SC_Heartbeat_IsAnyConfirmed());
}

/** @verifies SWR-SC-005
 *  Equivalence class: Boundary — rapid notify/monitor interleaving */
void test_hb_rapid_notify_monitor(void)
{
    uint8 i;
    for (i = 0u; i < 100u; i++) {
        SC_Heartbeat_NotifyRx(SC_ECU_CVC);
        SC_Heartbeat_NotifyRx(SC_ECU_FZC);
        SC_Heartbeat_NotifyRx(SC_ECU_RZC);
        SC_Heartbeat_Monitor();
    }

    TEST_ASSERT_FALSE(SC_Heartbeat_IsTimedOut(SC_ECU_CVC));
    TEST_ASSERT_FALSE(SC_Heartbeat_IsTimedOut(SC_ECU_FZC));
    TEST_ASSERT_FALSE(SC_Heartbeat_IsTimedOut(SC_ECU_RZC));
    TEST_ASSERT_FALSE(SC_Heartbeat_IsAnyConfirmed());
}

/** @verifies SWR-SC-006
 *  Equivalence class: Fault injection — single ECU timeout then resume
 *  after confirmation starts */
void test_hb_single_ecu_late_resume(void)
{
    uint8 i;

    /* All ECUs timeout */
    for (i = 0u; i < SC_HB_TIMEOUT_TICKS; i++) {
        SC_Heartbeat_Monitor();
    }

    /* Start confirmation: 2 ticks in */
    SC_Heartbeat_Monitor();
    SC_Heartbeat_Monitor();

    /* RZC resumes but CVC and FZC remain timed out */
    SC_Heartbeat_NotifyRx(SC_ECU_RZC);

    /* Complete confirmation window */
    for (i = 0u; i < (SC_HB_CONFIRM_TICKS - 2u); i++) {
        SC_Heartbeat_Monitor();
    }

    /* CVC and FZC should be confirmed */
    TEST_ASSERT_TRUE(SC_Heartbeat_IsAnyConfirmed());
    TEST_ASSERT_FALSE(SC_Heartbeat_IsTimedOut(SC_ECU_RZC));
}

/** @verifies SWR-SC-005
 *  Equivalence class: Boundary — LED cleared when heartbeat resumes before confirmation */
void test_hb_led_cleared_on_resume(void)
{
    uint8 i;

    /* Trigger timeout to set LEDs */
    for (i = 0u; i < SC_HB_TIMEOUT_TICKS; i++) {
        SC_Heartbeat_Monitor();
    }
    TEST_ASSERT_EQUAL_UINT8(1u, mock_gio_a_state[SC_PIN_LED_CVC]);

    /* Heartbeat resumes */
    SC_Heartbeat_NotifyRx(SC_ECU_CVC);
    for (i = 0u; i < 5u; i++) {
        SC_Heartbeat_Monitor();
    }

    /* LED should be cleared for CVC */
    TEST_ASSERT_EQUAL_UINT8(0u, mock_gio_a_state[SC_PIN_LED_CVC]);
}

/* ==================================================================
 * Test runner
 * ================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* SWR-SC-004: Individual counters */
    RUN_TEST(test_HB_Init_resets_all);
    RUN_TEST(test_HB_counter_increments);
    RUN_TEST(test_HB_counter_resets_on_rx);

    /* SWR-SC-005: Timeout detection */
    RUN_TEST(test_HB_timeout_at_15_ticks);
    RUN_TEST(test_HB_independent_ecus);
    RUN_TEST(test_HB_fault_led_on_timeout);

    /* SWR-SC-006: Confirmation window */
    RUN_TEST(test_HB_confirmation_window);
    RUN_TEST(test_HB_resume_cancels_confirmation);
    RUN_TEST(test_HB_partial_resume);

    /* Boundary / safety */
    RUN_TEST(test_HB_invalid_ecu_index);
    RUN_TEST(test_HB_notify_invalid_index);

    /* Hardened tests — boundary values, fault injection */
    RUN_TEST(test_hb_timeout_at_14_no_fault);
    RUN_TEST(test_hb_confirmation_at_4_not_confirmed);
    RUN_TEST(test_hb_rapid_notify_monitor);
    RUN_TEST(test_hb_single_ecu_late_resume);
    RUN_TEST(test_hb_led_cleared_on_resume);

    return UNITY_END();
}
