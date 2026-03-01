/**
 * @file    Rte_Cfg_Bcm.c
 * @brief   RTE configuration for BCM — signal table and runnable table
 * @date    2026-02-23
 *
 * @safety_req SWR-BCM-001 to SWR-BCM-012
 * @traces_to  SSR-BCM-001 to SSR-BCM-012, TSR-046, TSR-047
 *
 * @standard AUTOSAR RTE, ISO 26262 Part 6 (QM)
 * @copyright Taktflow Systems 2026
 */
#include "Rte.h"
#include "Bcm_Cfg.h"

/* ==================================================================
 * Forward declarations for SWC runnables
 * ================================================================== */

extern void Swc_Lights_10ms(void);
extern void Swc_Indicators_10ms(void);
extern void Swc_DoorLock_100ms(void);
extern void Com_MainFunction_Tx(void);
extern void Can_MainFunction_Read(void);
extern void Can_MainFunction_BusOff(void);

/* ==================================================================
 * Signal Configuration Table
 * ================================================================== */

static const Rte_SignalConfigType bcm_signal_config[BCM_SIG_COUNT] = {
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

    /* BCM application signals (16-25) */
    { BCM_SIG_VEHICLE_SPEED,    0u },   /* 16: Vehicle speed (from CVC via CAN 0x100)  */
    { BCM_SIG_VEHICLE_STATE,    BCM_VSTATE_INIT },  /* 17: Vehicle state               */
    { BCM_SIG_BODY_CONTROL_CMD, 0u },   /* 18: Body control command (CAN 0x350)        */
    { BCM_SIG_LIGHT_HEADLAMP,   0u },   /* 19: Headlamp output                         */
    { BCM_SIG_LIGHT_TAIL,       0u },   /* 20: Tail light output                       */
    { BCM_SIG_INDICATOR_LEFT,   0u },   /* 21: Left indicator output                   */
    { BCM_SIG_INDICATOR_RIGHT,  0u },   /* 22: Right indicator output                  */
    { BCM_SIG_HAZARD_ACTIVE,    0u },   /* 23: Hazard active flag                      */
    { BCM_SIG_DOOR_LOCK_STATE,  0u },   /* 24: Door lock state (0=unlocked, 1=locked)  */
    { BCM_SIG_ESTOP_ACTIVE,     0u },   /* 25: E-stop active (from CVC)                */
};

/* ==================================================================
 * Runnable Configuration Table
 * Priority: higher number = executes first within same period
 * ================================================================== */

static const Rte_RunnableConfigType bcm_runnable_config[] = {
    /* func,                        periodMs, priority, seId */
    { Can_MainFunction_Read,           10u,      7u,     0xFFu },  /* CAN RX first            */
    { Swc_Lights_10ms,                 10u,      6u,     0u    },  /* Lights processing       */
    { Swc_Indicators_10ms,             10u,      5u,     1u    },  /* Indicator flash         */
    { Swc_DoorLock_100ms,             100u,      4u,     2u    },  /* Door lock (100ms)       */
    { Com_MainFunction_Tx,             10u,      2u,     0xFFu },  /* COM TX (after all SWCs) */
    { Can_MainFunction_BusOff,         10u,      1u,     0xFFu },  /* Bus-off check           */
};

#define BCM_RUNNABLE_COUNT  (sizeof(bcm_runnable_config) / sizeof(bcm_runnable_config[0]))

/* ==================================================================
 * Aggregate RTE Configuration
 * ================================================================== */

const Rte_ConfigType bcm_rte_config = {
    .signalConfig   = bcm_signal_config,
    .signalCount    = BCM_SIG_COUNT,
    .runnableConfig = bcm_runnable_config,
    .runnableCount  = (uint8)BCM_RUNNABLE_COUNT,
};
