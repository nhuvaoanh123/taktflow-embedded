/**
 * @file    Rte_Cfg_Rzc.c
 * @brief   RTE configuration for RZC — signal table and runnable table
 * @date    2026-02-23
 *
 * @safety_req SWR-RZC-001 to SWR-RZC-030
 * @traces_to  SSR-RZC-001 to SSR-RZC-017, TSR-046, TSR-047
 *
 * @standard AUTOSAR RTE, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#include "Rte.h"
#include "Rzc_Cfg.h"

/* ==================================================================
 * Forward declarations for SWC runnables
 * ================================================================== */

extern void Swc_CurrentMonitor_MainFunction(void);
extern void Swc_Motor_MainFunction(void);
extern void Swc_Encoder_MainFunction(void);
extern void Can_MainFunction_Read(void);
extern void Swc_TempMonitor_MainFunction(void);
extern void Swc_Battery_MainFunction(void);
extern void Swc_Heartbeat_MainFunction(void);
extern void Com_MainFunction_Tx(void);
extern void Can_MainFunction_BusOff(void);
extern void Swc_RzcSafety_MainFunction(void);   /* WatchdogFeed */

/* ==================================================================
 * Signal Configuration Table
 * ================================================================== */

static const Rte_SignalConfigType rzc_signal_config[RZC_SIG_COUNT] = {
    /* BSW well-known signals (0-15) — initial values */
    {  0u, 0u },   /* RTE_SIG_TORQUE_REQUEST  */
    {  1u, 0u },   /* RTE_SIG_STEERING_ANGLE  */
    {  2u, 0u },   /* RTE_SIG_VEHICLE_SPEED   */
    {  3u, 0u },   /* RTE_SIG_BRAKE_PRESSURE  */
    {  4u, 0u },   /* RTE_SIG_MOTOR_STATUS    */
    {  5u, 0u },   /* RTE_SIG_BATTERY_VOLTAGE */
    {  6u, 0u },   /* RTE_SIG_BATTERY_CURRENT */
    {  7u, 0u },   /* RTE_SIG_MOTOR_TEMP      */
    {  8u, 0u },   /* RTE_SIG_ESTOP_STATUS    */
    {  9u, 0u },   /* RTE_SIG_HEARTBEAT       */
    { 10u, 0u },   /* Reserved                */
    { 11u, 0u },   /* Reserved                */
    { 12u, 0u },   /* Reserved                */
    { 13u, 0u },   /* Reserved                */
    { 14u, 0u },   /* Reserved                */
    { 15u, 0u },   /* Reserved                */

    /* RZC application signals (16-39) */
    { RZC_SIG_TORQUE_CMD,        0u },   /* 16: Torque command from CVC       */
    { RZC_SIG_TORQUE_ECHO,       0u },   /* 17: Torque echo (applied %)       */
    { RZC_SIG_MOTOR_SPEED,       0u },   /* 18: Motor speed RPM               */
    { RZC_SIG_MOTOR_DIR,         0u },   /* 19: Motor direction               */
    { RZC_SIG_MOTOR_ENABLE,      0u },   /* 20: Motor enable flag             */
    { RZC_SIG_MOTOR_FAULT,       0u },   /* 21: Motor fault code              */
    { RZC_SIG_CURRENT_MA,        0u },   /* 22: Motor current in mA           */
    { RZC_SIG_OVERCURRENT,       0u },   /* 23: Overcurrent flag              */
    { RZC_SIG_TEMP1_DC,          0u },   /* 24: Temperature sensor 1 deci-C   */
    { RZC_SIG_TEMP2_DC,          0u },   /* 25: Temperature sensor 2 deci-C   */
    { RZC_SIG_DERATING_PCT,      0u },   /* 26: Derating percentage           */
    { RZC_SIG_TEMP_FAULT,        0u },   /* 27: Temperature fault code        */
    { RZC_SIG_BATTERY_MV,        0u },   /* 28: Battery voltage in mV         */
    { RZC_SIG_BATTERY_STATUS,    0u },   /* 29: Battery status code           */
    { RZC_SIG_ENCODER_SPEED,     0u },   /* 30: Encoder speed RPM             */
    { RZC_SIG_ENCODER_DIR,       0u },   /* 31: Encoder direction             */
    { RZC_SIG_ENCODER_STALL,     0u },   /* 32: Encoder stall flag            */
    { RZC_SIG_VEHICLE_STATE,     RZC_STATE_INIT },  /* 33: Vehicle state      */
    { RZC_SIG_ESTOP_ACTIVE,      0u },   /* 34: E-stop active flag            */
    { RZC_SIG_FAULT_MASK,        0u },   /* 35: Fault bitmask                 */
    { RZC_SIG_SELF_TEST_RESULT,  0u },   /* 36: Self-test result              */
    { RZC_SIG_HEARTBEAT_ALIVE,   0u },   /* 37: Heartbeat alive counter       */
    { RZC_SIG_SAFETY_STATUS,     0u },   /* 38: Aggregated safety status      */
    { RZC_SIG_CMD_TIMEOUT,       0u },   /* 39: Command timeout flag          */
};

/* ==================================================================
 * Runnable Configuration Table
 * Priority: higher number = executes first within same period
 * ================================================================== */

static const Rte_RunnableConfigType rzc_runnable_config[] = {
    /* func,                              periodMs, priority, seId */
    { Swc_CurrentMonitor_MainFunction,       1u,      7u,     0u    },  /* Current monitor (1kHz) */
    { Swc_Motor_MainFunction,               10u,      6u,     1u    },  /* Motor control          */
    { Swc_Encoder_MainFunction,             10u,      6u,     2u    },  /* Encoder processing     */
    { Can_MainFunction_Read,                10u,      5u,     0xFFu },  /* CAN RX                 */
    { Com_MainFunction_Tx,                  10u,      5u,     0xFFu },  /* COM TX                 */
    { Swc_TempMonitor_MainFunction,        100u,      4u,     3u    },  /* Temperature monitor    */
    { Swc_Battery_MainFunction,            100u,      4u,     4u    },  /* Battery monitor        */
    { Swc_Heartbeat_MainFunction,           50u,      3u,     5u    },  /* Heartbeat TX/RX        */
    { Can_MainFunction_BusOff,              10u,      2u,     0xFFu },  /* Bus-off check          */
    { Swc_RzcSafety_MainFunction,          100u,      2u,     6u    },  /* Safety / WatchdogFeed  */
};

#define RZC_RUNNABLE_COUNT  (sizeof(rzc_runnable_config) / sizeof(rzc_runnable_config[0]))

/* ==================================================================
 * Aggregate RTE Configuration
 * ================================================================== */

const Rte_ConfigType rzc_rte_config = {
    .signalConfig   = rzc_signal_config,
    .signalCount    = RZC_SIG_COUNT,
    .runnableConfig = rzc_runnable_config,
    .runnableCount  = (uint8)RZC_RUNNABLE_COUNT,
};
