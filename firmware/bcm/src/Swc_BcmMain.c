/**
 * @file    Swc_BcmMain.c
 * @brief   BCM main loop — 10ms cycle, CAN processing, periodic transmit
 * @date    2026-02-24
 *
 * @safety_req SWR-BCM-012
 * @traces_to  SSR-BCM-012, TSR-046, TSR-047, TSR-048
 *
 * @details  10ms cooperative main loop for the Body Control Module.
 *           Each tick: reads CAN state + commands, processes SWC logic,
 *           and transmits body status every 10th cycle (100ms).
 *           Detects and logs cycle overrun conditions.
 *
 * @standard AUTOSAR, ISO 26262 Part 6 (QM)
 * @copyright Taktflow Systems 2026
 */
#include "Swc_BcmMain.h"

/* ====================================================================
 * Platform Abstraction (real or mock)
 * ==================================================================== */

#ifdef BCM_MAIN_USE_MOCK

extern Std_ReturnType BCM_CAN_ReceiveState(void);
extern Std_ReturnType BCM_CAN_ReceiveCommand(void);
extern Std_ReturnType BCM_CAN_TransmitStatus(void);
extern uint32         mock_get_tick_ms(void);
extern void           mock_log_overrun(uint32 duration_ms);

#define GET_TICK_MS()            mock_get_tick_ms()
#define LOG_OVERRUN(duration)    mock_log_overrun((duration))

#else /* Real implementation */

#include "Swc_BcmCan.h"
#include <stdio.h>
#include <time.h>

static uint32 real_get_tick_ms(void)
{
    struct timespec ts;
    (void)clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32)((ts.tv_sec * 1000u) + (ts.tv_nsec / 1000000u));
}

#define GET_TICK_MS()  real_get_tick_ms()
#define LOG_OVERRUN(duration)  (void)printf("[BCM] WARN: Cycle overrun %lu ms\n", (unsigned long)(duration))

#endif /* BCM_MAIN_USE_MOCK */

/* ====================================================================
 * Internal Constants
 * ==================================================================== */

#ifndef BCM_MAIN_CYCLE_MS
#define BCM_MAIN_CYCLE_MS       10u
#endif
#ifndef BCM_TX_EVERY_N_CYCLES
#define BCM_TX_EVERY_N_CYCLES   10u
#endif

/* ====================================================================
 * Module State
 * ==================================================================== */

static boolean bcm_main_initialized;
static uint16  bcm_main_cycle_counter;
static uint32  bcm_main_prev_tick_ms;

/* ====================================================================
 * Public API
 * ==================================================================== */

/**
 * @brief  BCM main loop tick — called every 10ms
 *
 * @safety_req SWR-BCM-012
 * @details  Processing order:
 *   1. Read vehicle state (CAN 0x100)
 *   2. Read body control command (CAN 0x350)
 *   3. Transmit body status (CAN 0x360) every 10th cycle
 *   4. Check for cycle overrun and log warning
 */
void BCM_Main(void)
{
    uint32 cycle_start;
    uint32 cycle_end;
    uint32 elapsed;

    if (bcm_main_initialized == FALSE) {
        return;
    }

    cycle_start = GET_TICK_MS();

    /* Check for overrun from previous cycle */
    if (bcm_main_prev_tick_ms != 0u) {
        elapsed = cycle_start - bcm_main_prev_tick_ms;
        if (elapsed > BCM_MAIN_CYCLE_MS) {
            LOG_OVERRUN(elapsed);
        }
    }

    /* ---- Step 1: CAN Read — vehicle state ---- */
    (void)BCM_CAN_ReceiveState();

    /* ---- Step 2: CAN Read — body control command ---- */
    (void)BCM_CAN_ReceiveCommand();

    /* ---- Step 3: Increment cycle counter ---- */
    bcm_main_cycle_counter++;

    /* ---- Step 4: Transmit every 10th cycle (100ms) ---- */
    if (bcm_main_cycle_counter >= BCM_TX_EVERY_N_CYCLES) {
        (void)BCM_CAN_TransmitStatus();
        bcm_main_cycle_counter = 0u;
    }

    /* Record end time for overrun detection */
    cycle_end = GET_TICK_MS();
    bcm_main_prev_tick_ms = cycle_end;
}
