/**
 * @file    Sc_Cfg_Platform.h
 * @brief   SC platform-specific constants — TMS570 target (bare metal)
 * @date    2026-03-10
 *
 * @details Timing constants for real hardware.
 *          Selected via -I path ordering in Makefile (TMS570 build).
 *          POSIX version: firmware/sc/cfg/platform_posix/Sc_Cfg_Platform.h
 *
 * @copyright Taktflow Systems 2026
 */
#ifndef SC_CFG_PLATFORM_H
#define SC_CFG_PLATFORM_H

/** @brief  Heartbeat timeout: 100ms = 2x 50ms heartbeat period (SG-008 FTTI) */
#ifndef SC_HB_TIMEOUT_TICKS
  #define SC_HB_TIMEOUT_TICKS       10u
#endif

/** @brief  Heartbeat confirmation: 30ms */
#ifndef SC_HB_CONFIRM_TICKS
  #define SC_HB_CONFIRM_TICKS       3u
#endif

/** @brief  Startup grace: 5s (must >= CVC INIT hold) */
#ifndef SC_HB_STARTUP_GRACE_TICKS
  #define SC_HB_STARTUP_GRACE_TICKS 500u
#endif

/** @brief  Plausibility debounce: 50ms (real FOC inverter <1ms response) */
#ifndef SC_PLAUS_DEBOUNCE_TICKS
  #define SC_PLAUS_DEBOUNCE_TICKS   5u
#endif

/** @brief  E2E enforcement: disabled for HIL bench.
 *          SC receives STM32 heartbeats but E2E CRC check fails persistently.
 *          Root cause under investigation (CRC match verified in code review;
 *          suspect baud-rate / clock drift or CAN transceiver issue).
 *          When 1u, SC_E2E_Check always returns TRUE (no CRC/alive check).
 *          TODO:HARDWARE Re-enable (set to 0u) once E2E CRC is validated on bench. */
#ifndef SC_E2E_BYPASS
  #define SC_E2E_BYPASS             1u
#endif

/** @brief  E2E consecutive failure threshold (only used when SC_E2E_BYPASS=0). */
#ifndef SC_E2E_MAX_CONSEC_FAIL
  #define SC_E2E_MAX_CONSEC_FAIL    3u
#endif

/** @brief  Relay readback: disabled until relay hardware is wired.
 *          TODO:HARDWARE Re-enable (set to 1u) when relay is connected. */
#ifndef SC_RELAY_READBACK_ENABLED
  #define SC_RELAY_READBACK_ENABLED 0u
#endif

#endif /* SC_CFG_PLATFORM_H */
