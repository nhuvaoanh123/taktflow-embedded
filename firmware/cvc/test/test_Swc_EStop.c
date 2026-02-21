/**
 * @file    test_Swc_EStop.c
 * @brief   Unit tests for Emergency Stop detection SWC
 * @date    2026-02-21
 *
 * @verifies SWR-CVC-018, SWR-CVC-019, SWR-CVC-020
 *
 * Tests E-stop detection, debounce, latch behaviour, CAN broadcast,
 * DTC reporting, and fail-safe on read failure.
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
#define STD_HIGH  0x01u
#define STD_LOW   0x00u

/* Prevent BSW headers from redefining types */
#define PLATFORM_TYPES_H
#define STD_TYPES_H
#define SWC_ESTOP_H
#define CVC_CFG_H

/* Signal/DTC IDs (must match Cvc_Cfg.h) */
#define CVC_SIG_ESTOP_ACTIVE      22u
#define CVC_DTC_ESTOP_ACTIVATED   12u
#define CVC_COM_TX_ESTOP           0u

/* ====================================================================
 * Mock: IoHwAb_ReadEStop
 * ==================================================================== */

static uint8          mock_estop_state;
static Std_ReturnType mock_estop_read_result;

Std_ReturnType IoHwAb_ReadEStop(uint8* State)
{
    if (State != NULL_PTR) {
        *State = mock_estop_state;
    }
    return mock_estop_read_result;
}

/* ====================================================================
 * Mock: Rte_Write
 * ==================================================================== */

static uint16 mock_rte_write_sig;
static uint32 mock_rte_write_val;
static uint8  mock_rte_write_count;

Std_ReturnType Rte_Write(uint16 SignalId, uint32 Data)
{
    mock_rte_write_sig = SignalId;
    mock_rte_write_val = Data;
    mock_rte_write_count++;
    return E_OK;
}

/* ====================================================================
 * Mock: Com_SendSignal
 * ==================================================================== */

static uint8 mock_com_send_count;
static uint8 mock_com_send_sig_id;

Std_ReturnType Com_SendSignal(uint8 SignalId, const void* SignalDataPtr)
{
    mock_com_send_sig_id = SignalId;
    mock_com_send_count++;
    (void)SignalDataPtr;
    return E_OK;
}

/* ====================================================================
 * Mock: E2E_Protect
 * ==================================================================== */

static uint8 mock_e2e_protect_count;

Std_ReturnType E2E_Protect(const void* Config, void* State,
                           uint8* DataPtr, uint16 Length)
{
    mock_e2e_protect_count++;
    (void)Config;
    (void)State;
    (void)DataPtr;
    (void)Length;
    return E_OK;
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

#include "../src/Swc_EStop.c"

/* ====================================================================
 * setUp / tearDown
 * ==================================================================== */

void setUp(void)
{
    mock_estop_state       = STD_LOW;
    mock_estop_read_result = E_OK;
    mock_rte_write_sig     = 0u;
    mock_rte_write_val     = 0u;
    mock_rte_write_count   = 0u;
    mock_com_send_count    = 0u;
    mock_com_send_sig_id   = 0xFFu;
    mock_e2e_protect_count = 0u;
    mock_dem_event_id      = 0xFFu;
    mock_dem_event_status  = 0xFFu;
    mock_dem_report_count  = 0u;

    Swc_EStop_Init();
}

void tearDown(void) { }

/* ====================================================================
 * SWR-CVC-018: E-stop detection, debounce, fail-safe
 * ==================================================================== */

/** @verifies SWR-CVC-018 */
void test_EStop_Init_inactive(void)
{
    TEST_ASSERT_EQUAL(FALSE, Swc_EStop_IsActive());
}

/** @verifies SWR-CVC-018 */
void test_EStop_Detection_after_debounce(void)
{
    mock_estop_state = STD_HIGH;

    /* One cycle = debounce threshold (1 at 10ms) */
    Swc_EStop_MainFunction();

    TEST_ASSERT_EQUAL(TRUE, Swc_EStop_IsActive());
}

/** @verifies SWR-CVC-018 */
void test_EStop_Debounce_transient_no_activation(void)
{
    /* HIGH for 0 full debounce cycles — released before threshold */
    mock_estop_state = STD_HIGH;
    /* Do NOT call MainFunction — button pressed but not yet sampled */
    /* Now release before first sample */
    mock_estop_state = STD_LOW;
    Swc_EStop_MainFunction();

    TEST_ASSERT_EQUAL(FALSE, Swc_EStop_IsActive());
}

/** @verifies SWR-CVC-018 */
void test_EStop_RTE_write_on_activation(void)
{
    mock_estop_state = STD_HIGH;
    Swc_EStop_MainFunction();

    TEST_ASSERT_EQUAL(CVC_SIG_ESTOP_ACTIVE, mock_rte_write_sig);
    TEST_ASSERT_EQUAL(TRUE, mock_rte_write_val);
}

/** @verifies SWR-CVC-018 */
void test_EStop_No_false_activation_when_low(void)
{
    mock_estop_state = STD_LOW;

    Swc_EStop_MainFunction();
    Swc_EStop_MainFunction();
    Swc_EStop_MainFunction();

    TEST_ASSERT_EQUAL(FALSE, Swc_EStop_IsActive());
    TEST_ASSERT_EQUAL(0u, mock_com_send_count);
}

/** @verifies SWR-CVC-018 */
void test_EStop_ReadFailure_failsafe_active(void)
{
    mock_estop_read_result = E_NOT_OK;

    Swc_EStop_MainFunction();

    TEST_ASSERT_EQUAL(TRUE, Swc_EStop_IsActive());
}

/* ====================================================================
 * SWR-CVC-019: E-stop CAN broadcast
 * ==================================================================== */

/** @verifies SWR-CVC-019 */
void test_EStop_Broadcast_on_activation(void)
{
    mock_estop_state = STD_HIGH;
    Swc_EStop_MainFunction();

    TEST_ASSERT_TRUE(mock_com_send_count >= 1u);
    TEST_ASSERT_EQUAL(CVC_COM_TX_ESTOP, mock_com_send_sig_id);
}

/** @verifies SWR-CVC-019 */
void test_EStop_Repeat_broadcasts_total_4(void)
{
    mock_estop_state = STD_HIGH;

    /* Cycle 1: activation + first broadcast */
    Swc_EStop_MainFunction();
    /* Cycles 2-4: repeat broadcasts */
    Swc_EStop_MainFunction();
    Swc_EStop_MainFunction();
    Swc_EStop_MainFunction();

    TEST_ASSERT_EQUAL(4u, mock_com_send_count);

    /* Cycle 5: no more broadcasts */
    mock_com_send_count = 0u;
    Swc_EStop_MainFunction();
    TEST_ASSERT_EQUAL(0u, mock_com_send_count);
}

/* ====================================================================
 * SWR-CVC-020: E-stop latch and DTC
 * ==================================================================== */

/** @verifies SWR-CVC-020 */
void test_EStop_Latch_stays_active_after_release(void)
{
    mock_estop_state = STD_HIGH;
    Swc_EStop_MainFunction();

    /* Release button */
    mock_estop_state = STD_LOW;
    Swc_EStop_MainFunction();
    Swc_EStop_MainFunction();
    Swc_EStop_MainFunction();

    TEST_ASSERT_EQUAL(TRUE, Swc_EStop_IsActive());
}

/** @verifies SWR-CVC-020 */
void test_EStop_DTC_reported_on_activation(void)
{
    mock_estop_state = STD_HIGH;
    Swc_EStop_MainFunction();

    TEST_ASSERT_EQUAL(CVC_DTC_ESTOP_ACTIVATED, mock_dem_event_id);
    TEST_ASSERT_EQUAL(DEM_EVENT_STATUS_FAILED, mock_dem_event_status);
}

/* ====================================================================
 * Test runner
 * ==================================================================== */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_EStop_Init_inactive);
    RUN_TEST(test_EStop_Detection_after_debounce);
    RUN_TEST(test_EStop_Debounce_transient_no_activation);
    RUN_TEST(test_EStop_Broadcast_on_activation);
    RUN_TEST(test_EStop_Repeat_broadcasts_total_4);
    RUN_TEST(test_EStop_Latch_stays_active_after_release);
    RUN_TEST(test_EStop_DTC_reported_on_activation);
    RUN_TEST(test_EStop_RTE_write_on_activation);
    RUN_TEST(test_EStop_No_false_activation_when_low);
    RUN_TEST(test_EStop_ReadFailure_failsafe_active);

    return UNITY_END();
}
