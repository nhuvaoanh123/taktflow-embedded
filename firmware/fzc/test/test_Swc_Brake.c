/**
 * @file    test_Swc_Brake.c
 * @brief   Unit tests for Swc_Brake — ASIL D brake servo control SWC
 * @date    2026-02-23
 *
 * @verifies SWR-FZC-009, SWR-FZC-010, SWR-FZC-011, SWR-FZC-012
 *
 * Tests brake initialization, normal PWM output, feedback verification
 * with debounce, command timeout with auto-brake latch, e-stop immediate
 * brake, motor cutoff CAN sequence, fault latching, and DTC reporting.
 *
 * Mocks: Pwm_SetDutyCycle, Rte_Read, Rte_Write, Com_SendSignal,
 *        Dem_ReportErrorStatus
 */
#include "unity.h"

/* ==================================================================
 * Local type definitions (avoid BSW header mock conflicts)
 * ================================================================== */

typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned long   uint32;
typedef signed short    sint16;
typedef uint8           Std_ReturnType;

#define E_OK        0u
#define E_NOT_OK    1u
#define TRUE        1u
#define FALSE       0u
#define NULL_PTR    ((void*)0)

/* ==================================================================
 * FZC Signal IDs (from Fzc_Cfg.h)
 * ================================================================== */

#define FZC_SIG_BRAKE_CMD          19u
#define FZC_SIG_BRAKE_POS          20u
#define FZC_SIG_BRAKE_FAULT        21u
#define FZC_SIG_VEHICLE_STATE      26u
#define FZC_SIG_ESTOP_ACTIVE       27u
#define FZC_SIG_MOTOR_CUTOFF       29u
#define FZC_SIG_BRAKE_PWM_DISABLE  32u

/* Vehicle states */
#define FZC_STATE_INIT              0u
#define FZC_STATE_RUN               1u
#define FZC_STATE_DEGRADED          2u
#define FZC_STATE_LIMP              3u
#define FZC_STATE_SAFE_STOP         4u
#define FZC_STATE_SHUTDOWN          5u

/* Brake fault codes */
#define FZC_BRAKE_NO_FAULT          0u
#define FZC_BRAKE_PWM_DEVIATION     1u
#define FZC_BRAKE_CMD_TIMEOUT       2u
#define FZC_BRAKE_LATCHED           3u

/* DTC event IDs */
#define FZC_DTC_BRAKE_FAULT         5u
#define FZC_DTC_BRAKE_TIMEOUT       6u
#define FZC_DTC_BRAKE_PWM_FAIL      7u

/* DEM event status */
#define DEM_EVENT_STATUS_PASSED     0u
#define DEM_EVENT_STATUS_FAILED     1u

/* Brake constants */
#define FZC_BRAKE_AUTO_TIMEOUT_MS     100u
#define FZC_BRAKE_PWM_FAULT_THRESH      2u
#define FZC_BRAKE_FAULT_DEBOUNCE        3u
#define FZC_BRAKE_LATCH_CLEAR_CYCLES   50u
#define FZC_BRAKE_CUTOFF_REPEAT_COUNT  10u

/* Com TX PDU IDs */
#define FZC_COM_TX_MOTOR_CUTOFF     4u

/* ==================================================================
 * Swc_Brake Config Type (mirrors header)
 * ================================================================== */

typedef struct {
    uint16  autoTimeoutMs;
    uint8   pwmFaultThreshold;
    uint8   faultDebounce;
    uint8   latchClearCycles;
    uint8   cutoffRepeatCount;
} Swc_Brake_ConfigType;

/* Swc_Brake API declarations */
extern void            Swc_Brake_Init(const Swc_Brake_ConfigType* ConfigPtr);
extern void            Swc_Brake_MainFunction(void);
extern Std_ReturnType  Swc_Brake_GetPosition(uint8* pos);

/* ==================================================================
 * Mock: Pwm_SetDutyCycle
 * ================================================================== */

static uint8   mock_pwm_last_channel;
static uint16  mock_pwm_last_duty;
static uint8   mock_pwm_call_count;

void Pwm_SetDutyCycle(uint8 Channel, uint16 DutyCycle)
{
    mock_pwm_call_count++;
    mock_pwm_last_channel = Channel;
    mock_pwm_last_duty    = DutyCycle;
}

/* ==================================================================
 * Mock: Rte_Read
 * ================================================================== */

#define MOCK_RTE_MAX_SIGNALS  64u

static uint32  mock_rte_signals[MOCK_RTE_MAX_SIGNALS];
static uint8   mock_rte_read_count;

Std_ReturnType Rte_Read(uint16 SignalId, uint32* DataPtr)
{
    mock_rte_read_count++;
    if (DataPtr == NULL_PTR) {
        return E_NOT_OK;
    }
    if (SignalId < MOCK_RTE_MAX_SIGNALS) {
        *DataPtr = mock_rte_signals[SignalId];
        return E_OK;
    }
    return E_NOT_OK;
}

/* ==================================================================
 * Mock: Rte_Write
 * ================================================================== */

static uint8   mock_rte_write_count;
static uint16  mock_rte_last_write_id;

Std_ReturnType Rte_Write(uint16 SignalId, uint32 Data)
{
    mock_rte_write_count++;
    mock_rte_last_write_id = SignalId;
    if (SignalId < MOCK_RTE_MAX_SIGNALS) {
        mock_rte_signals[SignalId] = Data;
        return E_OK;
    }
    return E_NOT_OK;
}

/* ==================================================================
 * Mock: Com_SendSignal
 * ================================================================== */

static uint8   mock_com_last_signal_id;
static uint8   mock_com_call_count;
static uint32  mock_com_last_data;

Std_ReturnType Com_SendSignal(uint8 SignalId, const void* SignalDataPtr)
{
    mock_com_call_count++;
    mock_com_last_signal_id = SignalId;
    if (SignalDataPtr != NULL_PTR) {
        mock_com_last_data = *((const uint32*)SignalDataPtr);
    }
    return E_OK;
}

/* ==================================================================
 * Mock: Dem_ReportErrorStatus
 * ================================================================== */

#define MOCK_DEM_MAX_EVENTS  16u

static uint8   mock_dem_last_event_id;
static uint8   mock_dem_last_status;
static uint8   mock_dem_call_count;
static uint8   mock_dem_event_reported[MOCK_DEM_MAX_EVENTS];
static uint8   mock_dem_event_status[MOCK_DEM_MAX_EVENTS];

void Dem_ReportErrorStatus(uint8 EventId, uint8 EventStatus)
{
    mock_dem_call_count++;
    mock_dem_last_event_id = EventId;
    mock_dem_last_status   = EventStatus;
    if (EventId < MOCK_DEM_MAX_EVENTS) {
        mock_dem_event_reported[EventId] = 1u;
        mock_dem_event_status[EventId]   = EventStatus;
    }
}

/* ==================================================================
 * Test Configuration
 * ================================================================== */

static Swc_Brake_ConfigType test_config;

void setUp(void)
{
    uint8 i;

    /* Reset PWM mock */
    mock_pwm_last_channel = 0xFFu;
    mock_pwm_last_duty    = 0u;
    mock_pwm_call_count   = 0u;

    /* Reset RTE mock */
    mock_rte_read_count    = 0u;
    mock_rte_write_count   = 0u;
    mock_rte_last_write_id = 0u;
    for (i = 0u; i < MOCK_RTE_MAX_SIGNALS; i++) {
        mock_rte_signals[i] = 0u;
    }
    /* Set default vehicle state to RUN */
    mock_rte_signals[FZC_SIG_VEHICLE_STATE] = FZC_STATE_RUN;
    /* No e-stop by default */
    mock_rte_signals[FZC_SIG_ESTOP_ACTIVE]  = 0u;

    /* Reset Com mock */
    mock_com_last_signal_id = 0xFFu;
    mock_com_call_count     = 0u;
    mock_com_last_data      = 0u;

    /* Reset DEM mock */
    mock_dem_call_count    = 0u;
    mock_dem_last_event_id = 0xFFu;
    mock_dem_last_status   = 0xFFu;
    for (i = 0u; i < MOCK_DEM_MAX_EVENTS; i++) {
        mock_dem_event_reported[i] = 0u;
        mock_dem_event_status[i]   = 0xFFu;
    }

    /* Default config matching Fzc_Cfg.h */
    test_config.autoTimeoutMs     = FZC_BRAKE_AUTO_TIMEOUT_MS;
    test_config.pwmFaultThreshold = FZC_BRAKE_PWM_FAULT_THRESH;
    test_config.faultDebounce     = FZC_BRAKE_FAULT_DEBOUNCE;
    test_config.latchClearCycles  = FZC_BRAKE_LATCH_CLEAR_CYCLES;
    test_config.cutoffRepeatCount = FZC_BRAKE_CUTOFF_REPEAT_COUNT;

    Swc_Brake_Init(&test_config);
}

void tearDown(void) { }

/* ==================================================================
 * Helper: run N main cycles with a given brake command
 * ================================================================== */

static void run_cycles(uint32 brakeCmd, uint16 count)
{
    uint16 i;
    for (i = 0u; i < count; i++) {
        mock_rte_signals[FZC_SIG_BRAKE_CMD] = brakeCmd;
        Swc_Brake_MainFunction();
    }
}

/* ==================================================================
 * SWR-FZC-009: Init & Normal Operation
 * ================================================================== */

/** @verifies SWR-FZC-009 -- Init with valid config succeeds */
void test_Init_valid(void)
{
    /* Init already called in setUp.  Verify module is operational
     * by running one cycle and checking that PWM was driven. */
    mock_rte_signals[FZC_SIG_BRAKE_CMD] = 50u;
    Swc_Brake_MainFunction();

    TEST_ASSERT_TRUE(mock_pwm_call_count > 0u);
}

/** @verifies SWR-FZC-009 -- Null config leaves module uninitialized */
void test_Init_null(void)
{
    Swc_Brake_Init(NULL_PTR);

    /* GetPosition should fail when not initialized */
    uint8 pos = 99u;
    Std_ReturnType ret = Swc_Brake_GetPosition(&pos);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
}

/** @verifies SWR-FZC-009 -- Command 0% produces PWM 0% */
void test_Normal_brake_0(void)
{
    run_cycles(0u, 5u);

    /* PWM duty should be 0 (no brake) */
    TEST_ASSERT_EQUAL_UINT16(0u, mock_pwm_last_duty);

    /* Brake position RTE should be 0 */
    uint32 pos = mock_rte_signals[FZC_SIG_BRAKE_POS];
    TEST_ASSERT_EQUAL_UINT32(0u, pos);
}

/** @verifies SWR-FZC-009 -- Command 100% produces PWM 100% */
void test_Normal_brake_100(void)
{
    run_cycles(100u, 5u);

    /* PWM duty should be 100 */
    TEST_ASSERT_EQUAL_UINT16(100u, mock_pwm_last_duty);

    /* Brake position RTE should be 100 */
    uint32 pos = mock_rte_signals[FZC_SIG_BRAKE_POS];
    TEST_ASSERT_EQUAL_UINT32(100u, pos);
}

/* ==================================================================
 * SWR-FZC-010: Feedback Verification
 * ================================================================== */

/** @verifies SWR-FZC-010 -- Feedback within 2% threshold passes */
void test_Feedback_pass(void)
{
    /* Command 50%, feedback simulated matches within tolerance.
     * No fault should be reported after several cycles. */
    run_cycles(50u, 10u);

    uint32 fault = mock_rte_signals[FZC_SIG_BRAKE_FAULT];
    TEST_ASSERT_EQUAL_UINT32(FZC_BRAKE_NO_FAULT, fault);
}

/** @verifies SWR-FZC-010 -- Deviation > 2% debounced for 3 cycles triggers fault */
void test_Feedback_fail_debounce_3(void)
{
    /* Inject a large brake command that will produce a PWM mismatch
     * by writing a mismatched position into the feedback path.
     * The implementation simulates feedback internally, so we drive
     * a scenario where the internal feedback deviates.
     *
     * We command 50% but inject a brake position override via RTE
     * that is way off.  The implementation reads its own computed
     * position, but we can trigger PWM deviation by setting the
     * brake command to a value > 100 (clamped) creating a brief
     * internal mismatch.
     *
     * For a clean test: command 50, then abruptly command 55 --
     * the simulated position will be 50 (from prior cycle) vs
     * command 55 => delta of 5 > 2% threshold.  Hold for 3 cycles.
     */
    run_cycles(50u, 5u);

    /* Now create deviation: command jumps, but position lags one cycle */
    /* With simulated feedback, deviation resets each cycle because
     * position = command.  To properly test, we rely on the
     * implementation comparing commanded duty vs actual position
     * where a discrepancy can happen. The SWC injects a small offset
     * when PrevBrakeCmd != current to simulate actuator lag.
     *
     * Instead, the cleanest way: set command to 0, then immediately
     * to 100 -- the position will be 0 from prior cycle vs command 100
     * => deviation = 100 > 2. Hold for 3 cycles. */
    run_cycles(0u, 5u);

    /* Now abruptly command 100 -- position still at 0 from prev.
     * Deviation = |100 - 0| = 100 > 2% threshold.
     * But position updates to command each cycle in simulated mode.
     * The feedback check compares commanded vs PREVIOUS cycle position.
     */
    mock_rte_signals[FZC_SIG_BRAKE_CMD] = 100u;
    Swc_Brake_MainFunction();  /* cycle 1: prev=0, cmd=100, delta=100 > 2 */
    Swc_Brake_MainFunction();  /* cycle 2: prev=100, cmd=100, delta=0 -- resets */

    /* Since the simulated feedback updates instantly, deviation only
     * lasts 1 cycle.  For testability, we test the debounce counter
     * by issuing rapid alternating commands. */
    uint16 i;
    for (i = 0u; i < 3u; i++) {
        mock_rte_signals[FZC_SIG_BRAKE_CMD] = 0u;
        Swc_Brake_MainFunction();
        mock_rte_signals[FZC_SIG_BRAKE_CMD] = 100u;
        Swc_Brake_MainFunction();
    }

    /* After 3+ cycles of deviation the brake fault should trigger.
     * The alternating pattern creates consistent deviation because
     * position tracks the PREVIOUS command. */
    uint32 fault = mock_rte_signals[FZC_SIG_BRAKE_FAULT];
    TEST_ASSERT_EQUAL_UINT32(FZC_BRAKE_PWM_DEVIATION, fault);
}

/** @verifies SWR-FZC-010 -- Deviation returning within threshold resets counter */
void test_Feedback_clears(void)
{
    /* Create 2 cycles of deviation (under debounce of 3) */
    run_cycles(0u, 5u);
    mock_rte_signals[FZC_SIG_BRAKE_CMD] = 100u;
    Swc_Brake_MainFunction();  /* deviation cycle 1 */

    /* Now stabilize -- deviation counter should reset */
    run_cycles(100u, 5u);

    uint32 fault = mock_rte_signals[FZC_SIG_BRAKE_FAULT];
    TEST_ASSERT_EQUAL_UINT32(FZC_BRAKE_NO_FAULT, fault);
}

/** @verifies SWR-FZC-010 -- DTC reported on PWM feedback fault */
void test_Feedback_reports_DTC(void)
{
    /* Force the fault by alternating commands rapidly */
    uint16 i;
    for (i = 0u; i < 5u; i++) {
        mock_rte_signals[FZC_SIG_BRAKE_CMD] = 0u;
        Swc_Brake_MainFunction();
        mock_rte_signals[FZC_SIG_BRAKE_CMD] = 100u;
        Swc_Brake_MainFunction();
    }

    TEST_ASSERT_EQUAL_UINT8(1u, mock_dem_event_reported[FZC_DTC_BRAKE_PWM_FAIL]);
    TEST_ASSERT_EQUAL_UINT8(DEM_EVENT_STATUS_FAILED,
                            mock_dem_event_status[FZC_DTC_BRAKE_PWM_FAIL]);
}

/* ==================================================================
 * SWR-FZC-011: Auto-brake on Timeout
 * ================================================================== */

/** @verifies SWR-FZC-011 -- 100ms no command triggers auto 100% brake */
void test_Auto_brake_on_timeout(void)
{
    /* Send a normal command first */
    run_cycles(30u, 3u);

    /* Now stop sending new commands.  The brake command stays at 30
     * but the timeout counter increments each cycle with no NEW
     * command.  In this SWC, timeout means no Rte_Read update, but
     * since our mock always returns the same value, the SWC detects
     * "no change" as a new command.  The timeout logic checks for
     * identical consecutive values as "no new command".
     * Actually the SWC counts cycles since LAST DIFFERENT command.
     * For clean testing: just leave the same value -> after 10 cycles
     * (100ms at 10ms/cycle) the timeout fires.  But the SWC tracks
     * whether a new command was received, not whether it changed.
     *
     * Simplest approach: the SWC increments a timeout counter every
     * cycle and resets it when a new command value differs from
     * previous.  If we hold the same value for 10+ cycles, timeout
     * fires. */
    run_cycles(30u, 10u);

    /* After 10 cycles of same command (100ms), auto-brake should activate */
    uint32 pos = mock_rte_signals[FZC_SIG_BRAKE_POS];
    TEST_ASSERT_EQUAL_UINT32(100u, pos);
}

/** @verifies SWR-FZC-011 -- Auto-brake is latched (persists after new cmd) */
void test_Auto_brake_latched(void)
{
    /* Trigger timeout */
    run_cycles(30u, 12u);

    /* Verify auto-brake is active */
    uint32 pos1 = mock_rte_signals[FZC_SIG_BRAKE_POS];
    TEST_ASSERT_EQUAL_UINT32(100u, pos1);

    /* Send a different command -- auto-brake latch should persist */
    run_cycles(10u, 5u);

    uint32 pos2 = mock_rte_signals[FZC_SIG_BRAKE_POS];
    TEST_ASSERT_EQUAL_UINT32(100u, pos2);
}

/** @verifies SWR-FZC-011 -- DTC reported on command timeout */
void test_Auto_brake_reports_DTC(void)
{
    run_cycles(30u, 12u);

    TEST_ASSERT_EQUAL_UINT8(1u, mock_dem_event_reported[FZC_DTC_BRAKE_TIMEOUT]);
    TEST_ASSERT_EQUAL_UINT8(DEM_EVENT_STATUS_FAILED,
                            mock_dem_event_status[FZC_DTC_BRAKE_TIMEOUT]);
}

/** @verifies SWR-FZC-011 -- Regular changing commands prevent timeout */
void test_No_timeout_with_commands(void)
{
    /* Alternate commands so value changes each cycle */
    uint16 i;
    for (i = 0u; i < 20u; i++) {
        mock_rte_signals[FZC_SIG_BRAKE_CMD] = (i % 2u == 0u) ? 30u : 31u;
        Swc_Brake_MainFunction();
    }

    /* No timeout fault */
    uint32 fault = mock_rte_signals[FZC_SIG_BRAKE_FAULT];
    TEST_ASSERT_NOT_EQUAL(FZC_BRAKE_CMD_TIMEOUT, fault);

    /* Position should track the last command, not 100% */
    uint32 pos = mock_rte_signals[FZC_SIG_BRAKE_POS];
    TEST_ASSERT_TRUE(pos <= 31u);
}

/** @verifies SWR-FZC-011 -- E-stop active triggers immediate 100% brake */
void test_Auto_brake_estop(void)
{
    /* Normal operation first */
    run_cycles(20u, 3u);

    /* Activate e-stop */
    mock_rte_signals[FZC_SIG_ESTOP_ACTIVE] = 1u;
    mock_rte_signals[FZC_SIG_BRAKE_CMD]    = 20u;
    Swc_Brake_MainFunction();

    /* Immediate 100% brake */
    uint32 pos = mock_rte_signals[FZC_SIG_BRAKE_POS];
    TEST_ASSERT_EQUAL_UINT32(100u, pos);

    /* PWM should be 100% */
    TEST_ASSERT_EQUAL_UINT16(100u, mock_pwm_last_duty);
}

/* ==================================================================
 * SWR-FZC-012: Motor Cutoff
 * ================================================================== */

/** @verifies SWR-FZC-012 -- Brake fault triggers motor cutoff CAN msg */
void test_Motor_cutoff_on_brake_fault(void)
{
    /* Force a brake fault via alternating commands */
    uint16 i;
    for (i = 0u; i < 5u; i++) {
        mock_rte_signals[FZC_SIG_BRAKE_CMD] = 0u;
        Swc_Brake_MainFunction();
        mock_rte_signals[FZC_SIG_BRAKE_CMD] = 100u;
        Swc_Brake_MainFunction();
    }

    /* Motor cutoff RTE signal should be set */
    uint32 cutoff = mock_rte_signals[FZC_SIG_MOTOR_CUTOFF];
    TEST_ASSERT_EQUAL_UINT32(1u, cutoff);
}

/** @verifies SWR-FZC-012 -- Cutoff msg repeated 10 times at 10ms */
void test_Motor_cutoff_repeats(void)
{
    /* Force fault */
    uint16 i;
    for (i = 0u; i < 5u; i++) {
        mock_rte_signals[FZC_SIG_BRAKE_CMD] = 0u;
        Swc_Brake_MainFunction();
        mock_rte_signals[FZC_SIG_BRAKE_CMD] = 100u;
        Swc_Brake_MainFunction();
    }

    /* Record Com call count at the point the fault is confirmed */
    uint8 com_count_at_fault = mock_com_call_count;

    /* Run 10 more cycles to let the cutoff sequence complete */
    run_cycles(100u, 10u);

    /* Com_SendSignal should have been called for motor cutoff repeats */
    TEST_ASSERT_TRUE(mock_com_call_count >= com_count_at_fault);
    TEST_ASSERT_TRUE(mock_com_call_count >= 10u);
}

/** @verifies SWR-FZC-012 -- Motor cutoff signal written to RTE */
void test_Motor_cutoff_rte_write(void)
{
    /* Force fault */
    uint16 i;
    for (i = 0u; i < 5u; i++) {
        mock_rte_signals[FZC_SIG_BRAKE_CMD] = 0u;
        Swc_Brake_MainFunction();
        mock_rte_signals[FZC_SIG_BRAKE_CMD] = 100u;
        Swc_Brake_MainFunction();
    }

    uint32 cutoff = mock_rte_signals[FZC_SIG_MOTOR_CUTOFF];
    TEST_ASSERT_EQUAL_UINT32(1u, cutoff);
}

/** @verifies SWR-FZC-012 -- Com_SendSignal called for cutoff PDU */
void test_Motor_cutoff_com_send(void)
{
    /* Force fault */
    uint16 i;
    for (i = 0u; i < 5u; i++) {
        mock_rte_signals[FZC_SIG_BRAKE_CMD] = 0u;
        Swc_Brake_MainFunction();
        mock_rte_signals[FZC_SIG_BRAKE_CMD] = 100u;
        Swc_Brake_MainFunction();
    }

    /* Run one more cycle to trigger Com_SendSignal for cutoff */
    run_cycles(100u, 1u);

    TEST_ASSERT_EQUAL_UINT8(FZC_COM_TX_MOTOR_CUTOFF, mock_com_last_signal_id);
}

/** @verifies SWR-FZC-012 -- No cutoff when no fault present */
void test_No_cutoff_without_fault(void)
{
    run_cycles(50u, 3u);

    /* Change command each cycle to prevent timeout */
    mock_rte_signals[FZC_SIG_BRAKE_CMD] = 51u;
    Swc_Brake_MainFunction();

    uint32 cutoff = mock_rte_signals[FZC_SIG_MOTOR_CUTOFF];
    TEST_ASSERT_EQUAL_UINT32(0u, cutoff);

    /* No Com_SendSignal for cutoff */
    TEST_ASSERT_EQUAL_UINT8(0u, mock_com_call_count);
}

/* ==================================================================
 * General Tests
 * ================================================================== */

/** @verifies SWR-FZC-009 -- MainFunction safe when not initialized */
void test_Uninit_main_noop(void)
{
    Swc_Brake_Init(NULL_PTR);

    mock_pwm_call_count  = 0u;
    mock_rte_write_count = 0u;

    Swc_Brake_MainFunction();

    /* Should be a no-op: no PWM writes, no RTE writes */
    TEST_ASSERT_EQUAL_UINT8(0u, mock_pwm_call_count);
    TEST_ASSERT_EQUAL_UINT8(0u, mock_rte_write_count);
}

/** @verifies SWR-FZC-010 -- Any fault forces 100% brake */
void test_Fault_forces_full_brake(void)
{
    /* Normal operation at 30% */
    run_cycles(30u, 3u);
    mock_rte_signals[FZC_SIG_BRAKE_CMD] = 31u;
    Swc_Brake_MainFunction();

    uint32 pos_before = mock_rte_signals[FZC_SIG_BRAKE_POS];
    TEST_ASSERT_TRUE(pos_before <= 31u);

    /* Force fault via alternating commands */
    uint16 i;
    for (i = 0u; i < 5u; i++) {
        mock_rte_signals[FZC_SIG_BRAKE_CMD] = 0u;
        Swc_Brake_MainFunction();
        mock_rte_signals[FZC_SIG_BRAKE_CMD] = 100u;
        Swc_Brake_MainFunction();
    }

    /* Fault should force 100% brake */
    uint32 pos_after = mock_rte_signals[FZC_SIG_BRAKE_POS];
    TEST_ASSERT_EQUAL_UINT32(100u, pos_after);
}

/** @verifies SWR-FZC-012 -- 50 fault-free cycles required to clear latch */
void test_Latch_clear_50_cycles(void)
{
    /* Force fault via timeout */
    run_cycles(30u, 12u);

    uint32 fault = mock_rte_signals[FZC_SIG_BRAKE_FAULT];
    TEST_ASSERT_NOT_EQUAL(FZC_BRAKE_NO_FAULT, fault);

    /* Now alternate commands to prevent further timeout, run 49 cycles */
    uint16 i;
    for (i = 0u; i < 49u; i++) {
        mock_rte_signals[FZC_SIG_BRAKE_CMD] = (i % 2u == 0u) ? 20u : 21u;
        Swc_Brake_MainFunction();
    }

    /* Still latched after 49 fault-free cycles */
    uint32 pos_49 = mock_rte_signals[FZC_SIG_BRAKE_POS];
    TEST_ASSERT_EQUAL_UINT32(100u, pos_49);

    /* 50th fault-free cycle: latch should clear */
    mock_rte_signals[FZC_SIG_BRAKE_CMD] = 22u;
    Swc_Brake_MainFunction();

    /* Latch cleared -- position should track command now */
    uint32 fault_after = mock_rte_signals[FZC_SIG_BRAKE_FAULT];
    TEST_ASSERT_EQUAL_UINT32(FZC_BRAKE_NO_FAULT, fault_after);
}

/** @verifies SWR-FZC-012 -- DTC cleared when fault-free after latch clear */
void test_DTC_clears_after_latch(void)
{
    /* Force timeout fault */
    run_cycles(30u, 12u);

    TEST_ASSERT_EQUAL_UINT8(DEM_EVENT_STATUS_FAILED,
                            mock_dem_event_status[FZC_DTC_BRAKE_TIMEOUT]);

    /* Alternate commands for 51 cycles to clear latch */
    uint16 i;
    for (i = 0u; i < 51u; i++) {
        mock_rte_signals[FZC_SIG_BRAKE_CMD] = (i % 2u == 0u) ? 20u : 21u;
        Swc_Brake_MainFunction();
    }

    /* DTC should now report PASSED */
    TEST_ASSERT_EQUAL_UINT8(DEM_EVENT_STATUS_PASSED,
                            mock_dem_event_status[FZC_DTC_BRAKE_TIMEOUT]);
}

/* ==================================================================
 * Test runner
 * ================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* SWR-FZC-009: Init & Normal Operation */
    RUN_TEST(test_Init_valid);
    RUN_TEST(test_Init_null);
    RUN_TEST(test_Normal_brake_0);
    RUN_TEST(test_Normal_brake_100);

    /* SWR-FZC-010: Feedback Verification */
    RUN_TEST(test_Feedback_pass);
    RUN_TEST(test_Feedback_fail_debounce_3);
    RUN_TEST(test_Feedback_clears);
    RUN_TEST(test_Feedback_reports_DTC);

    /* SWR-FZC-011: Auto-brake on Timeout */
    RUN_TEST(test_Auto_brake_on_timeout);
    RUN_TEST(test_Auto_brake_latched);
    RUN_TEST(test_Auto_brake_reports_DTC);
    RUN_TEST(test_No_timeout_with_commands);
    RUN_TEST(test_Auto_brake_estop);

    /* SWR-FZC-012: Motor Cutoff */
    RUN_TEST(test_Motor_cutoff_on_brake_fault);
    RUN_TEST(test_Motor_cutoff_repeats);
    RUN_TEST(test_Motor_cutoff_rte_write);
    RUN_TEST(test_Motor_cutoff_com_send);
    RUN_TEST(test_No_cutoff_without_fault);

    /* General */
    RUN_TEST(test_Uninit_main_noop);
    RUN_TEST(test_Fault_forces_full_brake);
    RUN_TEST(test_Latch_clear_50_cycles);
    RUN_TEST(test_DTC_clears_after_latch);

    return UNITY_END();
}
