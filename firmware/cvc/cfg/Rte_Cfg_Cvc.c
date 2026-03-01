/**
 * @file    Rte_Cfg_Cvc.c
 * @brief   RTE configuration for CVC — signal table and runnable table
 * @date    2026-02-21
 *
 * @safety_req SWR-CVC-001 to SWR-CVC-035
 * @traces_to  SSR-CVC-001 to SSR-CVC-035, TSR-046, TSR-047
 *
 * @standard AUTOSAR RTE, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#include "Rte.h"
#include "Cvc_Cfg.h"

/* ==================================================================
 * Forward declarations for SWC runnables
 * ================================================================== */

extern void Swc_Pedal_MainFunction(void);
extern void Swc_VehicleState_MainFunction(void);
extern void Swc_EStop_MainFunction(void);
extern void Swc_Heartbeat_MainFunction(void);
extern void Swc_Dashboard_MainFunction(void);
extern void Com_MainFunction_Tx(void);
extern void Can_MainFunction_Read(void);
extern void Can_MainFunction_BusOff(void);

/* ==================================================================
 * Signal Configuration Table
 * ================================================================== */

static const Rte_SignalConfigType cvc_signal_config[CVC_SIG_COUNT] = {
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

    /* CVC application signals (16-30) */
    { CVC_SIG_PEDAL_RAW_1,     0u },   /* 16: Raw pedal sensor 1            */
    { CVC_SIG_PEDAL_RAW_2,     0u },   /* 17: Raw pedal sensor 2            */
    { CVC_SIG_PEDAL_POSITION,  0u },   /* 18: Processed pedal position      */
    { CVC_SIG_PEDAL_FAULT,     0u },   /* 19: Pedal fault code              */
    { CVC_SIG_VEHICLE_STATE,   CVC_STATE_INIT },  /* 20: Vehicle state      */
    { CVC_SIG_TORQUE_REQUEST,  0u },   /* 21: Torque request 0-1000         */
    { CVC_SIG_ESTOP_ACTIVE,    0u },   /* 22: E-stop active flag            */
    { CVC_SIG_FZC_COMM_STATUS, CVC_COMM_OK },     /* 23: FZC comm status    */
    { CVC_SIG_RZC_COMM_STATUS, CVC_COMM_OK },     /* 24: RZC comm status    */
    { CVC_SIG_MOTOR_SPEED,     0u },   /* 25: Motor speed RPM               */
    { CVC_SIG_FAULT_MASK,      0u },   /* 26: Fault bitmask                 */
    { CVC_SIG_MOTOR_CURRENT,   0u },   /* 27: Motor current mA              */
    { CVC_SIG_MOTOR_CUTOFF,    0u },   /* 28: Motor cutoff received         */
    { CVC_SIG_STEERING_FAULT,  0u },   /* 29: Steering fault received       */
    { CVC_SIG_BRAKE_FAULT,     0u },   /* 30: Brake fault received          */
};

/* ==================================================================
 * Runnable Configuration Table
 * Priority: higher number = executes first within same period
 * ================================================================== */

static const Rte_RunnableConfigType cvc_runnable_config[] = {
    /* func,                           periodMs, priority, seId */
    { Can_MainFunction_Read,              10u,      8u,     0xFFu },  /* CAN RX first            */
    { Swc_EStop_MainFunction,             10u,      7u,     2u    },  /* E-stop highest SWC prio */
    { Swc_Pedal_MainFunction,             10u,      6u,     0u    },  /* Pedal processing        */
    { Swc_VehicleState_MainFunction,      10u,      5u,     1u    },  /* State machine           */
    { Swc_Heartbeat_MainFunction,         10u,      4u,     3u    },  /* Heartbeat TX/RX         */
    { Swc_Dashboard_MainFunction,         10u,      3u,     4u    },  /* Display                 */
    { Com_MainFunction_Tx,                10u,      2u,     0xFFu },  /* COM TX (after all SWCs) */
    { Can_MainFunction_BusOff,            10u,      1u,     0xFFu },  /* Bus-off check           */
};

#define CVC_RUNNABLE_COUNT  (sizeof(cvc_runnable_config) / sizeof(cvc_runnable_config[0]))

/* ==================================================================
 * Aggregate RTE Configuration
 * ================================================================== */

const Rte_ConfigType cvc_rte_config = {
    .signalConfig   = cvc_signal_config,
    .signalCount    = CVC_SIG_COUNT,
    .runnableConfig = cvc_runnable_config,
    .runnableCount  = (uint8)CVC_RUNNABLE_COUNT,
};
