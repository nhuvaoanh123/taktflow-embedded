/**
 * @file    test_sc_led.c
 * @brief   Unit tests for sc_led — fault LED panel driver
 * @date    2026-02-23
 *
 * @verifies SWR-SC-013
 *
 * Tests LED initialization, steady ON, blink pattern, and OFF state.
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
#define SC_GIO_PORT_B               1u
#define SC_PIN_LED_CVC              1u
#define SC_PIN_LED_FZC              2u
#define SC_PIN_LED_RZC              3u
#define SC_PIN_LED_SYS              4u
#define SC_LED_BLINK_ON_TICKS       25u
#define SC_LED_BLINK_OFF_TICKS      25u
#define SC_LED_BLINK_PERIOD         50u

#define SC_LED_OFF                  0u
#define SC_LED_BLINK                1u
#define SC_LED_ON                   2u

/* ==================================================================
 * Mock: GIO Register Access
 * ================================================================== */

static uint8 mock_gio_a[8];
static uint8 mock_gio_b[8];

void gioSetBit(uint8 port, uint8 pin, uint8 value)
{
    if ((port == SC_GIO_PORT_A) && (pin < 8u)) {
        mock_gio_a[pin] = value;
    } else if ((port == SC_GIO_PORT_B) && (pin < 8u)) {
        mock_gio_b[pin] = value;
    }
}

/* ==================================================================
 * Include source under test
 * ================================================================== */

#include "../src/sc_led.c"

/* ==================================================================
 * Test setup / teardown
 * ================================================================== */

void setUp(void)
{
    uint8 i;
    for (i = 0u; i < 8u; i++) {
        mock_gio_a[i] = 0u;
        mock_gio_b[i] = 0u;
    }
    SC_LED_Init();
}

void tearDown(void) { }

/* ==================================================================
 * SWR-SC-013: LED States
 * ================================================================== */

/** @verifies SWR-SC-013 -- Init turns all LEDs off */
void test_LED_Init_all_off(void)
{
    TEST_ASSERT_EQUAL_UINT8(0u, mock_gio_a[SC_PIN_LED_CVC]);
    TEST_ASSERT_EQUAL_UINT8(0u, mock_gio_a[SC_PIN_LED_FZC]);
    TEST_ASSERT_EQUAL_UINT8(0u, mock_gio_a[SC_PIN_LED_RZC]);
    TEST_ASSERT_EQUAL_UINT8(0u, mock_gio_a[SC_PIN_LED_SYS]);
}

/** @verifies SWR-SC-013 -- Steady ON drives LED HIGH */
void test_LED_steady_on(void)
{
    SC_LED_SetState(SC_LED_IDX_CVC, SC_LED_ON);
    SC_LED_Update();

    TEST_ASSERT_EQUAL_UINT8(1u, mock_gio_a[SC_PIN_LED_CVC]);
}

/** @verifies SWR-SC-013 -- Blink toggles with 500ms period */
void test_LED_blink_pattern(void)
{
    uint8 i;
    SC_LED_SetState(SC_LED_IDX_FZC, SC_LED_BLINK);

    /* First 25 ticks: ON */
    for (i = 0u; i < SC_LED_BLINK_ON_TICKS; i++) {
        SC_LED_Update();
    }
    TEST_ASSERT_EQUAL_UINT8(1u, mock_gio_a[SC_PIN_LED_FZC]);

    /* Next 25 ticks: OFF */
    for (i = 0u; i < SC_LED_BLINK_OFF_TICKS; i++) {
        SC_LED_Update();
    }
    TEST_ASSERT_EQUAL_UINT8(0u, mock_gio_a[SC_PIN_LED_FZC]);
}

/** @verifies SWR-SC-013 -- OFF state keeps LED low */
void test_LED_off_stays_low(void)
{
    SC_LED_SetState(SC_LED_IDX_RZC, SC_LED_ON);
    SC_LED_Update();
    TEST_ASSERT_EQUAL_UINT8(1u, mock_gio_a[SC_PIN_LED_RZC]);

    SC_LED_SetState(SC_LED_IDX_RZC, SC_LED_OFF);
    SC_LED_Update();
    TEST_ASSERT_EQUAL_UINT8(0u, mock_gio_a[SC_PIN_LED_RZC]);
}

/* ==================================================================
 * Test runner
 * ================================================================== */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_LED_Init_all_off);
    RUN_TEST(test_LED_steady_on);
    RUN_TEST(test_LED_blink_pattern);
    RUN_TEST(test_LED_off_stays_low);

    return UNITY_END();
}
