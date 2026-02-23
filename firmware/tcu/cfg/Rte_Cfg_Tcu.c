/**
 * @file    Rte_Cfg_Tcu.c
 * @brief   RTE configuration for TCU -- signal mapping and runnable schedule
 * @date    2026-02-23
 *
 * @safety_req SWR-TCU-001
 * @traces_to  TSR-035, TSR-046, TSR-047
 *
 * @copyright Taktflow Systems 2026
 */

#include "Rte.h"
#include "Tcu_Cfg.h"

/* ---- Forward declarations of runnables ---- */

extern void Swc_UdsServer_10ms(void);
extern void Swc_DtcStore_10ms(void);
extern void Com_MainFunction_Tx(void);
extern void Can_MainFunction_Read(void);
extern void Can_MainFunction_BusOff(void);
extern void Dcm_MainFunction(void);

/* ---- Signal Configuration ---- */

static const Rte_SignalConfigType tcu_signal_config[TCU_SIG_COUNT] = {
    /* Signals 0..15 reserved for global well-known signals */
    [0]  = { .signalId =  0u, .initialValue = 0u },
    [1]  = { .signalId =  1u, .initialValue = 0u },
    [2]  = { .signalId =  2u, .initialValue = 0u },
    [3]  = { .signalId =  3u, .initialValue = 0u },
    [4]  = { .signalId =  4u, .initialValue = 0u },
    [5]  = { .signalId =  5u, .initialValue = 0u },
    [6]  = { .signalId =  6u, .initialValue = 0u },
    [7]  = { .signalId =  7u, .initialValue = 0u },
    [8]  = { .signalId =  8u, .initialValue = 0u },
    [9]  = { .signalId =  9u, .initialValue = 0u },
    [10] = { .signalId = 10u, .initialValue = 0u },
    [11] = { .signalId = 11u, .initialValue = 0u },
    [12] = { .signalId = 12u, .initialValue = 0u },
    [13] = { .signalId = 13u, .initialValue = 0u },
    [14] = { .signalId = 14u, .initialValue = 0u },
    [15] = { .signalId = 15u, .initialValue = 0u },

    /* TCU-specific signals */
    [TCU_SIG_VEHICLE_SPEED]   = { .signalId = TCU_SIG_VEHICLE_SPEED,   .initialValue = 0u     },
    [TCU_SIG_MOTOR_TEMP]      = { .signalId = TCU_SIG_MOTOR_TEMP,      .initialValue = 25u    },
    [TCU_SIG_BATTERY_VOLTAGE] = { .signalId = TCU_SIG_BATTERY_VOLTAGE, .initialValue = 48000u },
    [TCU_SIG_MOTOR_CURRENT]   = { .signalId = TCU_SIG_MOTOR_CURRENT,   .initialValue = 0u     },
    [TCU_SIG_TORQUE_PCT]      = { .signalId = TCU_SIG_TORQUE_PCT,      .initialValue = 0u     },
    [TCU_SIG_MOTOR_RPM]       = { .signalId = TCU_SIG_MOTOR_RPM,       .initialValue = 0u     },
    [TCU_SIG_DTC_BROADCAST]   = { .signalId = TCU_SIG_DTC_BROADCAST,   .initialValue = 0u     },
};

/* ---- Runnable Configuration ---- */

static const Rte_RunnableConfigType tcu_runnable_config[] = {
    /* SWC runnables -- 10ms period */
    {
        .func     = Swc_UdsServer_10ms,
        .periodMs = 10u,
        .priority = 3u,
        .seId     = 0u,
    },
    {
        .func     = Swc_DtcStore_10ms,
        .periodMs = 10u,
        .priority = 2u,
        .seId     = 0u,
    },

    /* BSW runnables -- 10ms period */
    {
        .func     = Can_MainFunction_Read,
        .periodMs = 10u,
        .priority = 5u,
        .seId     = 0u,
    },
    {
        .func     = Can_MainFunction_BusOff,
        .periodMs = 10u,
        .priority = 4u,
        .seId     = 0u,
    },
    {
        .func     = Dcm_MainFunction,
        .periodMs = 10u,
        .priority = 3u,
        .seId     = 0u,
    },
    {
        .func     = Com_MainFunction_Tx,
        .periodMs = 10u,
        .priority = 1u,
        .seId     = 0u,
    },
};

/* ---- Aggregate RTE Configuration ---- */

const Rte_ConfigType tcu_rte_config = {
    .signalConfig   = tcu_signal_config,
    .signalCount    = TCU_SIG_COUNT,
    .runnableConfig = tcu_runnable_config,
    .runnableCount  = (uint8)(sizeof(tcu_runnable_config) / sizeof(tcu_runnable_config[0])),
};
