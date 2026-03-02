/**
 * @file    Swc_VehicleState.c
 * @brief   Vehicle state machine — transition table, fault monitoring
 * @date    2026-02-21
 *
 * @details Implements a table-driven state machine for the CVC with 6 states
 *          and 14 events. The transition table is a const 2D array that maps
 *          (current_state, event) -> next_state. Invalid combinations map to
 *          CVC_STATE_INVALID (0xFF) and are rejected.
 *
 *          The MainFunction runs every 10ms and:
 *          1. Reads fault signals from RTE (pedal, E-stop, comm, motor, brake, steering)
 *          2. Derives events from signal values
 *          3. Calls OnEvent for derived events
 *          4. Reports DTCs for motor cutoff, brake fault, steering fault
 *          5. Writes current state to RTE
 *
 * @safety_req SWR-CVC-009 to SWR-CVC-013
 * @traces_to  SSR-CVC-009 to SSR-CVC-013, TSR-046, TSR-047
 *
 * @standard AUTOSAR, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */

#include "Swc_VehicleState.h"
#include "Swc_CvcCom.h"
#include "Cvc_Cfg.h"
#include "Rte.h"
#include "BswM.h"
#include "Dem.h"
#include "Com.h"

/* SIL diagnostic logging — compile with -DSIL_DIAG to enable */
#ifdef SIL_DIAG
#include <stdio.h>
#define VSM_DIAG(fmt, ...) (void)fprintf(stderr, "[VSM] " fmt "\n", ##__VA_ARGS__)
static const char * const diag_state_names[CVC_STATE_COUNT] = {
    "INIT", "RUN", "DEGRADED", "LIMP", "SAFE_STOP", "SHUTDOWN"
};
static const char * const diag_event_names[CVC_EVT_COUNT] = {
    "SELF_TEST_PASS", "SELF_TEST_FAIL",
    "PEDAL_FAULT_S", "PEDAL_FAULT_D",
    "CAN_TMO_S", "CAN_TMO_D",
    "ESTOP", "SC_KILL",
    "FAULT_CLR", "CAN_RESTORED", "VEH_STOPPED",
    "MOTOR_CUTOFF", "BRAKE_FAULT", "STEER_FAULT"
};
#else
#define VSM_DIAG(fmt, ...) ((void)0)
#endif

/* ==================================================================
 * BswM mode mapping — vehicle state to BswM mode
 * ================================================================== */

#define BSWM_STARTUP    0u
#define BSWM_RUN        1u
#define BSWM_DEGRADED   2u
#define BSWM_SAFE_STOP  3u
#define BSWM_SHUTDOWN   4u

/** @brief  Maps CVC vehicle state to BswM mode value */
static const uint8 state_to_bswm_mode[CVC_STATE_COUNT] = {
    BSWM_STARTUP,     /* CVC_STATE_INIT      -> BSWM_STARTUP   */
    BSWM_RUN,         /* CVC_STATE_RUN       -> BSWM_RUN       */
    BSWM_DEGRADED,    /* CVC_STATE_DEGRADED  -> BSWM_DEGRADED  */
    BSWM_DEGRADED,    /* CVC_STATE_LIMP      -> BSWM_DEGRADED  */
    BSWM_SAFE_STOP,   /* CVC_STATE_SAFE_STOP -> BSWM_SAFE_STOP */
    BSWM_SHUTDOWN     /* CVC_STATE_SHUTDOWN  -> BSWM_SHUTDOWN   */
};

/* ==================================================================
 * Transition table: [current_state][event] -> next_state
 *
 * CVC_STATE_INVALID (0xFF) means the transition is not allowed.
 * ================================================================== */

static const uint8 transition_table[CVC_STATE_COUNT][CVC_EVT_COUNT] = {
    /* CVC_STATE_INIT */
    {
        CVC_STATE_RUN,         /* EVT_SELF_TEST_PASS     -> RUN (guarded: heartbeats OK) */
        CVC_STATE_SAFE_STOP,   /* EVT_SELF_TEST_FAIL     -> SAFE_STOP    */
        CVC_STATE_INVALID,     /* EVT_PEDAL_FAULT_SINGLE -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_PEDAL_FAULT_DUAL   -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_CAN_TIMEOUT_SINGLE -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_CAN_TIMEOUT_DUAL   -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_ESTOP              -> (invalid)    */
        CVC_STATE_SAFE_STOP,   /* EVT_SC_KILL            -> SAFE_STOP    */
        CVC_STATE_INVALID,     /* EVT_FAULT_CLEARED      -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_CAN_RESTORED       -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_VEHICLE_STOPPED    -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_MOTOR_CUTOFF       -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_BRAKE_FAULT        -> (invalid)    */
        CVC_STATE_INVALID      /* EVT_STEERING_FAULT     -> (invalid)    */
    },
    /* CVC_STATE_RUN */
    {
        CVC_STATE_INVALID,     /* EVT_SELF_TEST_PASS     -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_SELF_TEST_FAIL     -> (invalid)    */
        CVC_STATE_DEGRADED,    /* EVT_PEDAL_FAULT_SINGLE -> DEGRADED     */
        CVC_STATE_SAFE_STOP,   /* EVT_PEDAL_FAULT_DUAL   -> SAFE_STOP    */
        CVC_STATE_LIMP,        /* EVT_CAN_TIMEOUT_SINGLE -> LIMP         */
        CVC_STATE_SAFE_STOP,   /* EVT_CAN_TIMEOUT_DUAL   -> SAFE_STOP    */
        CVC_STATE_SAFE_STOP,   /* EVT_ESTOP              -> SAFE_STOP    */
        CVC_STATE_SAFE_STOP,   /* EVT_SC_KILL            -> SAFE_STOP    */
        CVC_STATE_INVALID,     /* EVT_FAULT_CLEARED      -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_CAN_RESTORED       -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_VEHICLE_STOPPED    -> (invalid)    */
        CVC_STATE_DEGRADED,    /* EVT_MOTOR_CUTOFF       -> DEGRADED     */
        CVC_STATE_SAFE_STOP,   /* EVT_BRAKE_FAULT        -> SAFE_STOP    */
        CVC_STATE_SAFE_STOP    /* EVT_STEERING_FAULT     -> SAFE_STOP    */
    },
    /* CVC_STATE_DEGRADED */
    {
        CVC_STATE_INVALID,     /* EVT_SELF_TEST_PASS     -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_SELF_TEST_FAIL     -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_PEDAL_FAULT_SINGLE -> (invalid)    */
        CVC_STATE_SAFE_STOP,   /* EVT_PEDAL_FAULT_DUAL   -> SAFE_STOP    */
        CVC_STATE_LIMP,        /* EVT_CAN_TIMEOUT_SINGLE -> LIMP         */
        CVC_STATE_SAFE_STOP,   /* EVT_CAN_TIMEOUT_DUAL   -> SAFE_STOP    */
        CVC_STATE_SAFE_STOP,   /* EVT_ESTOP              -> SAFE_STOP    */
        CVC_STATE_SAFE_STOP,   /* EVT_SC_KILL            -> SAFE_STOP    */
        CVC_STATE_RUN,         /* EVT_FAULT_CLEARED      -> RUN          */
        CVC_STATE_INVALID,     /* EVT_CAN_RESTORED       -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_VEHICLE_STOPPED    -> (invalid)    */
        CVC_STATE_SAFE_STOP,   /* EVT_MOTOR_CUTOFF       -> SAFE_STOP    */
        CVC_STATE_SAFE_STOP,   /* EVT_BRAKE_FAULT        -> SAFE_STOP    */
        CVC_STATE_SAFE_STOP    /* EVT_STEERING_FAULT     -> SAFE_STOP    */
    },
    /* CVC_STATE_LIMP */
    {
        CVC_STATE_INVALID,     /* EVT_SELF_TEST_PASS     -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_SELF_TEST_FAIL     -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_PEDAL_FAULT_SINGLE -> (invalid)    */
        CVC_STATE_SAFE_STOP,   /* EVT_PEDAL_FAULT_DUAL   -> SAFE_STOP    */
        CVC_STATE_INVALID,     /* EVT_CAN_TIMEOUT_SINGLE -> (invalid)    */
        CVC_STATE_SAFE_STOP,   /* EVT_CAN_TIMEOUT_DUAL   -> SAFE_STOP    */
        CVC_STATE_SAFE_STOP,   /* EVT_ESTOP              -> SAFE_STOP    */
        CVC_STATE_SAFE_STOP,   /* EVT_SC_KILL            -> SAFE_STOP    */
        CVC_STATE_RUN,         /* EVT_FAULT_CLEARED      -> RUN          */
        CVC_STATE_DEGRADED,    /* EVT_CAN_RESTORED       -> DEGRADED     */
        CVC_STATE_INVALID,     /* EVT_VEHICLE_STOPPED    -> (invalid)    */
        CVC_STATE_SAFE_STOP,   /* EVT_MOTOR_CUTOFF       -> SAFE_STOP    */
        CVC_STATE_SAFE_STOP,   /* EVT_BRAKE_FAULT        -> SAFE_STOP    */
        CVC_STATE_SAFE_STOP    /* EVT_STEERING_FAULT     -> SAFE_STOP    */
    },
    /* CVC_STATE_SAFE_STOP */
    {
        CVC_STATE_INVALID,     /* EVT_SELF_TEST_PASS     -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_SELF_TEST_FAIL     -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_PEDAL_FAULT_SINGLE -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_PEDAL_FAULT_DUAL   -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_CAN_TIMEOUT_SINGLE -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_CAN_TIMEOUT_DUAL   -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_ESTOP              -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_SC_KILL            -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_FAULT_CLEARED      -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_CAN_RESTORED       -> (invalid)    */
        CVC_STATE_SHUTDOWN,    /* EVT_VEHICLE_STOPPED    -> SHUTDOWN     */
        CVC_STATE_INVALID,     /* EVT_MOTOR_CUTOFF       -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_BRAKE_FAULT        -> (invalid)    */
        CVC_STATE_INVALID      /* EVT_STEERING_FAULT     -> (invalid)    */
    },
    /* CVC_STATE_SHUTDOWN */
    {
        CVC_STATE_INVALID,     /* EVT_SELF_TEST_PASS     -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_SELF_TEST_FAIL     -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_PEDAL_FAULT_SINGLE -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_PEDAL_FAULT_DUAL   -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_CAN_TIMEOUT_SINGLE -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_CAN_TIMEOUT_DUAL   -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_ESTOP              -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_SC_KILL            -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_FAULT_CLEARED      -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_CAN_RESTORED       -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_VEHICLE_STOPPED    -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_MOTOR_CUTOFF       -> (invalid)    */
        CVC_STATE_INVALID,     /* EVT_BRAKE_FAULT        -> (invalid)    */
        CVC_STATE_INVALID      /* EVT_STEERING_FAULT     -> (invalid)    */
    }
};

/* ==================================================================
 * Module-static variables
 * ================================================================== */

/** @brief  Current vehicle state */
static uint8 current_state;

/** @brief  Initialization flag — TRUE after Swc_VehicleState_Init() */
static uint8 initialized;

/** @brief  Pending self-test pass flag — held until heartbeats validated */
static uint8 self_test_pass_pending;

/** @brief  INIT hold counter — counts cycles in INIT before allowing RUN */
static uint16 init_hold_counter;

/** @brief  SAFE_STOP recovery counter — counts all-clear cycles before recovery */
static uint16 safe_stop_clear_count;

/** @brief  Cycles required with all faults clear before SAFE_STOP recovery (500ms) */
#define CVC_SAFE_STOP_RECOVERY_CYCLES   50u

/* ==================================================================
 * Confirmation-read pattern — ISO 26262 debounce + fresh Com + E2E
 * ================================================================== */

/** @brief  Per-fault debounce counters: [brake, motor_cutoff, steering] */
static uint8 fault_confirm_count[3];

#define CVC_FAULT_CONFIRM_THRESHOLD  3u   /**< 3 consecutive 10ms cycles = 30ms */
#define CVC_FAULT_IDX_BRAKE          0u
#define CVC_FAULT_IDX_MOTOR_CUTOFF   1u
#define CVC_FAULT_IDX_STEERING       2u
#define CVC_FAULT_CONFIRM_COUNT      3u

#define CVC_FAULT_COM_BRAKE         13u   /**< Com signal ID for brake_fault   */
#define CVC_FAULT_COM_MOTOR_CUTOFF  14u   /**< Com signal ID for motor_cutoff  */
#define CVC_FAULT_COM_STEERING      0xFFu /**< Not bridged via Com             */

#define CVC_FAULT_E2E_BRAKE          2u   /**< CvcCom RX index for 0x210       */
#define CVC_FAULT_E2E_MOTOR_CUTOFF   0xFFu /**< 0x211 not in CvcCom RxTable   */
#define CVC_FAULT_E2E_STEERING       0xFFu

/* ==================================================================
 * Public API
 * ================================================================== */

/**
 * @brief  Initialize the vehicle state machine
 * @safety_req SWR-CVC-009
 */
void Swc_VehicleState_Init(void)
{
    uint8 i;

    current_state          = CVC_STATE_INIT;
    initialized            = TRUE;
    self_test_pass_pending = FALSE;
    init_hold_counter      = 0u;
    safe_stop_clear_count  = 0u;

    for (i = 0u; i < CVC_FAULT_CONFIRM_COUNT; i++)
    {
        fault_confirm_count[i] = 0u;
    }
}

/**
 * @brief  Get the current vehicle state
 * @return Current state value (CVC_STATE_INIT..CVC_STATE_SHUTDOWN)
 * @safety_req SWR-CVC-009
 */
uint8 Swc_VehicleState_GetState(void)
{
    return current_state;
}

/**
 * @brief  Inject an event and execute the state transition if valid
 * @param  event  Event ID (0..CVC_EVT_COUNT-1)
 * @safety_req SWR-CVC-009 to SWR-CVC-013
 *
 * @note   For SELF_TEST_PASS in INIT state, the transition is deferred
 *         until MainFunction validates that both heartbeats are OK.
 *         All other valid transitions execute immediately and notify BswM.
 */
void Swc_VehicleState_OnEvent(uint8 event)
{
    uint8 next_state;

    /* Guard: must be initialized, event in range, state in range */
    if (initialized != TRUE)
    {
        return;
    }
    if (event >= CVC_EVT_COUNT)
    {
        return;
    }
    if (current_state >= CVC_STATE_COUNT)
    {
        return;
    }

    /* Special handling: SELF_TEST_PASS in INIT requires heartbeat guard */
    if ((event == CVC_EVT_SELF_TEST_PASS) && (current_state == CVC_STATE_INIT))
    {
        self_test_pass_pending = TRUE;
        VSM_DIAG("self-test pass pending (waiting for heartbeats)");
        return;
    }

    /* Look up transition */
    next_state = transition_table[current_state][event];

    /* Reject invalid transitions */
    if (next_state == CVC_STATE_INVALID)
    {
        return;
    }

    /* Execute transition */
    {
#ifdef SIL_DIAG
        uint8 prev = current_state;
#endif
        current_state = next_state;
#ifdef SIL_DIAG
        VSM_DIAG("%s + %s -> %s",
                 diag_state_names[prev],
                 diag_event_names[event],
                 diag_state_names[current_state]);
#endif
    }

    /* Notify BswM of the new mode */
    (void)BswM_RequestMode(CVC_ECU_ID_CVC, state_to_bswm_mode[current_state]);
}

/**
 * @brief  Confirm a fault signal before safety-critical transition
 * @param  faultIdx     Index into fault_confirm_count array
 * @param  rte_value    RTE signal value (non-zero = fault active)
 * @param  comSignalId  Com signal ID for fresh read (0xFF = no Com check)
 * @param  e2eRxIndex   CvcCom RX index for E2E check (0xFF = no E2E check)
 * @param  dtcId        DTC event ID for Dem_ReportErrorStatus
 * @param  eventId      Vehicle state event to fire on confirmed fault
 *
 * @details ISO 26262 confirmation-read pattern:
 *          1. Debounce: fault must persist for CVC_FAULT_CONFIRM_THRESHOLD cycles
 *          2. Com read: fresh value from Com shadow buffer (bypasses stale RTE)
 *          3. E2E check: verify message is not degraded (useSafeDefault == FALSE)
 *          Only if all checks pass is the event fired.
 */
static void Swc_VehicleState_ConfirmFault(
    uint8 faultIdx, uint32 rte_value,
    uint8 comSignalId, uint8 e2eRxIndex,
    uint8 dtcId, uint8 eventId)
{
    if (rte_value != 0u)
    {
        fault_confirm_count[faultIdx]++;

        if (fault_confirm_count[faultIdx] >= CVC_FAULT_CONFIRM_THRESHOLD)
        {
            uint8 confirmed = TRUE;

            /* Fresh read from Com (bypass RTE stale cache) */
            if (comSignalId != 0xFFu)
            {
                uint8 fresh_val = 0u;
                (void)Com_ReceiveSignal(comSignalId, &fresh_val);
                if (fresh_val == 0u)
                {
                    confirmed = FALSE;
                }
            }

            /* E2E status check */
            if ((confirmed == TRUE) && (e2eRxIndex != 0xFFu))
            {
                Swc_CvcCom_RxStatusType rx_status;
                rx_status.failCount     = 0u;
                rx_status.useSafeDefault = FALSE;

                if (Swc_CvcCom_GetRxStatus(e2eRxIndex, &rx_status) == E_OK)
                {
                    if (rx_status.useSafeDefault == TRUE)
                    {
                        confirmed = FALSE;
                    }
                }
            }

            if (confirmed == TRUE)
            {
                VSM_DIAG("CONFIRM idx=%u rte=%u com=%u e2e=%u evt=%u",
                         (unsigned)faultIdx, (unsigned)rte_value,
                         (unsigned)comSignalId, (unsigned)e2eRxIndex,
                         (unsigned)eventId);
                Dem_ReportErrorStatus(dtcId, DEM_EVENT_STATUS_FAILED);
                Swc_VehicleState_OnEvent(eventId);
            }

            fault_confirm_count[faultIdx] = 0u;
        }
    }
    else
    {
        fault_confirm_count[faultIdx] = 0u;
    }
}

/**
 * @brief  10ms cyclic main function — reads faults, derives events, reports DTCs
 * @safety_req SWR-CVC-009 to SWR-CVC-013
 *
 * @note   Execution order:
 *         1. Read all fault signals from RTE
 *         2. Handle pending self-test pass (check heartbeats)
 *         3. Derive events from current signal values
 *         4. Report DTCs for motor cutoff, brake fault, steering fault
 *         5. Write current state to RTE
 */
void Swc_VehicleState_MainFunction(void)
{
    uint32 pedal_fault    = 0u;
    uint32 estop_active   = 0u;
    uint32 fzc_comm       = 0u;
    uint32 rzc_comm       = 0u;
    uint32 motor_cutoff   = 0u;
    uint32 brake_fault    = 0u;
    uint32 steering_fault = 0u;
    uint32 sc_relay_kill  = 0u;

    if (initialized != TRUE)
    {
        return;
    }

    /* ---- Step 1: Read all fault signals from RTE ---- */
    (void)Rte_Read(CVC_SIG_PEDAL_FAULT,      &pedal_fault);
    (void)Rte_Read(CVC_SIG_ESTOP_ACTIVE,     &estop_active);
    (void)Rte_Read(CVC_SIG_FZC_COMM_STATUS,  &fzc_comm);
    (void)Rte_Read(CVC_SIG_RZC_COMM_STATUS,  &rzc_comm);
    (void)Rte_Read(CVC_SIG_MOTOR_CUTOFF,     &motor_cutoff);
    (void)Rte_Read(CVC_SIG_BRAKE_FAULT,      &brake_fault);
    (void)Rte_Read(CVC_SIG_STEERING_FAULT,   &steering_fault);
    (void)Rte_Read(CVC_SIG_SC_RELAY_KILL,   &sc_relay_kill);

#ifdef SIL_DIAG
    {
        static uint16 diag_cycle = 0u;
        static uint32 prev_mc = 0u;
        static uint32 prev_bf = 0u;
        diag_cycle++;
        if (diag_cycle <= 100u) {
            VSM_DIAG("c=%u st=%u ped=%u es=%u fzc=%u rzc=%u mc=%u bf=%u sf=%u",
                     (unsigned)diag_cycle, (unsigned)current_state,
                     (unsigned)pedal_fault, (unsigned)estop_active,
                     (unsigned)fzc_comm, (unsigned)rzc_comm,
                     (unsigned)motor_cutoff, (unsigned)brake_fault,
                     (unsigned)steering_fault);
        }
        /* Log whenever motor_cutoff or brake_fault transitions to non-zero */
        if ((motor_cutoff != 0u) && (prev_mc == 0u)) {
            VSM_DIAG("!! mc ONSET c=%u st=%u mc=%u bf=%u fzc=%u rzc=%u",
                     (unsigned)diag_cycle, (unsigned)current_state,
                     (unsigned)motor_cutoff, (unsigned)brake_fault,
                     (unsigned)fzc_comm, (unsigned)rzc_comm);
        }
        if ((brake_fault != 0u) && (prev_bf == 0u)) {
            VSM_DIAG("!! bf ONSET c=%u st=%u mc=%u bf=%u fzc=%u rzc=%u",
                     (unsigned)diag_cycle, (unsigned)current_state,
                     (unsigned)motor_cutoff, (unsigned)brake_fault,
                     (unsigned)fzc_comm, (unsigned)rzc_comm);
        }
        prev_mc = motor_cutoff;
        prev_bf = brake_fault;
    }
#endif

    /* ---- Step 2: INIT hold timer + pending self-test pass guard ---- */
    if (current_state == CVC_STATE_INIT)
    {
        if (init_hold_counter < CVC_INIT_HOLD_CYCLES)
        {
            init_hold_counter++;
        }

        if ((self_test_pass_pending == TRUE)
            && (init_hold_counter >= CVC_INIT_HOLD_CYCLES)
            && (fzc_comm == CVC_COMM_OK)
            && (rzc_comm == CVC_COMM_OK))
        {
            self_test_pass_pending = FALSE;
            current_state = CVC_STATE_RUN;
            (void)BswM_RequestMode(CVC_ECU_ID_CVC, BSWM_RUN);
            VSM_DIAG("INIT -> RUN (heartbeats confirmed)");
        }
        /* else: remain in INIT — hold time or heartbeats not yet OK */
    }

    /* ---- Step 3: Derive events from signal values ---- */

    /* E-stop — highest priority, check first */
    if (estop_active != 0u)
    {
        Swc_VehicleState_OnEvent(CVC_EVT_ESTOP);
    }

    /* SC relay kill — second highest priority */
    if (sc_relay_kill != 0u)
    {
        Swc_VehicleState_OnEvent(CVC_EVT_SC_KILL);
    }

    /* CAN communication faults */
    if ((fzc_comm == CVC_COMM_TIMEOUT) && (rzc_comm == CVC_COMM_TIMEOUT))
    {
        Swc_VehicleState_OnEvent(CVC_EVT_CAN_TIMEOUT_DUAL);
    }
    else if ((fzc_comm == CVC_COMM_TIMEOUT) || (rzc_comm == CVC_COMM_TIMEOUT))
    {
        if ((current_state == CVC_STATE_RUN) || (current_state == CVC_STATE_DEGRADED))
        {
            Swc_VehicleState_OnEvent(CVC_EVT_CAN_TIMEOUT_SINGLE);
        }
    }
    else
    {
        /* Both comm OK — if in LIMP, signal CAN restored */
        if (current_state == CVC_STATE_LIMP)
        {
            Swc_VehicleState_OnEvent(CVC_EVT_CAN_RESTORED);
        }
    }

    /* Pedal faults — only derive events when in relevant states */
    if (current_state == CVC_STATE_RUN)
    {
        if (pedal_fault != 0u)
        {
            /* Any non-zero pedal fault in RUN -> single pedal fault event.
             * Dual pedal fault detection would be handled by the pedal SWC
             * which sets a specific fault code — for now treat as single. */
            Swc_VehicleState_OnEvent(CVC_EVT_PEDAL_FAULT_SINGLE);
        }
    }

    /* Fault cleared — when in DEGRADED and no pedal fault */
    if ((current_state == CVC_STATE_DEGRADED) && (pedal_fault == 0u))
    {
        Swc_VehicleState_OnEvent(CVC_EVT_FAULT_CLEARED);
    }

    /* ---- Step 4: Confirmed fault events (ISO 26262 confirmation-read) ---- */
    Swc_VehicleState_ConfirmFault(
        CVC_FAULT_IDX_MOTOR_CUTOFF, motor_cutoff,
        CVC_FAULT_COM_MOTOR_CUTOFF, CVC_FAULT_E2E_MOTOR_CUTOFF,
        CVC_DTC_MOTOR_CUTOFF_RX, CVC_EVT_MOTOR_CUTOFF);

    Swc_VehicleState_ConfirmFault(
        CVC_FAULT_IDX_BRAKE, brake_fault,
        CVC_FAULT_COM_BRAKE, CVC_FAULT_E2E_BRAKE,
        CVC_DTC_BRAKE_FAULT_RX, CVC_EVT_BRAKE_FAULT);

    Swc_VehicleState_ConfirmFault(
        CVC_FAULT_IDX_STEERING, steering_fault,
        CVC_FAULT_COM_STEERING, CVC_FAULT_E2E_STEERING,
        CVC_DTC_STEERING_FAULT_RX, CVC_EVT_STEERING_FAULT);

    /* ---- Step 5: SAFE_STOP recovery when all faults clear ---- */
    if (current_state == CVC_STATE_SAFE_STOP)
    {
        if ((estop_active == 0u) &&
            (motor_cutoff == 0u) &&
            (brake_fault == 0u) &&
            (steering_fault == 0u) &&
            (pedal_fault == 0u) &&
            (sc_relay_kill == 0u) &&
            (fzc_comm == CVC_COMM_OK) &&
            (rzc_comm == CVC_COMM_OK))
        {
            safe_stop_clear_count++;
            if (safe_stop_clear_count >= CVC_SAFE_STOP_RECOVERY_CYCLES)
            {
                safe_stop_clear_count  = 0u;
                current_state          = CVC_STATE_INIT;
                self_test_pass_pending = TRUE;
                (void)BswM_RequestMode(CVC_ECU_ID_CVC, BSWM_STARTUP);
            }
        }
        else
        {
            safe_stop_clear_count = 0u;
        }
    }

    /* ---- Step 6: Write current state to RTE ---- */
    (void)Rte_Write(CVC_SIG_VEHICLE_STATE, (uint32)current_state);

    /* ---- Step 7: Publish vehicle state to Com -> CAN 0x100 ---- */
    {
        uint8 tx_state = current_state;
        (void)Com_SendSignal(4u, &tx_state);  /* Signal 4 = vehicle_state */
    }
}
