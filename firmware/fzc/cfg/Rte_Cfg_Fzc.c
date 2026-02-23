/**
 * @file    Rte_Cfg_Fzc.c
 * @brief   RTE configuration for FZC — signal table and runnable table
 * @date    2026-02-23
 *
 * @safety_req SWR-FZC-001 to SWR-FZC-032
 * @traces_to  SSR-FZC-001 to SSR-FZC-024, TSR-046, TSR-047
 *
 * @standard AUTOSAR RTE, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#include "Rte.h"
#include "Fzc_Cfg.h"

/* ==================================================================
 * Forward declarations for SWC runnables
 * ================================================================== */

extern void Swc_Steering_MainFunction(void);
extern void Swc_Brake_MainFunction(void);
extern void Swc_Lidar_MainFunction(void);
extern void Swc_Heartbeat_MainFunction(void);
extern void Swc_Buzzer_MainFunction(void);
extern void Com_MainFunction_Tx(void);
extern void Can_MainFunction_Read(void);
extern void Can_MainFunction_BusOff(void);

/* ==================================================================
 * Signal Configuration Table
 * ================================================================== */

static const Rte_SignalConfigType fzc_signal_config[FZC_SIG_COUNT] = {
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

    /* FZC application signals (16-35) */
    { FZC_SIG_STEER_CMD,         0u },   /* 16: Steering command from CVC     */
    { FZC_SIG_STEER_ANGLE,       0u },   /* 17: Current steering angle        */
    { FZC_SIG_STEER_FAULT,       0u },   /* 18: Steering fault code           */
    { FZC_SIG_BRAKE_CMD,         0u },   /* 19: Brake command from CVC        */
    { FZC_SIG_BRAKE_POS,         0u },   /* 20: Brake position feedback       */
    { FZC_SIG_BRAKE_FAULT,       0u },   /* 21: Brake fault code              */
    { FZC_SIG_LIDAR_DIST,        0u },   /* 22: Lidar distance in cm          */
    { FZC_SIG_LIDAR_SIGNAL,      0u },   /* 23: Lidar signal strength         */
    { FZC_SIG_LIDAR_ZONE,        0u },   /* 24: Lidar proximity zone          */
    { FZC_SIG_LIDAR_FAULT,       0u },   /* 25: Lidar fault code              */
    { FZC_SIG_VEHICLE_STATE,     FZC_STATE_INIT },  /* 26: Vehicle state       */
    { FZC_SIG_ESTOP_ACTIVE,      0u },   /* 27: E-stop active flag            */
    { FZC_SIG_BUZZER_PATTERN,    0u },   /* 28: Active buzzer pattern         */
    { FZC_SIG_MOTOR_CUTOFF,      0u },   /* 29: Motor cutoff flag             */
    { FZC_SIG_FAULT_MASK,        0u },   /* 30: Fault bitmask                 */
    { FZC_SIG_STEER_PWM_DISABLE, 0u },   /* 31: Steering PWM disable flag     */
    { FZC_SIG_BRAKE_PWM_DISABLE, 0u },   /* 32: Brake PWM disable flag        */
    { FZC_SIG_SELF_TEST_RESULT,  0u },   /* 33: Self-test result              */
    { FZC_SIG_HEARTBEAT_ALIVE,   0u },   /* 34: Heartbeat alive counter       */
    { FZC_SIG_SAFETY_STATUS,     0u },   /* 35: Aggregated safety status      */
};

/* ==================================================================
 * Runnable Configuration Table
 * Priority: higher number = executes first within same period
 * ================================================================== */

static const Rte_RunnableConfigType fzc_runnable_config[] = {
    /* func,                           periodMs, priority, seId */
    { Can_MainFunction_Read,              10u,      7u,     0xFFu },  /* CAN RX first          */
    { Com_MainFunction_Tx,                10u,      7u,     0xFFu },  /* COM TX                */
    { Swc_Steering_MainFunction,          10u,      6u,     0u    },  /* Steering highest SWC  */
    { Swc_Brake_MainFunction,             10u,      5u,     1u    },  /* Brake control         */
    { Swc_Lidar_MainFunction,             10u,      4u,     2u    },  /* Lidar processing      */
    { Swc_Heartbeat_MainFunction,         10u,      3u,     3u    },  /* Heartbeat TX/RX       */
    { Can_MainFunction_BusOff,            10u,      2u,     0xFFu },  /* Bus-off check         */
    { Swc_Buzzer_MainFunction,            10u,      1u,     4u    },  /* Buzzer (lowest prio)  */
};

#define FZC_RUNNABLE_COUNT  (sizeof(fzc_runnable_config) / sizeof(fzc_runnable_config[0]))

/* ==================================================================
 * Aggregate RTE Configuration
 * ================================================================== */

const Rte_ConfigType fzc_rte_config = {
    .signalConfig   = fzc_signal_config,
    .signalCount    = FZC_SIG_COUNT,
    .runnableConfig = fzc_runnable_config,
    .runnableCount  = (uint8)FZC_RUNNABLE_COUNT,
};
