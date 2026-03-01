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
#include "Cvc_Cfg.h"
#include "Rte.h"
#include "BswM.h"
#include "Dem.h"
#include "Com.h"

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

/* ==================================================================
 * Public API
 * ================================================================== */

/**
 * @brief  Initialize the vehicle state machine
 * @safety_req SWR-CVC-009
 */
void Swc_VehicleState_Init(void)
{
    current_state        = CVC_STATE_INIT;
    initialized          = TRUE;
    self_test_pass_pending = FALSE;
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
    current_state = next_state;

    /* Notify BswM of the new mode */
    (void)BswM_RequestMode(CVC_ECU_ID_CVC, state_to_bswm_mode[current_state]);
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

    /* ---- Step 2: Handle pending self-test pass (heartbeat guard) ---- */
    if ((self_test_pass_pending == TRUE) && (current_state == CVC_STATE_INIT))
    {
        if ((fzc_comm == CVC_COMM_OK) && (rzc_comm == CVC_COMM_OK))
        {
            self_test_pass_pending = FALSE;
            current_state = CVC_STATE_RUN;
            (void)BswM_RequestMode(CVC_ECU_ID_CVC, BSWM_RUN);
        }
        /* else: remain in INIT, keep pending — heartbeats not yet OK */
    }

    /* ---- Step 3: Derive events from signal values ---- */

    /* E-stop — highest priority, check first */
    if (estop_active != 0u)
    {
        Swc_VehicleState_OnEvent(CVC_EVT_ESTOP);
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

    /* ---- Step 4: Report DTCs and derive events for subsystem faults ---- */
    if (motor_cutoff != 0u)
    {
        Dem_ReportErrorStatus(CVC_DTC_MOTOR_CUTOFF_RX, DEM_EVENT_STATUS_FAILED);
        Swc_VehicleState_OnEvent(CVC_EVT_MOTOR_CUTOFF);
    }

    if (brake_fault != 0u)
    {
        Dem_ReportErrorStatus(CVC_DTC_BRAKE_FAULT_RX, DEM_EVENT_STATUS_FAILED);
        Swc_VehicleState_OnEvent(CVC_EVT_BRAKE_FAULT);
    }

    if (steering_fault != 0u)
    {
        Dem_ReportErrorStatus(CVC_DTC_STEERING_FAULT_RX, DEM_EVENT_STATUS_FAILED);
        Swc_VehicleState_OnEvent(CVC_EVT_STEERING_FAULT);
    }

    /* ---- Step 5: Write current state to RTE ---- */
    (void)Rte_Write(CVC_SIG_VEHICLE_STATE, (uint32)current_state);

    /* ---- Step 6: Publish vehicle state to Com -> CAN 0x100 ---- */
    {
        uint8 tx_state = current_state;
        (void)Com_SendSignal(4u, &tx_state);  /* Signal 4 = vehicle_state */
    }
}
