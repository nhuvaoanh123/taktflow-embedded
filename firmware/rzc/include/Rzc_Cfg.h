/**
 * @file    Rzc_Cfg.h
 * @brief   RZC configuration — all RZC-specific ID definitions
 * @date    2026-02-23
 *
 * @details Unified configuration header for the Rear Zone Controller.
 *          Contains RTE signal IDs, Com PDU IDs, DTC event IDs, E2E data IDs,
 *          motor constants, current constants, temperature constants, encoder
 *          constants, battery constants, heartbeat constants, and self-test
 *          constants.
 *
 * @safety_req SWR-RZC-001 to SWR-RZC-030
 * @traces_to  SSR-RZC-001 to SSR-RZC-017, TSR-022, TSR-030, TSR-038, TSR-046
 *
 * @standard AUTOSAR, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#ifndef RZC_CFG_H
#define RZC_CFG_H

/* ====================================================================
 * RTE Signal IDs (extends BSW well-known IDs at offset 16)
 * ==================================================================== */

#define RZC_SIG_TORQUE_CMD         16u
#define RZC_SIG_TORQUE_ECHO        17u
#define RZC_SIG_MOTOR_SPEED        18u
#define RZC_SIG_MOTOR_DIR          19u
#define RZC_SIG_MOTOR_ENABLE       20u
#define RZC_SIG_MOTOR_FAULT        21u
#define RZC_SIG_CURRENT_MA         22u
#define RZC_SIG_OVERCURRENT        23u
#define RZC_SIG_TEMP1_DC           24u
#define RZC_SIG_TEMP2_DC           25u
#define RZC_SIG_DERATING_PCT       26u
#define RZC_SIG_TEMP_FAULT         27u
#define RZC_SIG_BATTERY_MV         28u
#define RZC_SIG_BATTERY_STATUS     29u
#define RZC_SIG_ENCODER_SPEED      30u
#define RZC_SIG_ENCODER_DIR        31u
#define RZC_SIG_ENCODER_STALL      32u
#define RZC_SIG_VEHICLE_STATE      33u
#define RZC_SIG_ESTOP_ACTIVE       34u
#define RZC_SIG_FAULT_MASK         35u
#define RZC_SIG_SELF_TEST_RESULT   36u
#define RZC_SIG_HEARTBEAT_ALIVE    37u
#define RZC_SIG_SAFETY_STATUS      38u
#define RZC_SIG_CMD_TIMEOUT        39u
#define RZC_SIG_COUNT              40u

/* ====================================================================
 * Com TX PDU IDs
 * ==================================================================== */

#define RZC_COM_TX_HEARTBEAT       0u   /* CAN 0x012 */
#define RZC_COM_TX_MOTOR_STATUS    1u   /* CAN 0x300 */
#define RZC_COM_TX_MOTOR_CURRENT   2u   /* CAN 0x301 */
#define RZC_COM_TX_MOTOR_TEMP      3u   /* CAN 0x302 */
#define RZC_COM_TX_BATTERY_STATUS  4u   /* CAN 0x303 */

/* ====================================================================
 * Com RX PDU IDs
 * ==================================================================== */

#define RZC_COM_RX_ESTOP           0u   /* CAN 0x001 */
#define RZC_COM_RX_VEHICLE_TORQUE  1u   /* CAN 0x100 — Vehicle_State + Torque */

/* ====================================================================
 * DTC Event IDs (Dem_EventIdType)
 * ==================================================================== */

#define RZC_DTC_OVERCURRENT        0u   /* 0xE00100 */
#define RZC_DTC_OVERTEMP           1u   /* 0xE00200 */
#define RZC_DTC_STALL              2u   /* 0xE00300 */
#define RZC_DTC_DIRECTION          3u   /* 0xE00400 */
#define RZC_DTC_SHOOT_THROUGH      4u   /* 0xE00500 */
#define RZC_DTC_CAN_BUS_OFF        5u   /* 0xE00600 */
#define RZC_DTC_CMD_TIMEOUT        6u   /* 0xE00700 */
#define RZC_DTC_SELF_TEST_FAIL     7u   /* 0xE00800 */
#define RZC_DTC_WATCHDOG_FAIL      8u   /* 0xE00900 */
#define RZC_DTC_BATTERY            9u   /* 0xE00A00 */
#define RZC_DTC_ENCODER           10u   /* 0xE00B00 */
#define RZC_DTC_ZERO_CAL          11u   /* 0xE00C00 */

/* ====================================================================
 * E2E Data IDs (per CAN message)
 * ==================================================================== */

#define RZC_E2E_HEARTBEAT_DATA_ID    0x04u
#define RZC_E2E_MOTOR_STATUS_DATA_ID 0x0Eu
#define RZC_E2E_MOTOR_CURRENT_DATA_ID 0x0Fu
#define RZC_E2E_ESTOP_DATA_ID        0x00u

/* ====================================================================
 * Motor Constants (ASIL D — BTS7960 H-Bridge)
 * ==================================================================== */

/** TIM1 base frequency for 20kHz switching */
#define RZC_MOTOR_PWM_FREQ_HZ     20000u

/** Maximum duty cycle (95% cap — BTS7960 bootstrap capacitor) */
#define RZC_MOTOR_MAX_DUTY_PCT       95u

/** Dead-time between direction change (10 microseconds) */
#define RZC_MOTOR_DEADTIME_US        10u

/** Mode-based torque limits (percent of max torque) */
#define RZC_MOTOR_LIMIT_RUN         100u
#define RZC_MOTOR_LIMIT_DEGRADED     75u
#define RZC_MOTOR_LIMIT_LIMP         30u
#define RZC_MOTOR_LIMIT_SAFE_STOP     0u

/** Command timeout: 100ms with no valid torque command */
#define RZC_MOTOR_CMD_TIMEOUT_MS    100u

/** Recovery: 5 valid messages to re-enable after timeout */
#define RZC_MOTOR_CMD_RECOVERY        5u

/** PWM duty range: 0..10000 (0.01% resolution, from IoHwAb) */
#define RZC_MOTOR_PWM_SCALE       10000u

/* ====================================================================
 * Current Monitoring Constants (ASIL A — ACS723)
 * ==================================================================== */

/** Overcurrent threshold in milliamps */
#define RZC_CURRENT_OC_THRESH_MA  25000u

/** Overcurrent debounce: 10ms at 1kHz = 10 consecutive samples */
#define RZC_CURRENT_OC_DEBOUNCE      10u

/** Recovery time: 500ms below threshold */
#define RZC_CURRENT_RECOVERY_MS     500u

/** Zero-cal: 64 samples averaged at startup */
#define RZC_CURRENT_ZEROCAL_SAMPLES  64u

/** Zero-cal expected center (12-bit ADC midpoint) */
#define RZC_CURRENT_ZEROCAL_CENTER 2048u

/** Zero-cal acceptable offset range from center */
#define RZC_CURRENT_ZEROCAL_RANGE   200u

/** ACS723 sensitivity: 400mV/A = 0.4 mV/mA */
#define RZC_CURRENT_SENSITIVITY_UV  400u

/** Moving average window size */
#define RZC_CURRENT_AVG_WINDOW        4u

/* ====================================================================
 * Temperature Monitoring Constants (ASIL A — NTC Steinhart-Hart)
 * ==================================================================== */

/** Steinhart-Hart reference temperature (25°C in Kelvin) */
#define RZC_TEMP_T0_K             29815u   /* 298.15 K * 100 (scaled) */

/** NTC reference resistance at T0 in ohms */
#define RZC_TEMP_R0_OHM           10000u

/** NTC B-coefficient */
#define RZC_TEMP_B_COEFF           3950u

/** Temperature derating curve thresholds (degrees C) */
#define RZC_TEMP_DERATE_NONE_C       60u   /* Below: 100% power */
#define RZC_TEMP_DERATE_75_C         80u   /* 60-79°C: 75% power */
#define RZC_TEMP_DERATE_50_C        100u   /* 80-99°C: 50% power */
                                           /* >= 100°C: 0% power (shutdown) */

/** Derating power percentages */
#define RZC_TEMP_DERATE_100_PCT     100u
#define RZC_TEMP_DERATE_75_PCT       75u
#define RZC_TEMP_DERATE_50_PCT       50u
#define RZC_TEMP_DERATE_0_PCT         0u

/** Recovery hysteresis (10°C lower to recover to higher power level) */
#define RZC_TEMP_HYSTERESIS_C        10u

/** Plausible NTC temperature range (deci-degrees C) */
#define RZC_TEMP_MIN_DDC           (-300)  /* -30.0°C */
#define RZC_TEMP_MAX_DDC           1500    /* 150.0°C */

/* ====================================================================
 * Encoder Constants (ASIL C — Quadrature Encoder TIM4)
 * ==================================================================== */

/** Encoder pulses per revolution */
#define RZC_ENCODER_PPR             360u

/** Stall detection: PWM > 10% but zero speed for 500ms */
#define RZC_ENCODER_STALL_TIMEOUT_MS 500u

/** Stall check period: 10ms per check = 50 checks for 500ms */
#define RZC_ENCODER_STALL_CHECKS     50u

/** Direction mismatch: commanded vs encoder for 50ms */
#define RZC_ENCODER_DIR_MISMATCH_MS  50u

/** Direction mismatch check count at 10ms period */
#define RZC_ENCODER_DIR_CHECKS        5u

/** Grace period after direction change (stall) */
#define RZC_ENCODER_STALL_GRACE_MS  200u

/** Grace period after direction change (direction plausibility) */
#define RZC_ENCODER_DIR_GRACE_MS    100u

/** Minimum PWM % to trigger stall detection */
#define RZC_ENCODER_STALL_MIN_PWM    10u

/* ====================================================================
 * Battery Monitoring Constants (QM — Voltage Divider)
 * ==================================================================== */

/** Battery disable low threshold (mV) */
#define RZC_BATT_DISABLE_LOW_MV    8000u

/** Battery warning low threshold (mV) */
#define RZC_BATT_WARN_LOW_MV      10500u

/** Battery warning high threshold (mV) */
#define RZC_BATT_WARN_HIGH_MV     15000u

/** Battery disable high threshold (mV) */
#define RZC_BATT_DISABLE_HIGH_MV  17000u

/** Hysteresis band (mV) for recovery */
#define RZC_BATT_HYSTERESIS_MV      500u

/** Voltage divider ratio: (R_H + R_L) / R_L = (47k + 10k) / 10k = 5.7 */
#define RZC_BATT_DIVIDER_RH       47000u   /* High-side resistor ohms */
#define RZC_BATT_DIVIDER_RL       10000u   /* Low-side resistor ohms */

/** Moving average window */
#define RZC_BATT_AVG_WINDOW           4u

/** Battery status codes */
#define RZC_BATT_STATUS_DISABLE_LOW   0u
#define RZC_BATT_STATUS_WARN_LOW      1u
#define RZC_BATT_STATUS_NORMAL        2u
#define RZC_BATT_STATUS_WARN_HIGH     3u
#define RZC_BATT_STATUS_DISABLE_HIGH  4u

/* ====================================================================
 * Heartbeat Constants
 * ==================================================================== */

#define RZC_HB_TX_PERIOD_MS        50u     /* TX every 50ms */
#define RZC_HB_TIMEOUT_MS         150u     /* 3x TX period */
#define RZC_HB_MAX_MISS             3u     /* Consecutive misses before timeout */
#define RZC_HB_ALIVE_MAX           15u     /* 4-bit alive counter wraps at 15 */

#define RZC_ECU_ID               0x03u     /* RZC ECU identifier */

/* ====================================================================
 * Vehicle State (received from CVC)
 * ==================================================================== */

#define RZC_STATE_INIT              0u
#define RZC_STATE_RUN               1u
#define RZC_STATE_DEGRADED          2u
#define RZC_STATE_LIMP              3u
#define RZC_STATE_SAFE_STOP         4u
#define RZC_STATE_SHUTDOWN          5u
#define RZC_STATE_COUNT             6u

/* ====================================================================
 * Self-Test Constants
 * ==================================================================== */

#define RZC_SELF_TEST_PASS          1u
#define RZC_SELF_TEST_FAIL          0u

/** Number of self-test items (8 for RZC) */
#define RZC_SELF_TEST_ITEMS         8u

/* ====================================================================
 * Fault Bitmask Positions
 * ==================================================================== */

#define RZC_FAULT_NONE           0x00u
#define RZC_FAULT_OVERCURRENT    0x01u
#define RZC_FAULT_OVERTEMP       0x02u
#define RZC_FAULT_DIRECTION      0x04u
#define RZC_FAULT_CAN            0x08u
#define RZC_FAULT_WATCHDOG       0x10u
#define RZC_FAULT_SELF_TEST      0x20u
#define RZC_FAULT_BATTERY        0x40u
#define RZC_FAULT_STALL          0x80u

/* ====================================================================
 * Motor Fault Enum
 * ==================================================================== */

#define RZC_MOTOR_NO_FAULT          0u
#define RZC_MOTOR_SHOOT_THROUGH     1u
#define RZC_MOTOR_CMD_TIMEOUT       2u
#define RZC_MOTOR_OVERCURRENT       3u
#define RZC_MOTOR_OVERTEMP          4u
#define RZC_MOTOR_STALL             5u
#define RZC_MOTOR_DIRECTION         6u

/* ====================================================================
 * Motor Direction Enum (matches IoHwAb)
 * ==================================================================== */

#define RZC_DIR_FORWARD             0u
#define RZC_DIR_REVERSE             1u
#define RZC_DIR_STOP                2u

/* ====================================================================
 * Safety WDI Pin
 * ==================================================================== */

#define RZC_SAFETY_WDI_CHANNEL      4u     /* PB4 — TPS3823 WDI pin */

/* ====================================================================
 * BTS7960 Enable Pins (independent disable path)
 * ==================================================================== */

#define RZC_MOTOR_R_EN_CHANNEL      5u     /* R_EN GPIO */
#define RZC_MOTOR_L_EN_CHANNEL      6u     /* L_EN GPIO */

/* ====================================================================
 * CAN Bus Loss Constants
 * ==================================================================== */

#define RZC_CAN_SILENCE_TIMEOUT_MS 200u    /* 200ms CAN silence → disable */
#define RZC_CAN_ERR_WARN_TIMEOUT_MS 500u   /* Error warning > 500ms → disable */

#endif /* RZC_CFG_H */
