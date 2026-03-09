/**
 * @file    sc_relay.c
 * @brief   Kill relay GPIO control for Safety Controller
 * @date    2026-02-23
 *
 * @safety_req SWR-SC-010, SWR-SC-011, SWR-SC-012
 * @traces_to  SSR-SC-010, SSR-SC-011, SSR-SC-012
 * @note    Safety level: ASIL D
 * @standard ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#include "sc_relay.h"
#include "sc_cfg.h"
#include "sc_gio.h"
#include "sc_heartbeat.h"
#include "sc_plausibility.h"
#include "sc_selftest.h"
#include "sc_esm.h"
#include "sc_can.h"

/* SIL diagnostic logging — compile with -DSIL_DIAG to enable */
#ifdef SIL_DIAG
#include <stdio.h>  /* cppcheck-suppress misra-c2012-21.6 ; SIL-only diagnostic output */
#define SC_RELAY_DIAG(fmt, ...) (void)fprintf(stderr, "[SC_RELAY] " fmt "\n", ##__VA_ARGS__)
#else
#define SC_RELAY_DIAG(fmt, ...) ((void)0)
#endif

/* ==================================================================
 * Module State
 * ================================================================== */

/** Relay kill latch — once TRUE, cannot be cleared (power cycle only) */
static boolean relay_killed;

/** Commanded relay state (TRUE = HIGH = energized) */
static boolean relay_commanded;

/** GPIO readback mismatch counter */
static uint8 readback_mismatch_count;

/** Kill reason code (SC_KILL_REASON_*) */
static uint8 kill_reason;

#ifdef PLATFORM_POSIX
/** SIL broadcast tick counter */
static uint8 broadcast_tick_count;

/* Forward declaration for POSIX CAN send */
extern void sc_posix_can_send(uint32 can_id, const uint8 *data, uint8 dlc);
#endif

/* ==================================================================
 * Public API
 * ================================================================== */

void SC_Relay_Init(void)
{
    /* Kill latch is NOT cleared — once killed, only a power cycle resets it.
     * This ensures the kill latch survives re-initialization (SWR-SC-011). */
    relay_commanded         = FALSE;
    readback_mismatch_count = 0u;
    kill_reason             = SC_KILL_REASON_NONE;
#ifdef PLATFORM_POSIX
    broadcast_tick_count    = 0u;
#endif

    /* Ensure relay is de-energized (safe state) */
    gioSetBit(SC_GIO_PORT_A, SC_PIN_RELAY, 0u);
}

void SC_Relay_Energize(void)
{
    /* Kill latch prevents re-energize */
    if (relay_killed == TRUE) {
        return;
    }

    relay_commanded = TRUE;
    gioSetBit(SC_GIO_PORT_A, SC_PIN_RELAY, 1u);
}

void SC_Relay_DeEnergize(void)
{
    relay_commanded = FALSE;
    relay_killed    = TRUE;
    gioSetBit(SC_GIO_PORT_A, SC_PIN_RELAY, 0u);
}

void SC_Relay_CheckTriggers(void)
{
    uint8 readback;

    /* If already killed, nothing to check */
    if (relay_killed == TRUE) {
        return;
    }

    /* Trigger (a): E-Stop command (highest priority — explicit safety request) */
    if (SC_CAN_IsEStopActive() == TRUE) {
        kill_reason = SC_KILL_REASON_ESTOP;
        SC_RELAY_DIAG("KILL reason=ESTOP");
        SC_Relay_DeEnergize();
        return;
    }

    /* Trigger (b): Heartbeat confirmed timeout */
    if (SC_Heartbeat_IsAnyConfirmed() == TRUE) {
        kill_reason = SC_KILL_REASON_HB_TIMEOUT;
        SC_RELAY_DIAG("KILL reason=HB_TIMEOUT cvc=%u fzc=%u rzc=%u",
                      (unsigned)SC_Heartbeat_IsTimedOut(SC_ECU_CVC),
                      (unsigned)SC_Heartbeat_IsTimedOut(SC_ECU_FZC),
                      (unsigned)SC_Heartbeat_IsTimedOut(SC_ECU_RZC));
        SC_Relay_DeEnergize();
        return;
    }

    /* Trigger (b): Plausibility fault */
    if (SC_Plausibility_IsFaulted() == TRUE) {
        kill_reason = SC_KILL_REASON_PLAUSIBILITY;
        SC_RELAY_DIAG("KILL reason=PLAUSIBILITY");
        SC_Relay_DeEnergize();
        return;
    }

    /* Trigger (c/d): Self-test failure (startup or runtime) */
    if (SC_SelfTest_IsHealthy() == FALSE) {
        kill_reason = SC_KILL_REASON_SELFTEST;
        SC_RELAY_DIAG("KILL reason=SELFTEST");
        SC_Relay_DeEnergize();
        return;
    }

    /* Trigger (e): ESM lockstep error */
    if (SC_ESM_IsErrorActive() == TRUE) {
        kill_reason = SC_KILL_REASON_ESM;
        SC_RELAY_DIAG("KILL reason=ESM");
        SC_Relay_DeEnergize();
        return;
    }

    /* Trigger (f): CAN bus-off */
    if (SC_CAN_IsBusOff() == TRUE) {
        kill_reason = SC_KILL_REASON_BUSOFF;
        SC_RELAY_DIAG("KILL reason=BUSOFF");
        SC_Relay_DeEnergize();
        return;
    }

    /* Trigger (g): CAN bus silence — no valid frames for >= 200ms */
    if (SC_CAN_IsBusSilent() == TRUE) {
        kill_reason = SC_KILL_REASON_BUS_SILENCE;
        SC_RELAY_DIAG("KILL reason=BUS_SILENCE");
        SC_Relay_DeEnergize();
        return;
    }

    /* Trigger (h): GPIO readback mismatch */
    readback = gioGetBit(SC_GIO_PORT_A, SC_PIN_RELAY);
    if (relay_commanded == TRUE) {
        if (readback != 1u) {
            readback_mismatch_count++;
        } else {
            readback_mismatch_count = 0u;
        }
    } else {
        if (readback != 0u) {
            readback_mismatch_count++;
        } else {
            readback_mismatch_count = 0u;
        }
    }

    if (readback_mismatch_count >= SC_RELAY_READBACK_THRESHOLD) {
        kill_reason = SC_KILL_REASON_READBACK;
        SC_RELAY_DIAG("KILL reason=READBACK cmd=%u rb=%u cnt=%u",
                      (unsigned)relay_commanded, (unsigned)readback,
                      (unsigned)readback_mismatch_count);
        SC_Relay_DeEnergize();
    }
}

boolean SC_Relay_IsKilled(void)
{
    return relay_killed;
}

uint8 SC_Relay_GetKillReason(void)
{
    return kill_reason;
}

#ifdef PLATFORM_POSIX
void SC_Relay_BroadcastSil(void)
{
    uint8 payload[4];

    broadcast_tick_count++;
    if (broadcast_tick_count < SC_RELAY_BROADCAST_TICKS) {
        return;
    }
    broadcast_tick_count = 0u;

    payload[0] = (relay_killed == TRUE) ? 1u : 0u;
    payload[1] = kill_reason;
    payload[2] = SC_FAULT_SOURCE_NONE;  /* TODO: derive from heartbeat module */
    payload[3] = 0u;                     /* reserved */

    sc_posix_can_send(SC_CAN_ID_RELAY_STATUS, payload, SC_RELAY_STATUS_DLC);
}
#endif
