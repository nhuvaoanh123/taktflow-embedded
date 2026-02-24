/**
 * @file    test_sc_relay.c
 * @brief   Unit tests for sc_relay — kill relay GPIO control
 * @date    2026-02-23
 *
 * @verifies SWR-SC-010, SWR-SC-011, SWR-SC-012
 *
 * Tests relay initialization (LOW), energize/de-energize, kill latch,
 * readback verification, and all de-energize trigger conditions.
 *
 * Mocks: GIO registers, heartbeat/plausibility/selftest state getters.
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

#define SC_GIO_PORT_A               0u
#define SC_PIN_RELAY                0u
#define SC_RELAY_READBACK_THRESHOLD 2u

/* ==================================================================
 * Mock: GIO Register Access
 * ================================================================== */

static uint8 mock_gio_a_dout[8];
static uint8 mock_gio_a_din[8];

void gioSetBit(uint8 port, uint8 pin, uint8 value)
{
    (void)port;
    if (pin < 8u) {
        mock_gio_a_dout[pin] = value;
        /* By default, readback matches output */
        mock_gio_a_din[pin] = value;
    }
}

uint8 gioGetBit(uint8 port, uint8 pin)
{
    (void)port;
    if (pin < 8u) {
        return mock_gio_a_din[pin];
    }
    return 0u;
}

/* ==================================================================
 * Mock: Module state getters
 * ================================================================== */

static boolean mock_hb_any_confirmed;
static boolean mock_plaus_faulted;
static boolean mock_selftest_healthy;
static boolean mock_esm_error;
static boolean mock_can_bus_off;

boolean SC_Heartbeat_IsAnyConfirmed(void)
{
    return mock_hb_any_confirmed;
}

boolean SC_Plausibility_IsFaulted(void)
{
    return mock_plaus_faulted;
}

boolean SC_SelfTest_IsHealthy(void)
{
    return mock_selftest_healthy;
}

boolean SC_ESM_IsErrorActive(void)
{
    return mock_esm_error;
}

boolean SC_CAN_IsBusOff(void)
{
    return mock_can_bus_off;
}

/* ==================================================================
 * Include source under test
 * ================================================================== */

#include "../src/sc_relay.c"

/* ==================================================================
 * Test setup / teardown
 * ================================================================== */

void setUp(void)
{
    uint8 i;
    for (i = 0u; i < 8u; i++) {
        mock_gio_a_dout[i] = 0u;
        mock_gio_a_din[i]  = 0u;
    }

    mock_hb_any_confirmed = FALSE;
    mock_plaus_faulted    = FALSE;
    mock_selftest_healthy = TRUE;
    mock_esm_error        = FALSE;
    mock_can_bus_off      = FALSE;

    SC_Relay_Init();
}

void tearDown(void) { }

/* ==================================================================
 * SWR-SC-010: Initialization
 * ================================================================== */

/** @verifies SWR-SC-010 -- Init sets relay LOW (safe state) */
void test_Relay_Init_low(void)
{
    TEST_ASSERT_EQUAL_UINT8(0u, mock_gio_a_dout[SC_PIN_RELAY]);
    TEST_ASSERT_FALSE(SC_Relay_IsKilled());
}

/* ==================================================================
 * SWR-SC-010: Energize / De-energize
 * ================================================================== */

/** @verifies SWR-SC-010 -- Energize sets relay HIGH */
void test_Relay_Energize(void)
{
    SC_Relay_Energize();

    TEST_ASSERT_EQUAL_UINT8(1u, mock_gio_a_dout[SC_PIN_RELAY]);
}

/** @verifies SWR-SC-010 -- De-energize sets relay LOW and latches */
void test_Relay_DeEnergize(void)
{
    SC_Relay_Energize();
    SC_Relay_DeEnergize();

    TEST_ASSERT_EQUAL_UINT8(0u, mock_gio_a_dout[SC_PIN_RELAY]);
    TEST_ASSERT_TRUE(SC_Relay_IsKilled());
}

/* ==================================================================
 * SWR-SC-011: Kill Latch
 * ================================================================== */

/** @verifies SWR-SC-011 -- Once killed, energize has no effect */
void test_Relay_kill_latch(void)
{
    SC_Relay_Energize();
    SC_Relay_DeEnergize();
    TEST_ASSERT_TRUE(SC_Relay_IsKilled());

    /* Try to re-energize — should be blocked by latch */
    SC_Relay_Energize();
    TEST_ASSERT_EQUAL_UINT8(0u, mock_gio_a_dout[SC_PIN_RELAY]);
}

/* ==================================================================
 * SWR-SC-012: De-energize Triggers
 * ================================================================== */

/** @verifies SWR-SC-012 -- Heartbeat confirmed timeout triggers kill */
void test_Relay_trigger_heartbeat(void)
{
    SC_Relay_Energize();
    mock_hb_any_confirmed = TRUE;

    SC_Relay_CheckTriggers();

    TEST_ASSERT_EQUAL_UINT8(0u, mock_gio_a_dout[SC_PIN_RELAY]);
    TEST_ASSERT_TRUE(SC_Relay_IsKilled());
}

/** @verifies SWR-SC-012 -- Plausibility fault triggers kill */
void test_Relay_trigger_plausibility(void)
{
    SC_Relay_Energize();
    mock_plaus_faulted = TRUE;

    SC_Relay_CheckTriggers();

    TEST_ASSERT_TRUE(SC_Relay_IsKilled());
}

/** @verifies SWR-SC-012 -- Self-test failure triggers kill */
void test_Relay_trigger_selftest(void)
{
    SC_Relay_Energize();
    mock_selftest_healthy = FALSE;

    SC_Relay_CheckTriggers();

    TEST_ASSERT_TRUE(SC_Relay_IsKilled());
}

/** @verifies SWR-SC-012 -- ESM lockstep error triggers kill */
void test_Relay_trigger_esm(void)
{
    SC_Relay_Energize();
    mock_esm_error = TRUE;

    SC_Relay_CheckTriggers();

    TEST_ASSERT_TRUE(SC_Relay_IsKilled());
}

/** @verifies SWR-SC-012 -- GPIO readback mismatch (2 consecutive) triggers kill */
void test_Relay_trigger_readback_mismatch(void)
{
    SC_Relay_Energize();

    /* Force readback to differ from output */
    mock_gio_a_din[SC_PIN_RELAY] = 0u;  /* Output is 1, readback is 0 */

    SC_Relay_CheckTriggers();  /* Mismatch 1 */
    TEST_ASSERT_FALSE(SC_Relay_IsKilled());  /* Not yet — need 2 */

    SC_Relay_CheckTriggers();  /* Mismatch 2 */
    TEST_ASSERT_TRUE(SC_Relay_IsKilled());
}

/** @verifies SWR-SC-012 -- Readback mismatch counter resets on match */
void test_Relay_readback_resets(void)
{
    SC_Relay_Energize();

    /* Force 1 mismatch */
    mock_gio_a_din[SC_PIN_RELAY] = 0u;
    SC_Relay_CheckTriggers();

    /* Readback matches again */
    mock_gio_a_din[SC_PIN_RELAY] = 1u;
    SC_Relay_CheckTriggers();

    /* Another single mismatch — counter was reset, so no kill */
    mock_gio_a_din[SC_PIN_RELAY] = 0u;
    SC_Relay_CheckTriggers();

    TEST_ASSERT_FALSE(SC_Relay_IsKilled());
}

/** @verifies SWR-SC-012 -- No triggers = relay stays energized */
void test_Relay_no_trigger(void)
{
    SC_Relay_Energize();

    SC_Relay_CheckTriggers();
    SC_Relay_CheckTriggers();
    SC_Relay_CheckTriggers();

    TEST_ASSERT_EQUAL_UINT8(1u, mock_gio_a_dout[SC_PIN_RELAY]);
    TEST_ASSERT_FALSE(SC_Relay_IsKilled());
}

/* ==================================================================
 * HARDENED TESTS — Boundary Values, Fault Injection
 * ================================================================== */

/** @verifies SWR-SC-012
 *  Equivalence class: Fault injection — CAN bus-off triggers kill */
void test_relay_trigger_can_busoff(void)
{
    SC_Relay_Energize();
    mock_can_bus_off = TRUE;

    SC_Relay_CheckTriggers();

    TEST_ASSERT_TRUE(SC_Relay_IsKilled());
}

/** @verifies SWR-SC-012
 *  Equivalence class: Fault injection — multiple triggers simultaneously */
void test_relay_trigger_multiple_simultaneous(void)
{
    SC_Relay_Energize();
    mock_hb_any_confirmed = TRUE;
    mock_plaus_faulted    = TRUE;
    mock_esm_error        = TRUE;

    SC_Relay_CheckTriggers();

    TEST_ASSERT_TRUE(SC_Relay_IsKilled());
    TEST_ASSERT_EQUAL_UINT8(0u, mock_gio_a_dout[SC_PIN_RELAY]);
}

/** @verifies SWR-SC-011
 *  Equivalence class: Boundary — kill latch survives Init re-call */
void test_relay_kill_survives_double_init(void)
{
    SC_Relay_Energize();
    SC_Relay_DeEnergize();
    TEST_ASSERT_TRUE(SC_Relay_IsKilled());

    /* Re-init should NOT clear the kill latch */
    SC_Relay_Init();
    TEST_ASSERT_TRUE(SC_Relay_IsKilled());
}

/** @verifies SWR-SC-010
 *  Equivalence class: Boundary — de-energize without prior energize */
void test_relay_deenergize_without_energize(void)
{
    /* Relay starts LOW after Init; de-energize should be safe */
    SC_Relay_DeEnergize();

    TEST_ASSERT_EQUAL_UINT8(0u, mock_gio_a_dout[SC_PIN_RELAY]);
    TEST_ASSERT_TRUE(SC_Relay_IsKilled());
}

/** @verifies SWR-SC-012
 *  Equivalence class: Boundary — readback mismatch counter at exactly 1 (under threshold) */
void test_relay_readback_exactly_1_no_kill(void)
{
    SC_Relay_Energize();

    mock_gio_a_din[SC_PIN_RELAY] = 0u;
    SC_Relay_CheckTriggers();

    TEST_ASSERT_FALSE(SC_Relay_IsKilled());
}

/** @verifies SWR-SC-012
 *  Equivalence class: Fault injection — check triggers with relay not energized */
void test_relay_check_triggers_not_energized(void)
{
    /* Relay is LOW (not energized), check triggers should be safe */
    SC_Relay_CheckTriggers();

    TEST_ASSERT_FALSE(SC_Relay_IsKilled());
}

/* ==================================================================
 * Test runner
 * ================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* SWR-SC-010: Init */
    RUN_TEST(test_Relay_Init_low);

    /* SWR-SC-010: Energize / De-energize */
    RUN_TEST(test_Relay_Energize);
    RUN_TEST(test_Relay_DeEnergize);

    /* SWR-SC-011: Kill Latch */
    RUN_TEST(test_Relay_kill_latch);

    /* SWR-SC-012: Triggers */
    RUN_TEST(test_Relay_trigger_heartbeat);
    RUN_TEST(test_Relay_trigger_plausibility);
    RUN_TEST(test_Relay_trigger_selftest);
    RUN_TEST(test_Relay_trigger_esm);
    RUN_TEST(test_Relay_trigger_readback_mismatch);
    RUN_TEST(test_Relay_readback_resets);
    RUN_TEST(test_Relay_no_trigger);

    /* Hardened tests — boundary values, fault injection */
    RUN_TEST(test_relay_trigger_can_busoff);
    RUN_TEST(test_relay_trigger_multiple_simultaneous);
    RUN_TEST(test_relay_kill_survives_double_init);
    RUN_TEST(test_relay_deenergize_without_energize);
    RUN_TEST(test_relay_readback_exactly_1_no_kill);
    RUN_TEST(test_relay_check_triggers_not_energized);

    return UNITY_END();
}
