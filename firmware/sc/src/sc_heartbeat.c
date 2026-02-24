/**
 * @file    sc_heartbeat.c
 * @brief   Per-ECU heartbeat monitoring for Safety Controller
 * @date    2026-02-23
 *
 * @safety_req SWR-SC-004, SWR-SC-005, SWR-SC-006
 * @traces_to  SSR-SC-004, SSR-SC-005, SSR-SC-006
 * @note    Safety level: ASIL D
 * @standard ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#include "sc_heartbeat.h"
#include "sc_cfg.h"
#include "sc_gio.h"

/* ==================================================================
 * Module State
 * ================================================================== */

/** Per-ECU heartbeat timeout counter (10ms ticks) */
static uint16 hb_counter[SC_ECU_COUNT];

/** Per-ECU timeout detected flag (>= 150ms) */
static boolean hb_timed_out[SC_ECU_COUNT];

/** Per-ECU confirmation counter (ticks after timeout detection) */
static uint8 hb_confirm_counter[SC_ECU_COUNT];

/** Per-ECU confirmed timeout flag (>= 200ms total) */
static boolean hb_confirmed[SC_ECU_COUNT];

/* ==================================================================
 * LED pin lookup by ECU index
 * ================================================================== */

static const uint8 ecu_led_pin[SC_ECU_COUNT] = {
    SC_PIN_LED_CVC,     /* GIO_A1 */
    SC_PIN_LED_FZC,     /* GIO_A2 */
    SC_PIN_LED_RZC      /* GIO_A3 */
};

/* ==================================================================
 * Public API
 * ================================================================== */

void SC_Heartbeat_Init(void)
{
    uint8 i;
    for (i = 0u; i < SC_ECU_COUNT; i++) {
        hb_counter[i]         = 0u;
        hb_timed_out[i]       = FALSE;
        hb_confirm_counter[i] = 0u;
        hb_confirmed[i]       = FALSE;
    }
}

void SC_Heartbeat_NotifyRx(uint8 ecuIndex)
{
    if (ecuIndex >= SC_ECU_COUNT) {
        return;
    }

    hb_counter[ecuIndex]         = 0u;
    hb_timed_out[ecuIndex]       = FALSE;
    hb_confirm_counter[ecuIndex] = 0u;
    /* Note: do NOT clear hb_confirmed here — once confirmed,
     * it stays confirmed until relay de-energize (power cycle).
     * But per plan, if heartbeat resumes during confirmation window,
     * we cancel. So we only clear confirmed if NOT yet confirmed. */
    if (hb_confirmed[ecuIndex] == FALSE) {
        /* Confirmation window canceled by resume */
    }
}

void SC_Heartbeat_Monitor(void)
{
    uint8 i;

    for (i = 0u; i < SC_ECU_COUNT; i++) {
        /* Skip if already confirmed (latched) */
        if (hb_confirmed[i] == TRUE) {
            continue;
        }

        /* Increment counter */
        if (hb_counter[i] < 0xFFFFu) {
            hb_counter[i]++;
        }

        /* Check for timeout threshold (150ms) */
        if (hb_counter[i] >= SC_HB_TIMEOUT_TICKS) {
            if (hb_timed_out[i] == FALSE) {
                /* First detection — enter confirmation window */
                hb_timed_out[i] = TRUE;
                hb_confirm_counter[i] = 0u;

                /* Drive fault LED HIGH immediately */
                gioSetBit(SC_GIO_PORT_A, ecu_led_pin[i], 1u);
            }

            /* Increment confirmation counter */
            hb_confirm_counter[i]++;

            /* Check confirmation threshold (50ms additional) */
            if (hb_confirm_counter[i] >= SC_HB_CONFIRM_TICKS) {
                hb_confirmed[i] = TRUE;
            }
        }
    }
}

boolean SC_Heartbeat_IsTimedOut(uint8 ecuIndex)
{
    if (ecuIndex >= SC_ECU_COUNT) {
        return FALSE;
    }
    return hb_timed_out[ecuIndex];
}

boolean SC_Heartbeat_IsAnyConfirmed(void)
{
    uint8 i;
    for (i = 0u; i < SC_ECU_COUNT; i++) {
        if (hb_confirmed[i] == TRUE) {
            return TRUE;
        }
    }
    return FALSE;
}

boolean SC_Heartbeat_IsFzcBrakeFault(void)
{
    /* TODO:POST-BETA — Parse FZC heartbeat payload byte 1 for brake fault flag.
     * Currently returns FALSE (no brake-fault detection via heartbeat data). */
    return FALSE;
}
