/**
 * @file    sc_cfg.h
 * @brief   Safety Controller configuration — all SC-specific constants
 * @date    2026-02-23
 *
 * @details Unified configuration header for the Safety Controller.
 *          Contains CAN mailbox IDs, E2E Data IDs, timing constants,
 *          GIO pin assignments, threshold values, and state enums.
 *
 * @safety_req SWR-SC-001 to SWR-SC-026
 * @traces_to  SSR-SC-001 to SSR-SC-017
 * @note    Safety level: ASIL D
 * @standard ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#ifndef SC_CFG_H
#define SC_CFG_H

#include "sc_types.h"

/* ==================================================================
 * CAN Message IDs (DCAN1 receive mailboxes)
 * ================================================================== */

#define SC_CAN_ID_ESTOP             0x001u   /* E-Stop (CVC) */
#define SC_CAN_ID_CVC_HB            0x010u   /* CVC heartbeat */
#define SC_CAN_ID_FZC_HB            0x011u   /* FZC heartbeat */
#define SC_CAN_ID_RZC_HB            0x012u   /* RZC heartbeat */
#define SC_CAN_ID_VEHICLE_STATE     0x100u   /* Vehicle_State (torque in byte 4) */
#define SC_CAN_ID_MOTOR_CURRENT     0x301u   /* Motor_Current (RZC) */

/* ==================================================================
 * CAN Mailbox Numbers (1-indexed, DCAN1 hardware)
 * ================================================================== */

#define SC_MB_ESTOP                 1u
#define SC_MB_CVC_HB                2u
#define SC_MB_FZC_HB                3u
#define SC_MB_RZC_HB                4u
#define SC_MB_VEHICLE_STATE         5u
#define SC_MB_MOTOR_CURRENT         6u
#define SC_MB_COUNT                 6u

/* Mailbox index (0-based for arrays) */
#define SC_MB_IDX_ESTOP             0u
#define SC_MB_IDX_CVC_HB            1u
#define SC_MB_IDX_FZC_HB            2u
#define SC_MB_IDX_RZC_HB            3u
#define SC_MB_IDX_VEHICLE_STATE     4u
#define SC_MB_IDX_MOTOR_CURRENT     5u

/* ==================================================================
 * E2E Data IDs (per message type)
 * ================================================================== */

#define SC_E2E_ESTOP_DATA_ID        0x01u
#define SC_E2E_CVC_HB_DATA_ID      0x02u
#define SC_E2E_FZC_HB_DATA_ID      0x03u
#define SC_E2E_RZC_HB_DATA_ID      0x04u
#define SC_E2E_VEHSTATE_DATA_ID    0x05u
#define SC_E2E_MOTORCUR_DATA_ID    0x0Fu

/* ==================================================================
 * CRC-8 Parameters (SAE-J1850)
 * ================================================================== */

#define SC_CRC8_POLY                0x1Du
#define SC_CRC8_INIT                0xFFu

/* ==================================================================
 * Heartbeat Timing (in 10ms ticks)
 * ================================================================== */

#define SC_HB_TIMEOUT_TICKS         15u    /* 150ms = 3x 50ms heartbeat period */
#define SC_HB_CONFIRM_TICKS         5u     /* 50ms additional confirmation */
#define SC_HB_ALIVE_MAX             15u    /* 4-bit alive counter max */

/* ==================================================================
 * Bus Silence Monitoring
 * ================================================================== */

#define SC_BUS_SILENCE_TICKS        20u    /* 200ms = all-heartbeat timeout */

/* ==================================================================
 * Plausibility Thresholds
 * ================================================================== */

#define SC_PLAUS_REL_THRESHOLD      20u    /* 20% relative difference */
#define SC_PLAUS_ABS_THRESHOLD_MA   2000u  /* 2000 mA absolute (near-zero) */
#define SC_PLAUS_DEBOUNCE_TICKS     5u     /* 50ms debounce */

/* ==================================================================
 * Backup Cutoff (SWR-SC-024)
 * ================================================================== */

#define SC_BACKUP_CUTOFF_CURRENT_MA 1000u  /* 1000 mA threshold */
#define SC_BACKUP_CUTOFF_TICKS      10u    /* 100ms timeout */

/* ==================================================================
 * GIO Pin Assignments
 * ================================================================== */

#define SC_GIO_PORT_A               0u
#define SC_GIO_PORT_B               1u

#define SC_PIN_RELAY                0u     /* GIO_A0: Kill relay output */
#define SC_PIN_LED_CVC              1u     /* GIO_A1: CVC fault LED */
#define SC_PIN_LED_FZC              2u     /* GIO_A2: FZC fault LED */
#define SC_PIN_LED_RZC              3u     /* GIO_A3: RZC fault LED */
#define SC_PIN_LED_SYS              4u     /* GIO_A4: System fault LED (amber) */
#define SC_PIN_WDI                  5u     /* GIO_A5: TPS3823 watchdog input */
#define SC_PIN_LED_HB               1u     /* GIO_B1: Heartbeat LED (onboard) */

/* ==================================================================
 * LED Blink Timing (in 10ms ticks)
 * ================================================================== */

#define SC_LED_BLINK_ON_TICKS       25u    /* 250ms on */
#define SC_LED_BLINK_OFF_TICKS      25u    /* 250ms off = 500ms period */
#define SC_LED_BLINK_PERIOD         50u    /* Total blink period in ticks */

/* ==================================================================
 * Stack Canary
 * ================================================================== */

#define SC_STACK_CANARY_VALUE       0xDEADBEEFu
#define SC_STACK_CANARY_ADDR        0x08000420u

/* ==================================================================
 * RAM Self-Test
 * ================================================================== */

#define SC_RAM_TEST_ADDR            0x08000400u
#define SC_RAM_TEST_SIZE            32u    /* 32 bytes */

/* ==================================================================
 * Watchdog Conditions Bitmask
 * ================================================================== */

#define SC_WDG_COND_MONITOR         0x01u  /* Monitoring functions executed */
#define SC_WDG_COND_RAM_OK          0x02u  /* RAM pattern intact */
#define SC_WDG_COND_CAN_OK          0x04u  /* DCAN1 not bus-off */
#define SC_WDG_COND_ESM_OK          0x08u  /* ESM error not asserted */
#define SC_WDG_COND_STACK_OK        0x10u  /* Stack canary intact */
#define SC_WDG_COND_ALL             0x1Fu  /* All conditions met */

/* ==================================================================
 * E2E Failure Threshold
 * ================================================================== */

#define SC_E2E_MAX_CONSEC_FAIL      3u     /* 3 consecutive E2E failures */

/* ==================================================================
 * Relay Readback
 * ================================================================== */

#define SC_RELAY_READBACK_THRESHOLD 2u     /* 2 consecutive mismatches */

/* ==================================================================
 * Self-Test Runtime Period
 * ================================================================== */

#define SC_SELFTEST_RUNTIME_PERIOD  6000u  /* 60s = 6000 ticks at 10ms */
#define SC_SELFTEST_RUNTIME_STEPS   4u     /* 4 steps spread across ticks */

/* ==================================================================
 * ECU Index Enum
 * ================================================================== */

#define SC_ECU_CVC                  0u
#define SC_ECU_FZC                  1u
#define SC_ECU_RZC                  2u
#define SC_ECU_COUNT                3u

/* ==================================================================
 * SC State Enum
 * ================================================================== */

#define SC_STATE_INIT               0u
#define SC_STATE_MONITORING         1u
#define SC_STATE_FAULT              2u
#define SC_STATE_KILL               3u

/* ==================================================================
 * Torque Lookup Table Size
 * ================================================================== */

#define SC_TORQUE_LUT_SIZE          16u

/* ==================================================================
 * CAN DLC (standard 8 bytes)
 * ================================================================== */

#define SC_CAN_DLC                  8u

/* ==================================================================
 * DCAN1 Baud Rate Configuration (500 kbps from 75 MHz VCLK1)
 * ================================================================== */

#define SC_DCAN_BRP                 15u    /* Baud rate prescaler */
#define SC_DCAN_TSEG1               7u     /* Time segment 1 */
#define SC_DCAN_TSEG2               2u     /* Time segment 2 */

#endif /* SC_CFG_H */
