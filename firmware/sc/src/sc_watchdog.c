/**
 * @file    sc_watchdog.c
 * @brief   External watchdog (TPS3823) feed control for Safety Controller
 * @date    2026-02-23
 *
 * @safety_req SWR-SC-022
 * @traces_to  SSR-SC-015
 * @note    Safety level: ASIL D
 * @standard ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#include "sc_watchdog.h"
#include "sc_cfg.h"

/* ==================================================================
 * External: GIO access
 * ================================================================== */

extern void  gioSetBit(uint8 port, uint8 pin, uint8 value);
extern uint8 gioGetBit(uint8 port, uint8 pin);

/* ==================================================================
 * Module State
 * ================================================================== */

/** Current WDI pin toggle state */
static uint8 wdi_state;

/* ==================================================================
 * Public API
 * ================================================================== */

void SC_Watchdog_Init(void)
{
    wdi_state = 0u;
    gioSetBit(SC_GIO_PORT_A, SC_PIN_WDI, 0u);
}

void SC_Watchdog_Feed(boolean allChecksOk)
{
    if (allChecksOk == TRUE) {
        /* Toggle WDI pin — TPS3823 requires periodic toggling */
        wdi_state ^= 1u;
        gioSetBit(SC_GIO_PORT_A, SC_PIN_WDI, wdi_state);
    }
    /* If allChecksOk is FALSE, do NOT toggle — watchdog will starve
     * and TPS3823 will assert RESET after timeout */
}
