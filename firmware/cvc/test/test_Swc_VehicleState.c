/**
 * @file    test_Swc_VehicleState.c
 * @brief   Unit tests for Vehicle State Machine SWC
 * @date    2026-02-21
 *
 * @verifies SWR-CVC-009, SWR-CVC-010, SWR-CVC-011, SWR-CVC-012, SWR-CVC-013
 *
 * Tests state machine transitions (INIT -> RUN -> DEGRADED -> LIMP ->
 * SAFE_STOP -> SHUTDOWN), fault signal monitoring, BswM mode requests,
 * DTC reporting for motor/brake/steering faults, and defensive checks.
 */
#include "unity.h"

/* ==================================================================
 * Local type definitions (mirror AUTOSAR / Platform_Types)
 * ================================================================== */

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned long  uint32;
typedef uint8 Std_ReturnType;

#define E_OK        0u
#define E_NOT_OK    1u
#define TRUE        1u
#define FALSE       0u
#define NULL_PTR    ((void*)0)

/* ==================================================================
 * Signal IDs (from Cvc_Cfg.h)
 * ================================================================== */

#define CVC_SIG_PEDAL_FAULT       19u
#define CVC_SIG_VEHICLE_STATE     20u
#define CVC_SIG_TORQUE_REQUEST    21u
#define CVC_SIG_ESTOP_ACTIVE      22u
#define CVC_SIG_FZC_COMM_STATUS   23u
#define CVC_SIG_RZC_COMM_STATUS   24u
#define CVC_SIG_MOTOR_CUTOFF      28u
#define CVC_SIG_STEERING_FAULT    29u
#define CVC_SIG_BRAKE_FAULT       30u

/* ==================================================================
 * Vehicle State / Event / Comm definitions (from Cvc_Cfg.h)
 * ================================================================== */

#define CVC_STATE_INIT          0u
#define CVC_STATE_RUN           1u
#define CVC_STATE_DEGRADED      2u
#define CVC_STATE_LIMP          3u
#define CVC_STATE_SAFE_STOP     4u
#define CVC_STATE_SHUTDOWN      5u
#define CVC_STATE_COUNT         6u
#define CVC_STATE_INVALID       0xFFu

#define CVC_EVT_SELF_TEST_PASS      0u
#define CVC_EVT_SELF_TEST_FAIL      1u
#define CVC_EVT_PEDAL_FAULT_SINGLE  2u
#define CVC_EVT_PEDAL_FAULT_DUAL    3u
#define CVC_EVT_CAN_TIMEOUT_SINGLE  4u
#define CVC_EVT_CAN_TIMEOUT_DUAL    5u
#define CVC_EVT_ESTOP               6u
#define CVC_EVT_SC_KILL             7u
#define CVC_EVT_FAULT_CLEARED       8u
#define CVC_EVT_CAN_RESTORED        9u
#define CVC_EVT_VEHICLE_STOPPED    10u
#define CVC_EVT_COUNT              11u

#define CVC_COMM_OK                 0u
#define CVC_COMM_TIMEOUT            1u

/* Pedal fault values */
#define CVC_PEDAL_NO_FAULT          0u
#define CVC_PEDAL_PLAUSIBILITY      1u
#define CVC_PEDAL_STUCK             2u

/* DTC Event IDs */
#define CVC_DTC_MOTOR_CUTOFF_RX     9u
#define CVC_DTC_BRAKE_FAULT_RX     10u
#define CVC_DTC_STEERING_FAULT_RX  11u

/* DEM event status */
#define DEM_EVENT_STATUS_FAILED     1u

/* BswM mode values */
#define BSWM_STARTUP    0u
#define BSWM_RUN        1u
#define BSWM_DEGRADED   2u
#define BSWM_SAFE_STOP  3u
#define BSWM_SHUTDOWN   4u

/* ==================================================================
 * Mock: RTE signal store
 * ================================================================== */

static uint32 mock_rte_signals[32];
static Std_ReturnType mock_rte_read_return;
static Std_ReturnType mock_rte_write_return;

/* Track what was written */
static uint16 mock_rte_write_last_id;
static uint32 mock_rte_write_last_value;
static uint8  mock_rte_write_call_count;

Std_ReturnType Rte_Read(uint16 SignalId, uint32* DataPtr)
{
    if ((DataPtr == NULL_PTR) || (SignalId >= 32u))
    {
        return E_NOT_OK;
    }
    *DataPtr = mock_rte_signals[SignalId];
    return mock_rte_read_return;
}

Std_ReturnType Rte_Write(uint16 SignalId, uint32 Data)
{
    if (SignalId >= 32u)
    {
        return E_NOT_OK;
    }
    mock_rte_write_last_id    = SignalId;
    mock_rte_write_last_value = Data;
    mock_rte_write_call_count++;
    return mock_rte_write_return;
}

/* ==================================================================
 * Mock: BswM
 * ================================================================== */

static uint8  mock_bswm_requester_id;
static uint8  mock_bswm_requested_mode;
static uint8  mock_bswm_call_count;
static Std_ReturnType mock_bswm_return;

Std_ReturnType BswM_RequestMode(uint8 RequesterId, uint8 RequestedMode)
{
    mock_bswm_requester_id   = RequesterId;
    mock_bswm_requested_mode = RequestedMode;
    mock_bswm_call_count++;
    return mock_bswm_return;
}

/* ==================================================================
 * Mock: Dem
 * ================================================================== */

static uint8 mock_dem_event_id;
static uint8 mock_dem_event_status;
static uint8 mock_dem_call_count;

void Dem_ReportErrorStatus(uint8 EventId, uint8 EventStatus)
{
    mock_dem_event_id     = EventId;
    mock_dem_event_status = EventStatus;
    mock_dem_call_count++;
}

/* ==================================================================
 * SWC Under Test — declarations
 * ================================================================== */

extern void  Swc_VehicleState_Init(void);
extern void  Swc_VehicleState_MainFunction(void);
extern uint8 Swc_VehicleState_GetState(void);
extern void  Swc_VehicleState_OnEvent(uint8 event);

/* ==================================================================
 * setUp / tearDown
 * ================================================================== */

void setUp(void)
{
    uint8 i;

    /* Clear RTE signal store */
    for (i = 0u; i < 32u; i++)
    {
        mock_rte_signals[i] = 0u;
    }
    mock_rte_read_return    = E_OK;
    mock_rte_write_return   = E_OK;
    mock_rte_write_last_id  = 0u;
    mock_rte_write_last_value = 0u;
    mock_rte_write_call_count = 0u;

    /* Clear BswM mock */
    mock_bswm_requester_id   = 0xFFu;
    mock_bswm_requested_mode = 0xFFu;
    mock_bswm_call_count     = 0u;
    mock_bswm_return         = E_OK;

    /* Clear Dem mock */
    mock_dem_event_id     = 0xFFu;
    mock_dem_event_status = 0xFFu;
    mock_dem_call_count   = 0u;

    /* Initialize the SWC */
    Swc_VehicleState_Init();
}

void tearDown(void) { }

/* ==================================================================
 * SWR-CVC-009: Initialization and INIT State Transitions
 * ================================================================== */

/** @verifies SWR-CVC-009 — Init starts in INIT state */
void test_Init_starts_in_INIT_state(void)
{
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_INIT, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-009 — INIT -> RUN on self-test pass + both heartbeats OK */
void test_INIT_to_RUN_on_self_test_pass_heartbeats_ok(void)
{
    /* Set both comm statuses to OK */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;

    /* Fire self-test pass event */
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);

    /* Must have read heartbeats first via MainFunction to validate them.
     * OnEvent checks the table, but the heartbeat guard is in MainFunction.
     * Run MainFunction to apply the guarded transition. */
    Swc_VehicleState_MainFunction();

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-009 — INIT -> SAFE_STOP on self-test fail */
void test_INIT_to_SAFE_STOP_on_self_test_fail(void)
{
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_FAIL);

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());
}

/* ==================================================================
 * SWR-CVC-010: RUN State Transitions
 * ================================================================== */

/** @verifies SWR-CVC-010 — RUN -> DEGRADED on single pedal fault */
void test_RUN_to_DEGRADED_on_single_pedal_fault(void)
{
    /* Get to RUN state first */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());

    /* Now inject single pedal fault */
    Swc_VehicleState_OnEvent(CVC_EVT_PEDAL_FAULT_SINGLE);

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_DEGRADED, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-010 — RUN -> LIMP on single CAN timeout */
void test_RUN_to_LIMP_on_single_CAN_timeout(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();

    /* Single CAN timeout */
    Swc_VehicleState_OnEvent(CVC_EVT_CAN_TIMEOUT_SINGLE);

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_LIMP, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-010 — RUN -> SAFE_STOP on dual pedal fault */
void test_RUN_to_SAFE_STOP_on_dual_pedal_fault(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();

    /* Dual pedal fault */
    Swc_VehicleState_OnEvent(CVC_EVT_PEDAL_FAULT_DUAL);

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-010 — RUN -> SAFE_STOP on E-stop */
void test_RUN_to_SAFE_STOP_on_estop(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();

    /* E-stop */
    Swc_VehicleState_OnEvent(CVC_EVT_ESTOP);

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());
}

/* ==================================================================
 * SWR-CVC-011: DEGRADED State Transitions
 * ================================================================== */

/** @verifies SWR-CVC-011 — DEGRADED -> RUN on fault cleared */
void test_DEGRADED_to_RUN_on_fault_cleared(void)
{
    /* Get to RUN, then DEGRADED */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_OnEvent(CVC_EVT_PEDAL_FAULT_SINGLE);
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_DEGRADED, Swc_VehicleState_GetState());

    /* Fault cleared */
    Swc_VehicleState_OnEvent(CVC_EVT_FAULT_CLEARED);

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-011 — DEGRADED -> SAFE_STOP on E-stop */
void test_DEGRADED_to_SAFE_STOP_on_estop(void)
{
    /* Get to DEGRADED */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_OnEvent(CVC_EVT_PEDAL_FAULT_SINGLE);

    /* E-stop */
    Swc_VehicleState_OnEvent(CVC_EVT_ESTOP);

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-011 — DEGRADED -> LIMP on CAN timeout */
void test_DEGRADED_to_LIMP_on_CAN_timeout(void)
{
    /* Get to DEGRADED */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_OnEvent(CVC_EVT_PEDAL_FAULT_SINGLE);

    /* Single CAN timeout */
    Swc_VehicleState_OnEvent(CVC_EVT_CAN_TIMEOUT_SINGLE);

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_LIMP, Swc_VehicleState_GetState());
}

/* ==================================================================
 * SWR-CVC-012: LIMP State Transitions
 * ================================================================== */

/** @verifies SWR-CVC-012 — LIMP -> SAFE_STOP on E-stop */
void test_LIMP_to_SAFE_STOP_on_estop(void)
{
    /* Get to LIMP: RUN -> CAN_TIMEOUT_SINGLE */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_OnEvent(CVC_EVT_CAN_TIMEOUT_SINGLE);
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_LIMP, Swc_VehicleState_GetState());

    /* E-stop */
    Swc_VehicleState_OnEvent(CVC_EVT_ESTOP);

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-012 — LIMP -> DEGRADED on CAN restored (no pedal fault) */
void test_LIMP_to_DEGRADED_on_CAN_restored(void)
{
    /* Get to LIMP */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_OnEvent(CVC_EVT_CAN_TIMEOUT_SINGLE);
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_LIMP, Swc_VehicleState_GetState());

    /* CAN restored -> goes to DEGRADED (not directly RUN) */
    Swc_VehicleState_OnEvent(CVC_EVT_CAN_RESTORED);

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_DEGRADED, Swc_VehicleState_GetState());
}

/* ==================================================================
 * SWR-CVC-013: SAFE_STOP and Universal Transitions
 * ================================================================== */

/** @verifies SWR-CVC-013 — SAFE_STOP -> SHUTDOWN on vehicle stopped */
void test_SAFE_STOP_to_SHUTDOWN_on_vehicle_stopped(void)
{
    /* Get to SAFE_STOP */
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_FAIL);
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());

    /* Vehicle stopped */
    Swc_VehicleState_OnEvent(CVC_EVT_VEHICLE_STOPPED);

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SHUTDOWN, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-013 — Any -> SAFE_STOP on SC kill signal */
void test_Any_to_SAFE_STOP_on_SC_kill(void)
{
    /* From RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());

    /* SC kill */
    Swc_VehicleState_OnEvent(CVC_EVT_SC_KILL);

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());
}

/* ==================================================================
 * SWR-CVC-009: Invalid Transition Rejected
 * ================================================================== */

/** @verifies SWR-CVC-009 — Invalid transition rejected (RUN -> INIT) */
void test_Invalid_transition_rejected(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());

    /* Try to inject VEHICLE_STOPPED — not valid from RUN */
    mock_bswm_call_count = 0u;
    Swc_VehicleState_OnEvent(CVC_EVT_VEHICLE_STOPPED);

    /* State should remain RUN */
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());
    /* BswM should NOT have been called */
    TEST_ASSERT_EQUAL_UINT8(0u, mock_bswm_call_count);
}

/* ==================================================================
 * SWR-CVC-010: BswM Called on Valid Transitions
 * ================================================================== */

/** @verifies SWR-CVC-010 — BswM_RequestMode called on each valid transition */
void test_BswM_called_on_valid_transition(void)
{
    /* INIT -> SAFE_STOP — a valid transition */
    mock_bswm_call_count = 0u;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_FAIL);

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());
    TEST_ASSERT_TRUE(mock_bswm_call_count > 0u);
    TEST_ASSERT_EQUAL_UINT8(BSWM_SAFE_STOP, mock_bswm_requested_mode);
}

/* ==================================================================
 * SWR-CVC-009: State Written to RTE Each Cycle
 * ================================================================== */

/** @verifies SWR-CVC-009 — State written to RTE each cycle */
void test_State_written_to_RTE_each_cycle(void)
{
    mock_rte_write_call_count = 0u;

    Swc_VehicleState_MainFunction();

    /* State should be written via Rte_Write */
    TEST_ASSERT_TRUE(mock_rte_write_call_count > 0u);
    TEST_ASSERT_EQUAL_UINT16(CVC_SIG_VEHICLE_STATE, mock_rte_write_last_id);
    TEST_ASSERT_EQUAL_UINT32((uint32)CVC_STATE_INIT, mock_rte_write_last_value);
}

/* ==================================================================
 * SWR-CVC-010: DTC Reporting for Fault Signals
 * ================================================================== */

/** @verifies SWR-CVC-010 — Motor cutoff received -> report DTC */
void test_Motor_cutoff_reports_DTC(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();

    /* Set motor cutoff signal active */
    mock_rte_signals[CVC_SIG_MOTOR_CUTOFF] = 1u;
    mock_dem_call_count = 0u;

    Swc_VehicleState_MainFunction();

    /* Dem_ReportErrorStatus should have been called for motor cutoff */
    TEST_ASSERT_TRUE(mock_dem_call_count > 0u);
}

/** @verifies SWR-CVC-010 — Brake fault received -> report DTC */
void test_Brake_fault_reports_DTC(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();

    /* Set brake fault signal active */
    mock_rte_signals[CVC_SIG_BRAKE_FAULT] = 1u;
    mock_dem_call_count = 0u;

    Swc_VehicleState_MainFunction();

    /* Dem_ReportErrorStatus should have been called for brake fault */
    TEST_ASSERT_TRUE(mock_dem_call_count > 0u);
}

/** @verifies SWR-CVC-010 — Steering fault received -> report DTC */
void test_Steering_fault_reports_DTC(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();

    /* Set steering fault signal active */
    mock_rte_signals[CVC_SIG_STEERING_FAULT] = 1u;
    mock_dem_call_count = 0u;

    Swc_VehicleState_MainFunction();

    /* Dem_ReportErrorStatus should have been called for steering fault */
    TEST_ASSERT_TRUE(mock_dem_call_count > 0u);
}

/* ==================================================================
 * Test runner
 * ================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* SWR-CVC-009: Init and INIT state */
    RUN_TEST(test_Init_starts_in_INIT_state);
    RUN_TEST(test_INIT_to_RUN_on_self_test_pass_heartbeats_ok);
    RUN_TEST(test_INIT_to_SAFE_STOP_on_self_test_fail);

    /* SWR-CVC-010: RUN transitions */
    RUN_TEST(test_RUN_to_DEGRADED_on_single_pedal_fault);
    RUN_TEST(test_RUN_to_LIMP_on_single_CAN_timeout);
    RUN_TEST(test_RUN_to_SAFE_STOP_on_dual_pedal_fault);
    RUN_TEST(test_RUN_to_SAFE_STOP_on_estop);

    /* SWR-CVC-011: DEGRADED transitions */
    RUN_TEST(test_DEGRADED_to_RUN_on_fault_cleared);
    RUN_TEST(test_DEGRADED_to_SAFE_STOP_on_estop);
    RUN_TEST(test_DEGRADED_to_LIMP_on_CAN_timeout);

    /* SWR-CVC-012: LIMP transitions */
    RUN_TEST(test_LIMP_to_SAFE_STOP_on_estop);
    RUN_TEST(test_LIMP_to_DEGRADED_on_CAN_restored);

    /* SWR-CVC-013: SAFE_STOP and universal */
    RUN_TEST(test_SAFE_STOP_to_SHUTDOWN_on_vehicle_stopped);
    RUN_TEST(test_Any_to_SAFE_STOP_on_SC_kill);

    /* SWR-CVC-009: Defensive checks */
    RUN_TEST(test_Invalid_transition_rejected);

    /* SWR-CVC-010: BswM integration */
    RUN_TEST(test_BswM_called_on_valid_transition);

    /* SWR-CVC-009: RTE output */
    RUN_TEST(test_State_written_to_RTE_each_cycle);

    /* SWR-CVC-010: DTC reporting */
    RUN_TEST(test_Motor_cutoff_reports_DTC);
    RUN_TEST(test_Brake_fault_reports_DTC);
    RUN_TEST(test_Steering_fault_reports_DTC);

    return UNITY_END();
}
