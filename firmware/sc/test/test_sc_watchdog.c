/**
 * @file    test_sc_watchdog.c
 * @brief   Unit tests for sc_watchdog — external watchdog feed control
 * @date    2026-02-23
 *
 * @verifies SWR-SC-022
 *
 * Tests WDI toggle when all conditions met, no toggle when conditions fail,
 * and toggle pattern correctness.
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
typedef uint8               boolean;

#define TRUE                1u
#define FALSE               0u

/* ==================================================================
 * SC Configuration Constants
 * ================================================================== */

#define SC_GIO_PORT_A               0u
#define SC_PIN_WDI                  4u

/* ==================================================================
 * Mock: GIO Register Access
 * ================================================================== */

static uint8 mock_gio_a[8];
static uint8 mock_gio_set_count;

void gioSetBit(uint8 port, uint8 pin, uint8 value)
{
    (void)port;
    if (pin < 8u) {
        mock_gio_a[pin] = value;
    }
    mock_gio_set_count++;
}

uint8 gioGetBit(uint8 port, uint8 pin)
{
    (void)port;
    if (pin < 8u) {
        return mock_gio_a[pin];
    }
    return 0u;
}

/* ==================================================================
 * Include source under test
 * ================================================================== */

#include "../src/sc_watchdog.c"

/* ==================================================================
 * Test setup / teardown
 * ================================================================== */

void setUp(void)
{
    uint8 i;
    for (i = 0u; i < 8u; i++) {
        mock_gio_a[i] = 0u;
    }
    mock_gio_set_count = 0u;
    SC_Watchdog_Init();
}

void tearDown(void) { }

/* ==================================================================
 * SWR-SC-022: Watchdog Feed
 * ================================================================== */

/** @verifies SWR-SC-022 -- Init sets WDI LOW */
void test_WDG_Init_low(void)
{
    TEST_ASSERT_EQUAL_UINT8(0u, mock_gio_a[SC_PIN_WDI]);
}

/** @verifies SWR-SC-022 -- Feed toggles WDI when all checks pass */
void test_WDG_Feed_toggles(void)
{
    SC_Watchdog_Feed(TRUE);
    TEST_ASSERT_EQUAL_UINT8(1u, mock_gio_a[SC_PIN_WDI]);

    SC_Watchdog_Feed(TRUE);
    TEST_ASSERT_EQUAL_UINT8(0u, mock_gio_a[SC_PIN_WDI]);

    SC_Watchdog_Feed(TRUE);
    TEST_ASSERT_EQUAL_UINT8(1u, mock_gio_a[SC_PIN_WDI]);
}

/** @verifies SWR-SC-022 -- Feed does NOT toggle when checks fail */
void test_WDG_Feed_no_toggle_on_fail(void)
{
    SC_Watchdog_Feed(TRUE);
    TEST_ASSERT_EQUAL_UINT8(1u, mock_gio_a[SC_PIN_WDI]);

    uint8 count_before = mock_gio_set_count;
    SC_Watchdog_Feed(FALSE);

    /* WDI should not have changed */
    TEST_ASSERT_EQUAL_UINT8(1u, mock_gio_a[SC_PIN_WDI]);
    TEST_ASSERT_EQUAL_UINT8(count_before, mock_gio_set_count);
}

/** @verifies SWR-SC-022 -- Multiple failed feeds keep WDI stuck */
void test_WDG_Feed_stuck_on_fail(void)
{
    uint8 i;
    for (i = 0u; i < 10u; i++) {
        SC_Watchdog_Feed(FALSE);
    }

    /* WDI should still be at init value (0) */
    TEST_ASSERT_EQUAL_UINT8(0u, mock_gio_a[SC_PIN_WDI]);
}

/* ==================================================================
 * Test runner
 * ================================================================== */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_WDG_Init_low);
    RUN_TEST(test_WDG_Feed_toggles);
    RUN_TEST(test_WDG_Feed_no_toggle_on_fail);
    RUN_TEST(test_WDG_Feed_stuck_on_fail);

    return UNITY_END();
}
