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
typedef unsigned int  uint32;
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
#define CVC_EVT_MOTOR_CUTOFF       11u
#define CVC_EVT_BRAKE_FAULT        12u
#define CVC_EVT_STEERING_FAULT     13u
#define CVC_EVT_COUNT              14u

#define CVC_COMM_OK                 0u
#define CVC_COMM_TIMEOUT            1u

/* ECU IDs (from Cvc_Cfg.h) */
#define CVC_ECU_ID_CVC              0x01u

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
 * Mock: Com_SendSignal (Swc_VehicleState publishes state to Com)
 * ================================================================== */

static uint16  mock_com_last_signal_id;
static uint8   mock_com_send_count;

Std_ReturnType Com_SendSignal(uint16 SignalId, const void* SignalDataPtr)
{
    mock_com_last_signal_id = SignalId;
    mock_com_send_count++;
    (void)SignalDataPtr;
    return E_OK;
}

/* ==================================================================
 * Mock: Com_ReceiveSignal (confirmation read from Com shadow buffer)
 * ================================================================== */

typedef uint8 Com_SignalIdType;

static uint8 mock_com_rx_values[32];

Std_ReturnType Com_ReceiveSignal(Com_SignalIdType SignalId, void* SignalDataPtr)
{
    if (SignalId < 32u)
    {
        *((uint8*)SignalDataPtr) = mock_com_rx_values[SignalId];
    }
    return E_OK;
}

/* ==================================================================
 * Mock: Swc_CvcCom_GetRxStatus (E2E status check)
 * ================================================================== */

typedef struct {
    uint8   failCount;
    uint8   useSafeDefault;
} Swc_CvcCom_RxStatusType;

static Swc_CvcCom_RxStatusType mock_rx_status[4];

Std_ReturnType Swc_CvcCom_GetRxStatus(uint8 rxIndex,
                                       Swc_CvcCom_RxStatusType* status)
{
    if (rxIndex < 4u)
    {
        *status = mock_rx_status[rxIndex];
    }
    return E_OK;
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

    /* Clear Com send mock */
    mock_com_last_signal_id = 0u;
    mock_com_send_count     = 0u;

    /* Clear Com receive mock (confirmation reads) */
    for (i = 0u; i < 32u; i++)
    {
        mock_com_rx_values[i] = 0u;
    }

    /* Clear CvcCom RX status mock (E2E) */
    {
        uint8 j;
        for (j = 0u; j < 4u; j++)
        {
            mock_rx_status[j].failCount     = 0u;
            mock_rx_status[j].useSafeDefault = FALSE;
        }
    }

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

/** @verifies SWR-CVC-010 — Motor cutoff confirmed -> report DTC */
void test_Motor_cutoff_reports_DTC(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();

    /* Set motor cutoff signal active + Com confirms */
    mock_rte_signals[CVC_SIG_MOTOR_CUTOFF] = 1u;
    mock_com_rx_values[14u] = 1u;
    mock_dem_call_count = 0u;

    /* DTC reported after 3-cycle confirmation */
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();

    TEST_ASSERT_TRUE(mock_dem_call_count > 0u);
}

/** @verifies SWR-CVC-010 — Brake fault confirmed -> report DTC */
void test_Brake_fault_reports_DTC(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();

    /* Set brake fault signal active + Com confirms + E2E OK */
    mock_rte_signals[CVC_SIG_BRAKE_FAULT] = 1u;
    mock_com_rx_values[13u] = 1u;
    mock_dem_call_count = 0u;

    /* DTC reported after 3-cycle confirmation */
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();

    TEST_ASSERT_TRUE(mock_dem_call_count > 0u);
}

/** @verifies SWR-CVC-010 — Steering fault confirmed -> report DTC */
void test_Steering_fault_reports_DTC(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();

    /* Set steering fault signal active (debounce only, no Com) */
    mock_rte_signals[CVC_SIG_STEERING_FAULT] = 1u;
    mock_dem_call_count = 0u;

    /* DTC reported after 3-cycle debounce */
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();

    TEST_ASSERT_TRUE(mock_dem_call_count > 0u);
}

/* ==================================================================
 * HARDENED TESTS — Boundary Value, Fault Injection, Defensive Checks
 * ================================================================== */

/* ------------------------------------------------------------------
 * SWR-CVC-009: Event boundary — out-of-range event IDs
 * ------------------------------------------------------------------ */

/** @verifies SWR-CVC-009
 *  Equivalence class: INVALID — event ID at boundary (CVC_EVT_COUNT)
 *  Boundary: event = CVC_EVT_COUNT (first invalid) */
void test_OnEvent_boundary_event_at_count(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());

    /* Fire event with ID = CVC_EVT_COUNT (14) — out of range */
    Swc_VehicleState_OnEvent(CVC_EVT_COUNT);

    /* State should remain RUN */
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-009
 *  Equivalence class: INVALID — event ID = 0xFF (max uint8)
 *  Boundary: maximum possible uint8 value */
void test_OnEvent_boundary_event_max_uint8(void)
{
    Swc_VehicleState_OnEvent(0xFFu);

    /* State should remain INIT (no transition) */
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_INIT, Swc_VehicleState_GetState());
}

/* ------------------------------------------------------------------
 * SWR-CVC-009: E-stop override from every state
 * ------------------------------------------------------------------ */

/** @verifies SWR-CVC-009
 *  Equivalence class: VALID — E-stop from INIT state (via signal in MainFunction)
 *  Fault injection: E-stop while still in INIT */
void test_Estop_from_INIT_via_signal(void)
{
    /* Set E-stop active signal, run MainFunction */
    mock_rte_signals[CVC_SIG_ESTOP_ACTIVE] = 1u;
    Swc_VehicleState_MainFunction();

    /* INIT does not have EVT_ESTOP as valid transition in the table,
     * so the state should remain INIT (per transition table design) */
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_INIT, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-009
 *  Equivalence class: VALID — E-stop from LIMP (direct OnEvent call)
 *  Fault injection: critical E-stop from degraded operational mode */
void test_Estop_from_LIMP_via_OnEvent(void)
{
    /* Get to LIMP */
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

/* ------------------------------------------------------------------
 * SWR-CVC-009: SC_KILL from every relevant state
 * ------------------------------------------------------------------ */

/** @verifies SWR-CVC-009
 *  Equivalence class: VALID — SC_KILL from DEGRADED
 *  Fault injection: safety controller kill from DEGRADED */
void test_SC_KILL_from_DEGRADED(void)
{
    /* Get to DEGRADED */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_OnEvent(CVC_EVT_PEDAL_FAULT_SINGLE);
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_DEGRADED, Swc_VehicleState_GetState());

    Swc_VehicleState_OnEvent(CVC_EVT_SC_KILL);
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-009
 *  Equivalence class: VALID — SC_KILL from LIMP
 *  Fault injection: safety controller kill from LIMP mode */
void test_SC_KILL_from_LIMP(void)
{
    /* Get to LIMP */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_OnEvent(CVC_EVT_CAN_TIMEOUT_SINGLE);
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_LIMP, Swc_VehicleState_GetState());

    Swc_VehicleState_OnEvent(CVC_EVT_SC_KILL);
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());
}

/* ------------------------------------------------------------------
 * SWR-CVC-009: SHUTDOWN is a terminal state — all events rejected
 * ------------------------------------------------------------------ */

/** @verifies SWR-CVC-009
 *  Equivalence class: INVALID — no transitions from SHUTDOWN
 *  Boundary: terminal state, all events should be rejected */
void test_SHUTDOWN_rejects_all_events(void)
{
    uint8 evt;

    /* Get to SHUTDOWN: INIT -> SAFE_STOP -> SHUTDOWN */
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_FAIL);
    Swc_VehicleState_OnEvent(CVC_EVT_VEHICLE_STOPPED);
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SHUTDOWN, Swc_VehicleState_GetState());

    /* Try every event — state should remain SHUTDOWN */
    for (evt = 0u; evt < CVC_EVT_COUNT; evt++) {
        Swc_VehicleState_OnEvent(evt);
        TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SHUTDOWN, Swc_VehicleState_GetState());
    }
}

/* ------------------------------------------------------------------
 * SWR-CVC-009: INIT -> RUN blocked when heartbeats not OK
 * ------------------------------------------------------------------ */

/** @verifies SWR-CVC-009
 *  Equivalence class: INVALID — self-test pass but FZC heartbeat missing
 *  Fault injection: FZC timeout prevents transition to RUN */
void test_INIT_to_RUN_blocked_when_fzc_timeout(void)
{
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_TIMEOUT;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;

    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();

    /* Should remain in INIT — FZC heartbeat not OK */
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_INIT, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-009
 *  Equivalence class: INVALID — self-test pass but RZC heartbeat missing
 *  Fault injection: RZC timeout prevents transition to RUN */
void test_INIT_to_RUN_blocked_when_rzc_timeout(void)
{
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_TIMEOUT;

    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();

    /* Should remain in INIT — RZC heartbeat not OK */
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_INIT, Swc_VehicleState_GetState());
}

/* ------------------------------------------------------------------
 * SWR-CVC-010: RTE read failure — MainFunction defensive behavior
 * ------------------------------------------------------------------ */

/** @verifies SWR-CVC-010
 *  Equivalence class: FAULT — RTE read returns failure
 *  Fault injection: Rte_Read returns E_NOT_OK */
void test_MainFunction_rte_read_failure_no_crash(void)
{
    mock_rte_read_return = E_NOT_OK;

    /* Should not crash, state remains INIT */
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_INIT, Swc_VehicleState_GetState());
}

/* ------------------------------------------------------------------
 * SWR-CVC-010: Dual CAN timeout from RUN goes to SAFE_STOP
 * ------------------------------------------------------------------ */

/** @verifies SWR-CVC-010
 *  Equivalence class: VALID — dual CAN timeout from RUN = SAFE_STOP
 *  Boundary: both comm channels fail simultaneously */
void test_RUN_to_SAFE_STOP_on_dual_CAN_timeout(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());

    /* Dual CAN timeout */
    Swc_VehicleState_OnEvent(CVC_EVT_CAN_TIMEOUT_DUAL);
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());
}

/* ==================================================================
 * SWR-CVC-010: Subsystem Fault State Transitions (Motor/Brake/Steering)
 * ================================================================== */

/** @verifies SWR-CVC-010 — RUN -> DEGRADED on motor cutoff signal (confirmed) */
void test_RUN_to_DEGRADED_on_motor_cutoff(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());

    /* Motor cutoff in RTE + Com confirms, 3 cycles */
    mock_rte_signals[CVC_SIG_MOTOR_CUTOFF] = 1u;
    mock_com_rx_values[14u] = 1u;  /* Com signal 14 = motor_cutoff */
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_DEGRADED, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-010 — RUN -> SAFE_STOP on brake fault signal (confirmed) */
void test_RUN_to_SAFE_STOP_on_brake_fault(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());

    /* Brake fault in RTE + Com confirms + E2E OK, 3 cycles for confirmation */
    mock_rte_signals[CVC_SIG_BRAKE_FAULT] = 1u;
    mock_com_rx_values[13u] = 1u;  /* Com signal 13 = brake_fault */
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-010 — RUN -> SAFE_STOP on steering fault signal (debounce only) */
void test_RUN_to_SAFE_STOP_on_steering_fault(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());

    /* Steering fault: debounce only (no Com, no E2E), 3 cycles */
    mock_rte_signals[CVC_SIG_STEERING_FAULT] = 1u;
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-011 — DEGRADED -> SAFE_STOP on motor cutoff (escalation, confirmed) */
void test_DEGRADED_to_SAFE_STOP_on_motor_cutoff(void)
{
    /* Get to DEGRADED via pedal fault */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_OnEvent(CVC_EVT_PEDAL_FAULT_SINGLE);
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_DEGRADED, Swc_VehicleState_GetState());

    /* Motor cutoff in DEGRADED -> escalate to SAFE_STOP (3-cycle confirmation) */
    mock_rte_signals[CVC_SIG_PEDAL_FAULT] = 1u;  /* Keep pedal fault to prevent FAULT_CLEARED */
    mock_rte_signals[CVC_SIG_MOTOR_CUTOFF] = 1u;
    mock_com_rx_values[14u] = 1u;  /* Com signal 14 = motor_cutoff */
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());
}

/* ==================================================================
 * SWR-CVC-013: SAFE_STOP Recovery
 * ================================================================== */

/** @verifies SWR-CVC-013 — SAFE_STOP recovery after all faults clear for 50 cycles */
void test_SAFE_STOP_recovery_when_all_faults_clear(void)
{
    uint16 i;

    /* Get to SAFE_STOP via dual CAN timeout */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());

    Swc_VehicleState_OnEvent(CVC_EVT_CAN_TIMEOUT_DUAL);
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());

    /* All faults clear, heartbeats restored */
    mock_rte_signals[CVC_SIG_ESTOP_ACTIVE]    = 0u;
    mock_rte_signals[CVC_SIG_MOTOR_CUTOFF]    = 0u;
    mock_rte_signals[CVC_SIG_BRAKE_FAULT]     = 0u;
    mock_rte_signals[CVC_SIG_STEERING_FAULT]  = 0u;
    mock_rte_signals[CVC_SIG_PEDAL_FAULT]     = 0u;
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;

    /* Run 49 cycles — should NOT recover yet */
    for (i = 0u; i < 49u; i++)
    {
        Swc_VehicleState_MainFunction();
    }
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());

    /* 50th cycle — recovery triggers, transitions to INIT */
    Swc_VehicleState_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_INIT, Swc_VehicleState_GetState());

    /* self_test_pass_pending should be set — next MF with heartbeats OK → RUN */
    Swc_VehicleState_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-013 — SAFE_STOP NO recovery when fault persists */
void test_SAFE_STOP_no_recovery_when_fault_persists(void)
{
    uint16 i;

    /* Get to SAFE_STOP */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_OnEvent(CVC_EVT_CAN_TIMEOUT_DUAL);
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());

    /* Motor cutoff still active — recovery blocked */
    mock_rte_signals[CVC_SIG_MOTOR_CUTOFF] = 1u;
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;

    /* Run 100 cycles — should NOT recover */
    for (i = 0u; i < 100u; i++)
    {
        Swc_VehicleState_MainFunction();
    }
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-013 — SAFE_STOP recovery counter resets on intermittent fault */
void test_SAFE_STOP_recovery_counter_resets_on_fault(void)
{
    uint16 i;

    /* Get to SAFE_STOP */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_OnEvent(CVC_EVT_CAN_TIMEOUT_DUAL);
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());

    /* All clear for 40 cycles */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    for (i = 0u; i < 40u; i++)
    {
        Swc_VehicleState_MainFunction();
    }
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());

    /* Fault appears briefly — resets counter */
    mock_rte_signals[CVC_SIG_BRAKE_FAULT] = 1u;
    Swc_VehicleState_MainFunction();
    mock_rte_signals[CVC_SIG_BRAKE_FAULT] = 0u;

    /* Need another 50 clear cycles from scratch */
    for (i = 0u; i < 49u; i++)
    {
        Swc_VehicleState_MainFunction();
    }
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());

    /* 50th cycle — NOW it recovers */
    Swc_VehicleState_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_INIT, Swc_VehicleState_GetState());
}

/* ==================================================================
 * SWR-CVC-010: Confirmation-Read Pattern Tests (ISO 26262)
 * ================================================================== */

/** @verifies SWR-CVC-010 — Brake fault: no immediate SAFE_STOP (1 cycle) */
void test_brake_fault_no_immediate_safe_stop(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());

    /* Brake fault in RTE for only 1 cycle — should NOT transition */
    mock_rte_signals[CVC_SIG_BRAKE_FAULT] = 1u;
    mock_com_rx_values[13u] = 1u;
    Swc_VehicleState_MainFunction();

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-010 — Brake fault confirmed after 3 cycles */
void test_brake_fault_confirmed_after_threshold(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());

    /* Brake fault in RTE + Com(13)=1, 3 cycles */
    mock_rte_signals[CVC_SIG_BRAKE_FAULT] = 1u;
    mock_com_rx_values[13u] = 1u;
    Swc_VehicleState_MainFunction();  /* count=1 */
    Swc_VehicleState_MainFunction();  /* count=2 */
    Swc_VehicleState_MainFunction();  /* count=3 -> confirmed */

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-010 — Brake fault clears before threshold: stays RUN */
void test_brake_fault_clears_before_threshold(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());

    /* Brake fault for 2 cycles, then clears */
    mock_rte_signals[CVC_SIG_BRAKE_FAULT] = 1u;
    mock_com_rx_values[13u] = 1u;
    Swc_VehicleState_MainFunction();  /* count=1 */
    Swc_VehicleState_MainFunction();  /* count=2 */

    /* Clear the fault */
    mock_rte_signals[CVC_SIG_BRAKE_FAULT] = 0u;
    mock_com_rx_values[13u] = 0u;
    Swc_VehicleState_MainFunction();  /* count reset to 0 */

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-010 — Brake fault: Com disagrees -> no transition (stale RTE) */
void test_brake_fault_com_disagrees_no_transition(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());

    /* RTE says brake_fault=1 but Com(13)=0 — stale RTE data */
    mock_rte_signals[CVC_SIG_BRAKE_FAULT] = 1u;
    mock_com_rx_values[13u] = 0u;  /* Com disagrees */
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();

    /* Should NOT transition — Com confirmation failed */
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-010 — Brake fault: E2E degraded -> no transition */
void test_brake_fault_e2e_degraded_no_transition(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());

    /* RTE=1, Com(13)=1, but E2E says useSafeDefault=TRUE */
    mock_rte_signals[CVC_SIG_BRAKE_FAULT] = 1u;
    mock_com_rx_values[13u] = 1u;
    mock_rx_status[2u].useSafeDefault = TRUE;  /* RX index 2 = 0x210 (brake) */
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();

    /* Should NOT transition — E2E check failed */
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-010 — Motor cutoff confirmed after 3 cycles */
void test_motor_cutoff_confirmed_after_threshold(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());

    /* Motor cutoff in RTE + Com(14)=1, 3 cycles */
    mock_rte_signals[CVC_SIG_MOTOR_CUTOFF] = 1u;
    mock_com_rx_values[14u] = 1u;
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();

    /* Motor cutoff from RUN -> DEGRADED (per transition table) */
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_DEGRADED, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-010 — Motor cutoff: Com disagrees -> no transition */
void test_motor_cutoff_com_disagrees_no_transition(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());

    /* RTE says motor_cutoff=1 but Com(14)=0 */
    mock_rte_signals[CVC_SIG_MOTOR_CUTOFF] = 1u;
    mock_com_rx_values[14u] = 0u;  /* Com disagrees */
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-010 — Steering fault: debounce only (3 cycles, no Com/E2E) */
void test_steering_fault_debounce_only(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());

    /* Steering fault: no Com, no E2E — debounce only */
    mock_rte_signals[CVC_SIG_STEERING_FAULT] = 1u;
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();
    Swc_VehicleState_MainFunction();

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());
}

/** @verifies SWR-CVC-010 — E-stop: immediate, no debounce (ASIL D) */
void test_estop_immediate_no_debounce(void)
{
    /* Get to RUN */
    mock_rte_signals[CVC_SIG_FZC_COMM_STATUS] = CVC_COMM_OK;
    mock_rte_signals[CVC_SIG_RZC_COMM_STATUS] = CVC_COMM_OK;
    Swc_VehicleState_OnEvent(CVC_EVT_SELF_TEST_PASS);
    Swc_VehicleState_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_RUN, Swc_VehicleState_GetState());

    /* E-stop: 1 cycle, immediate transition — no debounce */
    mock_rte_signals[CVC_SIG_ESTOP_ACTIVE] = 1u;
    Swc_VehicleState_MainFunction();

    TEST_ASSERT_EQUAL_UINT8(CVC_STATE_SAFE_STOP, Swc_VehicleState_GetState());
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

    /* --- HARDENED TESTS --- */

    /* SWR-CVC-009: Event boundary — out-of-range */
    RUN_TEST(test_OnEvent_boundary_event_at_count);
    RUN_TEST(test_OnEvent_boundary_event_max_uint8);

    /* SWR-CVC-009: E-stop override from various states */
    RUN_TEST(test_Estop_from_INIT_via_signal);
    RUN_TEST(test_Estop_from_LIMP_via_OnEvent);

    /* SWR-CVC-009: SC_KILL from various states */
    RUN_TEST(test_SC_KILL_from_DEGRADED);
    RUN_TEST(test_SC_KILL_from_LIMP);

    /* SWR-CVC-009: Terminal state — SHUTDOWN rejects all events */
    RUN_TEST(test_SHUTDOWN_rejects_all_events);

    /* SWR-CVC-009: Heartbeat guard on INIT->RUN transition */
    RUN_TEST(test_INIT_to_RUN_blocked_when_fzc_timeout);
    RUN_TEST(test_INIT_to_RUN_blocked_when_rzc_timeout);

    /* SWR-CVC-010: RTE failure and dual CAN timeout */
    RUN_TEST(test_MainFunction_rte_read_failure_no_crash);
    RUN_TEST(test_RUN_to_SAFE_STOP_on_dual_CAN_timeout);

    /* SWR-CVC-010/011: Subsystem fault state transitions */
    RUN_TEST(test_RUN_to_DEGRADED_on_motor_cutoff);
    RUN_TEST(test_RUN_to_SAFE_STOP_on_brake_fault);
    RUN_TEST(test_RUN_to_SAFE_STOP_on_steering_fault);
    RUN_TEST(test_DEGRADED_to_SAFE_STOP_on_motor_cutoff);

    /* SWR-CVC-013: SAFE_STOP recovery */
    RUN_TEST(test_SAFE_STOP_recovery_when_all_faults_clear);
    RUN_TEST(test_SAFE_STOP_no_recovery_when_fault_persists);
    RUN_TEST(test_SAFE_STOP_recovery_counter_resets_on_fault);

    /* SWR-CVC-010: Confirmation-read pattern (ISO 26262) */
    RUN_TEST(test_brake_fault_no_immediate_safe_stop);
    RUN_TEST(test_brake_fault_confirmed_after_threshold);
    RUN_TEST(test_brake_fault_clears_before_threshold);
    RUN_TEST(test_brake_fault_com_disagrees_no_transition);
    RUN_TEST(test_brake_fault_e2e_degraded_no_transition);
    RUN_TEST(test_motor_cutoff_confirmed_after_threshold);
    RUN_TEST(test_motor_cutoff_com_disagrees_no_transition);
    RUN_TEST(test_steering_fault_debounce_only);
    RUN_TEST(test_estop_immediate_no_debounce);

    return UNITY_END();
}

/* ==================================================================
 * Source inclusion — link SWC under test directly into test binary
 * ================================================================== */

/* Prevent BSW headers from redefining types when source is included */
#define PLATFORM_TYPES_H
#define STD_TYPES_H
#define SWC_VEHICLESTATE_H
#define SWC_CVCCOM_H
#define CVC_CFG_H
#define RTE_H
#define BSWM_H
#define DEM_H
#define COM_H

#include "../src/Swc_VehicleState.c"
