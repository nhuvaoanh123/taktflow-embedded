/**
 * @file    test_Swc_Dashboard.c
 * @brief   Unit tests for Swc_Dashboard — ncurses instrument cluster dashboard
 * @date    2026-02-23
 *
 * @verifies SWR-ICU-002, SWR-ICU-003, SWR-ICU-004, SWR-ICU-005,
 *           SWR-ICU-006, SWR-ICU-007, SWR-ICU-009
 *
 * Tests dashboard initialization, speed computation from RPM, torque
 * percentage display, temperature zones, battery voltage zones, warning
 * flags, vehicle state text, and ECU health timeout detection.
 *
 * Data processing logic ONLY — ncurses rendering is excluded via
 * PLATFORM_POSIX_TEST guard.
 *
 * Mocks: Rte_Read
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
#define SWC_DASHBOARD_H
#define ICU_CFG_H

/* Prevent ncurses inclusion in test builds */
#define PLATFORM_POSIX_TEST

/* ====================================================================
 * Signal IDs (must match Icu_Cfg.h)
 * ==================================================================== */

#define ICU_SIG_MOTOR_RPM         16u
#define ICU_SIG_TORQUE_PCT        17u
#define ICU_SIG_MOTOR_TEMP        18u
#define ICU_SIG_BATTERY_VOLTAGE   19u
#define ICU_SIG_VEHICLE_STATE     20u
#define ICU_SIG_ESTOP_ACTIVE      21u
#define ICU_SIG_HEARTBEAT_CVC     22u
#define ICU_SIG_HEARTBEAT_FZC     23u
#define ICU_SIG_HEARTBEAT_RZC     24u
#define ICU_SIG_OVERCURRENT_FLAG  25u
#define ICU_SIG_LIGHT_STATUS      26u
#define ICU_SIG_INDICATOR_STATE   27u
#define ICU_SIG_DTC_BROADCAST     28u
#define ICU_SIG_COUNT             29u

/* Temperature zone thresholds */
#define ICU_TEMP_GREEN_MAX        59u
#define ICU_TEMP_YELLOW_MAX       79u
#define ICU_TEMP_ORANGE_MAX       99u

/* Battery voltage zone thresholds (millivolts) */
#define ICU_BATT_GREEN_MIN      11000u
#define ICU_BATT_YELLOW_MIN     10000u

/* Heartbeat timeout (ticks at 50ms) */
#define ICU_HB_TIMEOUT_TICKS       4u

/* Temperature/battery zone enums */
#define DASH_ZONE_GREEN   0u
#define DASH_ZONE_YELLOW  1u
#define DASH_ZONE_ORANGE  2u
#define DASH_ZONE_RED     3u

/* Warning flags */
#define DASH_WARN_CHECK_ENGINE  0x01u
#define DASH_WARN_TEMPERATURE   0x02u
#define DASH_WARN_BATTERY       0x04u
#define DASH_WARN_ESTOP         0x08u
#define DASH_WARN_OVERCURRENT   0x10u

/* Vehicle states */
#define ICU_VSTATE_INIT       0u
#define ICU_VSTATE_RUN        1u
#define ICU_VSTATE_DEGRADED   2u
#define ICU_VSTATE_LIMP       3u
#define ICU_VSTATE_SAFE_STOP  4u
#define ICU_VSTATE_SHUTDOWN   5u

/* ====================================================================
 * Mock: Rte_Read — inject signal values via array
 * ==================================================================== */

#define MOCK_RTE_MAX_SIGNALS  64u

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
 * Include source under test (source inclusion for test build)
 * ==================================================================== */

#include "../src/Swc_Dashboard.c"

/* ====================================================================
 * setUp / tearDown
 * ==================================================================== */

void setUp(void)
{
    uint8 i;

    for (i = 0u; i < MOCK_RTE_MAX_SIGNALS; i++) {
        mock_rte_signals[i] = 0u;
    }

    /* Default: vehicle in RUN state, healthy battery, cool temp */
    mock_rte_signals[ICU_SIG_VEHICLE_STATE]    = ICU_VSTATE_RUN;
    mock_rte_signals[ICU_SIG_BATTERY_VOLTAGE]  = 12000u;  /* 12V = green */
    mock_rte_signals[ICU_SIG_MOTOR_TEMP]       = 40u;     /* 40C = green */
    mock_rte_signals[ICU_SIG_ESTOP_ACTIVE]     = 0u;
    mock_rte_signals[ICU_SIG_OVERCURRENT_FLAG] = 0u;
    mock_rte_signals[ICU_SIG_HEARTBEAT_CVC]    = 0u;
    mock_rte_signals[ICU_SIG_HEARTBEAT_FZC]    = 0u;
    mock_rte_signals[ICU_SIG_HEARTBEAT_RZC]    = 0u;

    Swc_Dashboard_Init();
}

void tearDown(void) { }

/* ====================================================================
 * SWR-ICU-002: Init — all display values zeroed
 * ==================================================================== */

/** @verifies SWR-ICU-002 */
void test_Dashboard_init_all_zero(void)
{
    /* After init, computed values should be zeroed */
    TEST_ASSERT_EQUAL_UINT32(0u, dash_data.display_speed);
    TEST_ASSERT_EQUAL_UINT32(0u, dash_data.display_torque);
    TEST_ASSERT_EQUAL_UINT8(DASH_ZONE_GREEN, dash_data.temp_zone);
    TEST_ASSERT_EQUAL_UINT8(DASH_ZONE_GREEN, dash_data.battery_zone);
    TEST_ASSERT_EQUAL_UINT8(0u, dash_data.warning_flags);
    TEST_ASSERT_TRUE(initialized == TRUE);
}

/* ====================================================================
 * SWR-ICU-003: Speed computed from motor RPM
 * ==================================================================== */

/** @verifies SWR-ICU-003 */
void test_Dashboard_speed_from_rpm(void)
{
    /* motor_rpm = 3000 -> speed = 3000 * 60 / 1000 = 180 km/h */
    mock_rte_signals[ICU_SIG_MOTOR_RPM] = 3000u;

    Swc_Dashboard_50ms();

    TEST_ASSERT_EQUAL_UINT32(180u, dash_data.display_speed);
}

/* ====================================================================
 * SWR-ICU-003: Speed zero when RPM is zero
 * ==================================================================== */

/** @verifies SWR-ICU-003 */
void test_Dashboard_speed_zero_rpm(void)
{
    mock_rte_signals[ICU_SIG_MOTOR_RPM] = 0u;

    Swc_Dashboard_50ms();

    TEST_ASSERT_EQUAL_UINT32(0u, dash_data.display_speed);
}

/* ====================================================================
 * SWR-ICU-004: Torque percentage direct pass-through
 * ==================================================================== */

/** @verifies SWR-ICU-004 */
void test_Dashboard_torque_percentage(void)
{
    mock_rte_signals[ICU_SIG_TORQUE_PCT] = 75u;

    Swc_Dashboard_50ms();

    TEST_ASSERT_EQUAL_UINT32(75u, dash_data.display_torque);
}

/* ====================================================================
 * SWR-ICU-005: Temperature zone GREEN (< 60C)
 * ==================================================================== */

/** @verifies SWR-ICU-005 */
void test_Dashboard_temp_zone_green(void)
{
    mock_rte_signals[ICU_SIG_MOTOR_TEMP] = 45u;

    Swc_Dashboard_50ms();

    TEST_ASSERT_EQUAL_UINT8(DASH_ZONE_GREEN, dash_data.temp_zone);
}

/* ====================================================================
 * SWR-ICU-005: Temperature zone YELLOW (60-79C)
 * ==================================================================== */

/** @verifies SWR-ICU-005 */
void test_Dashboard_temp_zone_yellow(void)
{
    mock_rte_signals[ICU_SIG_MOTOR_TEMP] = 70u;

    Swc_Dashboard_50ms();

    TEST_ASSERT_EQUAL_UINT8(DASH_ZONE_YELLOW, dash_data.temp_zone);
}

/* ====================================================================
 * SWR-ICU-005: Temperature zone ORANGE (80-99C)
 * ==================================================================== */

/** @verifies SWR-ICU-005 */
void test_Dashboard_temp_zone_orange(void)
{
    mock_rte_signals[ICU_SIG_MOTOR_TEMP] = 90u;

    Swc_Dashboard_50ms();

    TEST_ASSERT_EQUAL_UINT8(DASH_ZONE_ORANGE, dash_data.temp_zone);
}

/* ====================================================================
 * SWR-ICU-005: Temperature zone RED (>= 100C)
 * ==================================================================== */

/** @verifies SWR-ICU-005 */
void test_Dashboard_temp_zone_red(void)
{
    mock_rte_signals[ICU_SIG_MOTOR_TEMP] = 105u;

    Swc_Dashboard_50ms();

    TEST_ASSERT_EQUAL_UINT8(DASH_ZONE_RED, dash_data.temp_zone);

    /* Also check boundary: exactly 100 */
    mock_rte_signals[ICU_SIG_MOTOR_TEMP] = 100u;
    Swc_Dashboard_50ms();
    TEST_ASSERT_EQUAL_UINT8(DASH_ZONE_RED, dash_data.temp_zone);
}

/* ====================================================================
 * SWR-ICU-006: Battery zone GREEN (> 11000mV)
 * ==================================================================== */

/** @verifies SWR-ICU-006 */
void test_Dashboard_battery_zone_green(void)
{
    mock_rte_signals[ICU_SIG_BATTERY_VOLTAGE] = 12500u;

    Swc_Dashboard_50ms();

    TEST_ASSERT_EQUAL_UINT8(DASH_ZONE_GREEN, dash_data.battery_zone);
}

/* ====================================================================
 * SWR-ICU-006: Battery zone YELLOW (10000-11000mV)
 * ==================================================================== */

/** @verifies SWR-ICU-006 */
void test_Dashboard_battery_zone_yellow(void)
{
    mock_rte_signals[ICU_SIG_BATTERY_VOLTAGE] = 10500u;

    Swc_Dashboard_50ms();

    TEST_ASSERT_EQUAL_UINT8(DASH_ZONE_YELLOW, dash_data.battery_zone);

    /* Boundary: exactly 11000 is yellow (not strictly > 11000) */
    mock_rte_signals[ICU_SIG_BATTERY_VOLTAGE] = 11000u;
    Swc_Dashboard_50ms();
    TEST_ASSERT_EQUAL_UINT8(DASH_ZONE_YELLOW, dash_data.battery_zone);
}

/* ====================================================================
 * SWR-ICU-006: Battery zone RED (< 10000mV)
 * ==================================================================== */

/** @verifies SWR-ICU-006 */
void test_Dashboard_battery_zone_red(void)
{
    mock_rte_signals[ICU_SIG_BATTERY_VOLTAGE] = 9500u;

    Swc_Dashboard_50ms();

    TEST_ASSERT_EQUAL_UINT8(DASH_ZONE_RED, dash_data.battery_zone);
}

/* ====================================================================
 * SWR-ICU-007: Warning — check engine when DTC active
 * ==================================================================== */

/** @verifies SWR-ICU-007 */
void test_Dashboard_warning_check_engine(void)
{
    /* DTC broadcast with a non-zero value indicates active DTC */
    mock_rte_signals[ICU_SIG_DTC_BROADCAST] = 0x00C001u;  /* Any non-zero DTC */

    Swc_Dashboard_50ms();

    TEST_ASSERT_TRUE((dash_data.warning_flags & DASH_WARN_CHECK_ENGINE) != 0u);
}

/* ====================================================================
 * SWR-ICU-007: Warning — e-stop active
 * ==================================================================== */

/** @verifies SWR-ICU-007 */
void test_Dashboard_warning_estop(void)
{
    mock_rte_signals[ICU_SIG_ESTOP_ACTIVE] = 1u;

    Swc_Dashboard_50ms();

    TEST_ASSERT_TRUE((dash_data.warning_flags & DASH_WARN_ESTOP) != 0u);
}

/* ====================================================================
 * SWR-ICU-007: Warning — overcurrent flag
 * ==================================================================== */

/** @verifies SWR-ICU-007 */
void test_Dashboard_warning_overcurrent(void)
{
    mock_rte_signals[ICU_SIG_OVERCURRENT_FLAG] = 1u;

    Swc_Dashboard_50ms();

    TEST_ASSERT_TRUE((dash_data.warning_flags & DASH_WARN_OVERCURRENT) != 0u);
}

/* ====================================================================
 * SWR-ICU-004: Vehicle state enum to string
 * ==================================================================== */

/** @verifies SWR-ICU-004 */
void test_Dashboard_vehicle_state_text(void)
{
    const char* result;

    result = Dashboard_GetVehicleStateStr(ICU_VSTATE_INIT);
    TEST_ASSERT_EQUAL_STRING("INIT", result);

    result = Dashboard_GetVehicleStateStr(ICU_VSTATE_RUN);
    TEST_ASSERT_EQUAL_STRING("RUN", result);

    result = Dashboard_GetVehicleStateStr(ICU_VSTATE_DEGRADED);
    TEST_ASSERT_EQUAL_STRING("DEGRADED", result);

    result = Dashboard_GetVehicleStateStr(ICU_VSTATE_LIMP);
    TEST_ASSERT_EQUAL_STRING("LIMP", result);

    result = Dashboard_GetVehicleStateStr(ICU_VSTATE_SAFE_STOP);
    TEST_ASSERT_EQUAL_STRING("SAFE_STOP", result);

    result = Dashboard_GetVehicleStateStr(ICU_VSTATE_SHUTDOWN);
    TEST_ASSERT_EQUAL_STRING("SHUTDOWN", result);

    /* Out of range: should return "UNKNOWN" */
    result = Dashboard_GetVehicleStateStr(99u);
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", result);
}

/* ====================================================================
 * SWR-ICU-009: ECU health timeout detection
 * ==================================================================== */

/** @verifies SWR-ICU-009 */
void test_Dashboard_ecu_health_timeout(void)
{
    uint8 i;

    /* Simulate heartbeat counters not incrementing.
     * After ICU_HB_TIMEOUT_TICKS + 1 cycles without increment,
     * the health status should flag timeout. */
    mock_rte_signals[ICU_SIG_HEARTBEAT_CVC] = 5u;
    mock_rte_signals[ICU_SIG_HEARTBEAT_FZC] = 5u;
    mock_rte_signals[ICU_SIG_HEARTBEAT_RZC] = 5u;

    /* Run enough cycles to detect timeout */
    for (i = 0u; i < (ICU_HB_TIMEOUT_TICKS + 2u); i++) {
        Swc_Dashboard_50ms();
    }

    /* CVC heartbeat should be flagged as timed out */
    TEST_ASSERT_TRUE(dash_data.ecu_health_cvc == FALSE);
    TEST_ASSERT_TRUE(dash_data.ecu_health_fzc == FALSE);
    TEST_ASSERT_TRUE(dash_data.ecu_health_rzc == FALSE);
}

/* ====================================================================
 * SWR-ICU-009: ECU health OK when heartbeat increments
 * ==================================================================== */

/** @verifies SWR-ICU-009 */
void test_Dashboard_ecu_health_ok(void)
{
    /* Simulate heartbeat incrementing each cycle */
    mock_rte_signals[ICU_SIG_HEARTBEAT_CVC] = 1u;
    mock_rte_signals[ICU_SIG_HEARTBEAT_FZC] = 1u;
    mock_rte_signals[ICU_SIG_HEARTBEAT_RZC] = 1u;
    Swc_Dashboard_50ms();

    mock_rte_signals[ICU_SIG_HEARTBEAT_CVC] = 2u;
    mock_rte_signals[ICU_SIG_HEARTBEAT_FZC] = 2u;
    mock_rte_signals[ICU_SIG_HEARTBEAT_RZC] = 2u;
    Swc_Dashboard_50ms();

    TEST_ASSERT_TRUE(dash_data.ecu_health_cvc == TRUE);
    TEST_ASSERT_TRUE(dash_data.ecu_health_fzc == TRUE);
    TEST_ASSERT_TRUE(dash_data.ecu_health_rzc == TRUE);
}

/* ====================================================================
 * Test runner
 * ==================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* SWR-ICU-002: Initialization */
    RUN_TEST(test_Dashboard_init_all_zero);

    /* SWR-ICU-003: Speed from RPM */
    RUN_TEST(test_Dashboard_speed_from_rpm);
    RUN_TEST(test_Dashboard_speed_zero_rpm);

    /* SWR-ICU-004: Torque and vehicle state */
    RUN_TEST(test_Dashboard_torque_percentage);
    RUN_TEST(test_Dashboard_vehicle_state_text);

    /* SWR-ICU-005: Temperature zones */
    RUN_TEST(test_Dashboard_temp_zone_green);
    RUN_TEST(test_Dashboard_temp_zone_yellow);
    RUN_TEST(test_Dashboard_temp_zone_orange);
    RUN_TEST(test_Dashboard_temp_zone_red);

    /* SWR-ICU-006: Battery zones */
    RUN_TEST(test_Dashboard_battery_zone_green);
    RUN_TEST(test_Dashboard_battery_zone_yellow);
    RUN_TEST(test_Dashboard_battery_zone_red);

    /* SWR-ICU-007: Warning flags */
    RUN_TEST(test_Dashboard_warning_check_engine);
    RUN_TEST(test_Dashboard_warning_estop);
    RUN_TEST(test_Dashboard_warning_overcurrent);

    /* SWR-ICU-009: ECU health monitoring */
    RUN_TEST(test_Dashboard_ecu_health_timeout);
    RUN_TEST(test_Dashboard_ecu_health_ok);

    return UNITY_END();
}
