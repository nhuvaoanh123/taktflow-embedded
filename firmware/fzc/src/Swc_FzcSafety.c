/**
 * @file    Swc_FzcSafety.c
 * @brief   FZC local safety monitoring — watchdog feed, fault aggregation, self-test
 * @date    2026-02-23
 *
 * @safety_req SWR-FZC-023, SWR-FZC-025
 * @traces_to  SSR-FZC-023, SSR-FZC-025, TSR-046
 *
 * @details  Implements the FZC safety monitoring SWC:
 *           1. Watchdog feed (TPS3823 WDI toggle on PB0) with 4-condition check
 *           2. Local plausibility aggregation from steering, brake, lidar faults
 *           3. Combined fault bitmask written to RTE
 *           4. DTC reporting for watchdog failures
 *
 *           Watchdog feed conditions (all must be true):
 *           - Main loop is executing (this function called)
 *           - No critical fault latched (steering or brake)
 *           - Vehicle state is not SHUTDOWN
 *           - Self-test passed (if completed)
 *
 *           All variables are static file-scope. No dynamic memory.
 *
 * @standard AUTOSAR SWC pattern, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */

#include "Swc_FzcSafety.h"
#include "Fzc_Cfg.h"

/* ==================================================================
 * External Dependencies
 * ================================================================== */

extern Std_ReturnType Rte_Read(uint16 SignalId, uint32* DataPtr);
extern Std_ReturnType Rte_Write(uint16 SignalId, uint32 Data);
extern void           Dio_WriteChannel(uint8 Channel, uint8 Level);
extern void           Dem_ReportErrorStatus(uint8 EventId, uint8 EventStatus);

/* ==================================================================
 * Constants
 * ================================================================== */

#define DEM_EVENT_STATUS_FAILED  1u

#define SAFETY_WDI_CHANNEL     0u    /* PB0 — TPS3823 WDI pin */

#define SAFETY_STATUS_OK       0u
#define SAFETY_STATUS_DEGRADED 1u
#define SAFETY_STATUS_FAULT    2u

/* ==================================================================
 * Module State
 * ================================================================== */

static uint8   Safety_Initialized;
static uint8   Safety_WdiToggle;     /* Alternates 0/1 for WDI pin */
static uint8   Safety_Status;
static uint8   Safety_SelfTestDone;

/* ==================================================================
 * API: Swc_FzcSafety_Init
 * ================================================================== */

void Swc_FzcSafety_Init(void)
{
    Safety_WdiToggle    = 0u;
    Safety_Status       = SAFETY_STATUS_OK;
    Safety_SelfTestDone = FALSE;
    Safety_Initialized  = TRUE;
}

/* ==================================================================
 * API: Swc_FzcSafety_MainFunction (10ms cyclic)
 * ================================================================== */

void Swc_FzcSafety_MainFunction(void)
{
    uint32 steer_fault;
    uint32 brake_fault;
    uint32 lidar_fault;
    uint32 vehicle_state;
    uint32 self_test_result;
    uint8  fault_mask;
    uint8  wdg_feed_ok;

    if (Safety_Initialized != TRUE) {
        return;
    }

    /* ----------------------------------------------------------
     * Step 1: Read fault signals from RTE
     * ---------------------------------------------------------- */
    steer_fault = 0u;
    brake_fault = 0u;
    lidar_fault = 0u;
    vehicle_state = FZC_STATE_INIT;
    self_test_result = FZC_SELF_TEST_PASS;

    (void)Rte_Read(FZC_SIG_STEER_FAULT, &steer_fault);
    (void)Rte_Read(FZC_SIG_BRAKE_FAULT, &brake_fault);
    (void)Rte_Read(FZC_SIG_LIDAR_FAULT, &lidar_fault);
    (void)Rte_Read(FZC_SIG_VEHICLE_STATE, &vehicle_state);
    (void)Rte_Read(FZC_SIG_SELF_TEST_RESULT, &self_test_result);

    /* ----------------------------------------------------------
     * Step 2: Aggregate fault bitmask
     * ---------------------------------------------------------- */
    fault_mask = FZC_FAULT_NONE;

    if (steer_fault != 0u) {
        fault_mask |= FZC_FAULT_STEER;
    }

    if (brake_fault != 0u) {
        fault_mask |= FZC_FAULT_BRAKE;
    }

    if (lidar_fault != 0u) {
        fault_mask |= FZC_FAULT_LIDAR;
    }

    if (self_test_result == FZC_SELF_TEST_FAIL) {
        fault_mask |= FZC_FAULT_SELF_TEST;
    }

    /* ----------------------------------------------------------
     * Step 3: Determine safety status
     * ---------------------------------------------------------- */
    if ((fault_mask & (FZC_FAULT_STEER | FZC_FAULT_BRAKE)) != 0u) {
        Safety_Status = SAFETY_STATUS_FAULT;
    } else if (fault_mask != FZC_FAULT_NONE) {
        Safety_Status = SAFETY_STATUS_DEGRADED;
    } else {
        Safety_Status = SAFETY_STATUS_OK;
    }

    /* ----------------------------------------------------------
     * Step 4: Watchdog feed — 4 conditions must all be true
     * ---------------------------------------------------------- */
    wdg_feed_ok = TRUE;

    /* Condition 1: No critical fault (steering or brake) */
    if ((fault_mask & (FZC_FAULT_STEER | FZC_FAULT_BRAKE)) != 0u) {
        wdg_feed_ok = FALSE;
    }

    /* Condition 2: Vehicle not in SHUTDOWN */
    if (vehicle_state == FZC_STATE_SHUTDOWN) {
        wdg_feed_ok = FALSE;
    }

    /* Condition 3: Self-test passed (or not yet run) */
    if (self_test_result == FZC_SELF_TEST_FAIL) {
        wdg_feed_ok = FALSE;
    }

    /* Condition 4: Main loop executing (implicit — we are here) */

    if (wdg_feed_ok == TRUE) {
        /* Toggle WDI pin */
        Safety_WdiToggle = (uint8)(Safety_WdiToggle ^ 1u);
        Dio_WriteChannel(SAFETY_WDI_CHANNEL, Safety_WdiToggle);
    } else {
        /* Do not feed watchdog — will trigger HW reset */
        Dem_ReportErrorStatus(FZC_DTC_WATCHDOG_FAIL, DEM_EVENT_STATUS_FAILED);
        fault_mask |= FZC_FAULT_WATCHDOG;
    }

    /* ----------------------------------------------------------
     * Step 5: Write outputs to RTE
     * ---------------------------------------------------------- */
    (void)Rte_Write(FZC_SIG_FAULT_MASK, (uint32)fault_mask);
    (void)Rte_Write(FZC_SIG_SAFETY_STATUS, (uint32)Safety_Status);
}

/* ==================================================================
 * API: Swc_FzcSafety_GetStatus
 * ================================================================== */

uint8 Swc_FzcSafety_GetStatus(void)
{
    return Safety_Status;
}
