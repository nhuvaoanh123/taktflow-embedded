/**
 * @file    Sc_Cfg_Platform.h
 * @brief   SC platform-specific constants — POSIX (SIL/Docker)
 * @date    2026-03-10
 *
 * @details Timing constants tuned for Docker SIL simulation.
 *          Selected via -I path ordering in Makefile.posix.
 *          Target version: firmware/sc/cfg/platform_target/Sc_Cfg_Platform.h
 *
 * @copyright Taktflow Systems 2026
 */
#ifndef SC_CFG_PLATFORM_H
#define SC_CFG_PLATFORM_H

/** @brief  Heartbeat timeout: 5s virtual / 500ms wall at scale=10.
 *          CI shared runners have 50-200ms Docker scheduling jitter. */
#ifndef SC_HB_TIMEOUT_TICKS
  #define SC_HB_TIMEOUT_TICKS       500u
#endif

/** @brief  Heartbeat confirmation: 1s virtual / 100ms wall at scale=10 */
#ifndef SC_HB_CONFIRM_TICKS
  #define SC_HB_CONFIRM_TICKS       100u
#endif

/** @brief  Startup grace: 30s virtual / 3s wall (sequential Docker restarts) */
#ifndef SC_HB_STARTUP_GRACE_TICKS
  #define SC_HB_STARTUP_GRACE_TICKS 3000u
#endif

/** @brief  Plausibility debounce: 300ms virtual / 30ms wall */
#ifndef SC_PLAUS_DEBOUNCE_TICKS
  #define SC_PLAUS_DEBOUNCE_TICKS   30u
#endif

/** @brief  Bus silence: 2s virtual / 200ms wall.
 *          Overrides sc_cfg.h default (20 ticks = 20ms wall — too tight for CI). */
#ifndef SC_BUS_SILENCE_TICKS
  #define SC_BUS_SILENCE_TICKS      200u
#endif

/** @brief  E2E consecutive failure threshold: 10 (vs 3 on target).
 *          CI CAN delivery jitter causes alive counter mismatches. */
#ifndef SC_E2E_MAX_CONSEC_FAIL
  #define SC_E2E_MAX_CONSEC_FAIL    10u
#endif

#endif /* SC_CFG_PLATFORM_H */
