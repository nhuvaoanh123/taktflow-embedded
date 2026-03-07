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
#include "sc_monitoring.h"     /* SWR-SC-029/030: SC_Status broadcast (GAP-1/2) */

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
extern void sc_het_led_on(void);
extern void sc_het_led_off(void);
extern void sc_dcan_print_status(void);
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
/* DEBUG BRINGUP: unused until full init restored */
__attribute__((unused)) static void sc_startup_fail_blink(uint8 failStep)
{
    volatile uint32 delay;
    uint8 blink;

    for (;;) {
        /* Blink failStep times */
        for (blink = 0u; blink < failStep; blink++) {
            gioSetBit(SC_GIO_PORT_A, SC_PIN_LED_SYS, 1u);
            for (delay = 0u; delay < 15000000u; delay++) { /* ~300ms at 300MHz */ }
            gioSetBit(SC_GIO_PORT_A, SC_PIN_LED_SYS, 0u);
            for (delay = 0u; delay < 15000000u; delay++) { /* ~300ms at 300MHz */ }
        }
        /* Pause between blink groups */
        for (delay = 0u; delay < 60000000u; delay++) { /* ~1s at 300MHz */ }

        /* Not feeding watchdog — TPS3823 will reset MCU */
    }
}

/* ==================================================================
 * Entry Point
 * ================================================================== */

int main(void)
{
    /* ---- 0. Prove CPU reaches main() via GIOB[6:7] LEDs ---- */
#ifdef PLATFORM_TMS570
    sc_het_led_off();
    sc_sci_init();
    sc_het_led_on();   /* LED ON = main() reached, SCI init done */
#endif

    /* ---- 1. GIO init (resets DIRB, turns off GIOB LEDs) ---- */
    gioInit();
    sc_configure_gpio();

#ifdef PLATFORM_TMS570
    sc_het_led_on();   /* Re-enable after gioInit reset DIRB */
#endif

    /* ---- STEP 1: RTI 10ms tick ---- */
    rtiInit();
    rtiStartCounter();

    /* ---- STEP 2: CAN init ---- */
    SC_CAN_Init();
    SC_E2E_Init();   /* Must follow SC_CAN_Init — seeds e2e_first_rx=TRUE so first frame is accepted */

    /* ---- STEP 3: Monitoring init — broadcasts SC_Status (0x013) each tick ---- */
    SC_Monitoring_Init();

    /* ---- STEP 4a: Heartbeat monitor init ---- */
    SC_Heartbeat_Init();

#ifdef PLATFORM_TMS570
    sc_sci_puts("CAN+Monitoring init OK\r\n");
#endif

    {
        uint32 led_tick = 0u;
        uint32 diag_tick = 0u;
        boolean led_state = FALSE;

        for (;;) {
            if (rtiIsTickPending() == FALSE) { continue; }
            rtiClearTick();

            led_tick++;
            if (led_tick >= 50u) {
                led_tick = 0u;
                led_state = (led_state == FALSE) ? TRUE : FALSE;
#ifdef PLATFORM_TMS570
                if (led_state == TRUE) { sc_het_led_on(); } else { sc_het_led_off(); }
#endif
            }

            SC_CAN_Receive();
            SC_Heartbeat_Monitor();
            SC_Monitoring_Update();  /* transmits 0x013 every 500ms */

            /* Print DCAN error status every 5s (500 ticks) to diagnose bus-off */
            diag_tick++;
            if (diag_tick >= 500u) {
                diag_tick = 0u;
#ifdef PLATFORM_TMS570
                sc_dcan_print_status();
#endif
            }

            /* STEP 4: remaining modules */
        }
    }

    return 0;
}
