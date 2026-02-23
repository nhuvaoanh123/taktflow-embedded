/**
 * @file    Rte_Cfg_Icu.c
 * @brief   RTE configuration for ICU — signal table and runnable table
 * @date    2026-02-23
 *
 * @safety_req SWR-ICU-001 to SWR-ICU-010
 * @traces_to  SSR-ICU-001 to SSR-ICU-010, TSR-046, TSR-047
 *
 * @standard AUTOSAR RTE, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#include "Rte.h"
#include "Icu_Cfg.h"

/* ==================================================================
 * Forward declarations for SWC runnables
 * ================================================================== */

extern void Swc_Dashboard_50ms(void);
extern void Swc_DtcDisplay_50ms(void);
extern void Com_MainFunction_Tx(void);
extern void Can_MainFunction_Read(void);
extern void Can_MainFunction_BusOff(void);

/* ==================================================================
 * Signal Configuration Table
 * ================================================================== */

static const Rte_SignalConfigType icu_signal_config[ICU_SIG_COUNT] = {
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

    /* ICU application signals (16-28) */
    { ICU_SIG_MOTOR_RPM,         0u },   /* 16: Motor RPM from 0x301         */
    { ICU_SIG_TORQUE_PCT,        0u },   /* 17: Torque percentage            */
    { ICU_SIG_MOTOR_TEMP,        0u },   /* 18: Motor temperature (C)        */
    { ICU_SIG_BATTERY_VOLTAGE,   0u },   /* 19: Battery voltage (mV)         */
    { ICU_SIG_VEHICLE_STATE,     0u },   /* 20: Vehicle state enum           */
    { ICU_SIG_ESTOP_ACTIVE,      0u },   /* 21: E-stop active flag           */
    { ICU_SIG_HEARTBEAT_CVC,     0u },   /* 22: CVC heartbeat alive counter  */
    { ICU_SIG_HEARTBEAT_FZC,     0u },   /* 23: FZC heartbeat alive counter  */
    { ICU_SIG_HEARTBEAT_RZC,     0u },   /* 24: RZC heartbeat alive counter  */
    { ICU_SIG_OVERCURRENT_FLAG,  0u },   /* 25: Overcurrent flag             */
    { ICU_SIG_LIGHT_STATUS,      0u },   /* 26: Light status from BCM        */
    { ICU_SIG_INDICATOR_STATE,   0u },   /* 27: Turn indicator state         */
    { ICU_SIG_DTC_BROADCAST,     0u },   /* 28: DTC broadcast from 0x500     */
};

/* ==================================================================
 * Runnable Configuration Table
 * Priority: higher number = executes first within same period
 *
 * ICU runs at 50ms base tick (20 Hz). CAN read at 10ms for
 * timely message processing.
 * ================================================================== */

static const Rte_RunnableConfigType icu_runnable_config[] = {
    /* func,                    periodMs, priority, seId */
    { Can_MainFunction_Read,       10u,      5u,     0xFFu },  /* CAN RX first        */
    { Com_MainFunction_Tx,         10u,      4u,     0xFFu },  /* COM TX (no-op, ICU has no TX) */
    { Swc_Dashboard_50ms,          50u,      3u,     0u    },  /* Dashboard (20 Hz)    */
    { Swc_DtcDisplay_50ms,         50u,      2u,     1u    },  /* DTC display (20 Hz)  */
    { Can_MainFunction_BusOff,     50u,      1u,     0xFFu },  /* Bus-off check        */
};

#define ICU_RUNNABLE_COUNT  (sizeof(icu_runnable_config) / sizeof(icu_runnable_config[0]))

/* ==================================================================
 * Aggregate RTE Configuration
 * ================================================================== */

const Rte_ConfigType icu_rte_config = {
    .signalConfig   = icu_signal_config,
    .signalCount    = ICU_SIG_COUNT,
    .runnableConfig = icu_runnable_config,
    .runnableCount  = (uint8)ICU_RUNNABLE_COUNT,
};
