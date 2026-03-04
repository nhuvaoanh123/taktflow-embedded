/**
 * @file    sc_main.c
 * @brief   Safety Controller entry point and 10ms cooperative main loop
 * @date    2026-02-23
 *
 * @details Initializes all SC modules, runs 7-step startup self-test,
 *          then enters a 10ms cooperative loop driven by RTI tick timer.
 *          Each iteration: CAN receive, heartbeat monitor, plausibility
 *          check, relay trigger evaluation, LED update, runtime self-test,
 *          stack canary check, and conditional watchdog feed.
 *
 * @safety_req SWR-SC-025, SWR-SC-026
 * @traces_to  SSR-SC-001 to SSR-SC-017
 * @note    Safety level: ASIL D
 * @standard ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#include "sc_types.h"
#include "sc_cfg.h"
#include "sc_e2e.h"
#include "sc_can.h"
#include "sc_heartbeat.h"
#include "sc_plausibility.h"
#include "sc_relay.h"
#include "sc_led.h"
#include "sc_watchdog.h"
#include "sc_esm.h"
#include "sc_selftest.h"
#include "sc_gio.h"

/* ==================================================================
 * External: Platform initialization (HALCoGen-generated or mock)
 * ================================================================== */

/** Initialize system clocks (PLL to 300 MHz) */
extern void systemInit(void);

/** Initialize GIO module (pin directions, defaults) */
extern void gioInit(void);

/** Initialize RTI timer for 10ms tick */
extern void rtiInit(void);

/** Start RTI counter */
extern void rtiStartCounter(void);

/** Check if RTI tick flag is set (10ms elapsed) */
extern boolean rtiIsTickPending(void);

/** Clear RTI tick flag */
extern void rtiClearTick(void);

/** Set GIO pin direction (0=input, 1=output) */
extern void gioSetDirection(uint8 port, uint8 pin, uint8 direction);

#ifdef PLATFORM_TMS570
/* ==================================================================
 * External: SCI debug UART (from sc_hw_tms570.c)
 * ================================================================== */

extern void sc_sci_init(void);
extern void sc_sci_puts(const char* str);
extern void sc_sci_put_uint(uint32 val);
#endif

/* ==================================================================
 * Internal: Configure GIO pins
 * ================================================================== */

static void sc_configure_gpio(void)
{
    /* Port A outputs: A0=relay, A1=LED_CVC, A2=LED_FZC, A3=LED_RZC, A4=LED_SYS, A5=WDI */
    gioSetDirection(SC_GIO_PORT_A, SC_PIN_RELAY,   1u);
    gioSetDirection(SC_GIO_PORT_A, SC_PIN_LED_CVC, 1u);
    gioSetDirection(SC_GIO_PORT_A, SC_PIN_LED_FZC, 1u);
    gioSetDirection(SC_GIO_PORT_A, SC_PIN_LED_RZC, 1u);
    gioSetDirection(SC_GIO_PORT_A, SC_PIN_LED_SYS, 1u);
    gioSetDirection(SC_GIO_PORT_A, SC_PIN_WDI,     1u);

    /* Port B output: B1=Heartbeat LED */
    gioSetDirection(SC_GIO_PORT_B, SC_PIN_LED_HB, 1u);

    /* All pins LOW initially */
    gioSetBit(SC_GIO_PORT_A, SC_PIN_RELAY,   0u);
    gioSetBit(SC_GIO_PORT_A, SC_PIN_LED_CVC, 0u);
    gioSetBit(SC_GIO_PORT_A, SC_PIN_LED_FZC, 0u);
    gioSetBit(SC_GIO_PORT_A, SC_PIN_LED_RZC, 0u);
    gioSetBit(SC_GIO_PORT_A, SC_PIN_LED_SYS, 0u);
    gioSetBit(SC_GIO_PORT_A, SC_PIN_WDI,     0u);
    gioSetBit(SC_GIO_PORT_B, SC_PIN_LED_HB,  0u);
}

/* ==================================================================
 * Internal: Startup failure blink pattern
 * ================================================================== */

/**
 * @brief  Blink system LED at step-number rate, then halt
 *
 * Watchdog will eventually reset the MCU since we stop feeding WDI.
 *
 * @param  failStep  Step number that failed (1-7)
 */
static void sc_startup_fail_blink(uint8 failStep)
{
    volatile uint32 delay;
    uint8 blink;

    for (;;) {
        /* Blink failStep times */
        for (blink = 0u; blink < failStep; blink++) {
            gioSetBit(SC_GIO_PORT_A, SC_PIN_LED_SYS, 1u);
            for (delay = 0u; delay < 500000u; delay++) { /* ~100ms */ }
            gioSetBit(SC_GIO_PORT_A, SC_PIN_LED_SYS, 0u);
            for (delay = 0u; delay < 500000u; delay++) { /* ~100ms */ }
        }
        /* Pause between blink groups */
        for (delay = 0u; delay < 2500000u; delay++) { /* ~500ms */ }

        /* Not feeding watchdog — TPS3823 will reset MCU */
    }
}

/* ==================================================================
 * Entry Point
 * ================================================================== */

int main(void)
{
    uint8 startup_result;
    boolean all_checks_ok;
#ifdef PLATFORM_TMS570
    uint16 dbg_tick_counter = 0u;  /* 5s periodic debug print */
#endif

    /* ---- 1. System initialization ---- */
    systemInit();           /* PLL to 300 MHz */
    gioInit();              /* GIO module init */
    sc_configure_gpio();    /* SC-specific pin config */
    rtiInit();              /* RTI 10ms tick timer */

#ifdef PLATFORM_TMS570
    /* ---- 1b. Debug UART init ---- */
    sc_sci_init();
    sc_sci_puts("=== SC Boot (TMS570LC43x) ===\r\n");
#endif

    /* ---- 2. Module initialization ---- */
    SC_E2E_Init();
    SC_CAN_Init();
    SC_Heartbeat_Init();
    SC_Plausibility_Init();
    SC_Relay_Init();
    SC_LED_Init();
    SC_Watchdog_Init();
    SC_ESM_Init();
    SC_SelfTest_Init();

#ifdef PLATFORM_TMS570
    sc_sci_puts("Modules init: 9 OK\r\n");
#endif

    /* ---- 3. Startup self-test (7 steps) ---- */
    startup_result = SC_SelfTest_Startup();

#ifdef PLATFORM_TMS570
    sc_sci_puts("BIST: ");
    if (startup_result == 0u) {
        sc_sci_puts("7/7 PASS\r\n");
    } else {
        sc_sci_puts("FAIL at step ");
        sc_sci_put_uint((uint32)startup_result);
        sc_sci_puts("\r\n");
    }
#endif

    if (startup_result != 0u) {
        /* Startup failed — blink failure pattern and halt */
        sc_startup_fail_blink(startup_result);
        /* Never returns — watchdog will reset */
    }

    /* ---- 4. Startup passed — energize relay ---- */
    SC_Relay_Energize();

#ifdef PLATFORM_TMS570
    sc_sci_puts("SC_Relay: energized (MONITORING)\r\n");
#endif

    /* ---- 5. Start RTI timer and enter main loop ---- */
    rtiStartCounter();

    for (;;) {
        /* Wait for 10ms RTI tick */
        if (rtiIsTickPending() == FALSE) {
            continue;
        }
        rtiClearTick();

        /* ---- Step 1: CAN Receive ---- */
        SC_CAN_Receive();

        /* ---- Step 2: Heartbeat Monitor ---- */
        SC_Heartbeat_Monitor();

        /* ---- Step 3: Plausibility Check ---- */
        SC_Plausibility_Check();

        /* ---- Step 4: Relay Trigger Evaluation ---- */
        SC_Relay_CheckTriggers();

#ifdef PLATFORM_POSIX
        /* ---- Step 4b: SIL Relay Status Broadcast (50ms) ---- */
        SC_Relay_BroadcastSil();
#endif

        /* ---- Step 5: LED Update ---- */
        SC_LED_Update();

        /* ---- Step 6: Bus Silence Monitor ---- */
        SC_CAN_MonitorBus();

        /* ---- Step 7: Runtime Self-Test (1 step) ---- */
        SC_SelfTest_Runtime();

        /* ---- Step 8: Stack Canary Check ---- */
        /* Must be BEFORE watchdog feed */
        all_checks_ok = TRUE;

        if (SC_SelfTest_StackCanaryOk() == FALSE) {
            all_checks_ok = FALSE;
        }

        if (SC_SelfTest_IsHealthy() == FALSE) {
            all_checks_ok = FALSE;
        }

        if (SC_CAN_IsBusOff() == TRUE) {
            all_checks_ok = FALSE;
        }

        if (SC_ESM_IsErrorActive() == TRUE) {
            all_checks_ok = FALSE;
        }

#ifdef PLATFORM_TMS570
        /* ---- Step 8b: Periodic debug status (every 5 seconds) ---- */
        dbg_tick_counter++;
        if (dbg_tick_counter >= 500u) {  /* 500 * 10ms = 5s */
            dbg_tick_counter = 0u;
            sc_sci_puts("[5s] SC: CVC=");
            sc_sci_puts(SC_Heartbeat_IsTimedOut(SC_ECU_CVC) ? "TIMEOUT" : "OK");
            sc_sci_puts(" FZC=");
            sc_sci_puts(SC_Heartbeat_IsTimedOut(SC_ECU_FZC) ? "TIMEOUT" : "OK");
            sc_sci_puts(" RZC=");
            sc_sci_puts(SC_Heartbeat_IsTimedOut(SC_ECU_RZC) ? "TIMEOUT" : "OK");
            sc_sci_puts(" relay=");
            sc_sci_puts((SC_Relay_IsKilled() == FALSE) ? "ON" : "OFF");
            sc_sci_puts("\r\n");
        }
#endif

        /* ---- Step 9: Watchdog Feed ---- */
        SC_Watchdog_Feed(all_checks_ok);
    }

    /* Should never reach here */
    return 0;
}
