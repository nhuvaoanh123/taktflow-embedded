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

/* ==================================================================
 * Module State
 * ================================================================== */

/** Relay kill latch — once TRUE, cannot be cleared (power cycle only) */
static boolean relay_killed;

/** Commanded relay state (TRUE = HIGH = energized) */
static boolean relay_commanded;

/** GPIO readback mismatch counter */
static uint8 readback_mismatch_count;

/* ==================================================================
 * Public API
 * ================================================================== */

void SC_Relay_Init(void)
{
    relay_killed            = FALSE;
    relay_commanded         = FALSE;
    readback_mismatch_count = 0u;

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

    /* Trigger (a): Heartbeat confirmed timeout */
    if (SC_Heartbeat_IsAnyConfirmed() == TRUE) {
        SC_Relay_DeEnergize();
        return;
    }

    /* Trigger (b): Plausibility fault */
    if (SC_Plausibility_IsFaulted() == TRUE) {
        SC_Relay_DeEnergize();
        return;
    }

    /* Trigger (c/d): Self-test failure (startup or runtime) */
    if (SC_SelfTest_IsHealthy() == FALSE) {
        SC_Relay_DeEnergize();
        return;
    }

    /* Trigger (e): ESM lockstep error */
    if (SC_ESM_IsErrorActive() == TRUE) {
        SC_Relay_DeEnergize();
        return;
    }

    /* Trigger (f): GPIO readback mismatch */
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
        SC_Relay_DeEnergize();
    }
}

boolean SC_Relay_IsKilled(void)
{
    return relay_killed;
}
