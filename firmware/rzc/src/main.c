/**
 * @file    main.c
 * @brief   RZC main entry point — BSW init, self-test, 1ms tick loop
 * @date    2026-02-23
 *
 * @safety_req SWR-RZC-025 to SWR-RZC-030
 * @traces_to  SSR-RZC-013 to SSR-RZC-017, TSR-046, TSR-047, TSR-048
 *
 * @details  Initializes system clock, MPU, all BSW modules, all SWCs
 *           (Motor, CurrentMonitor, Encoder, TempMonitor, Battery,
 *           Heartbeat, RzcSafety), runs 8-item self-test, then enters
 *           the main loop which dispatches the RTE scheduler from a
 *           1ms SysTick.
 *
 *           Self-test items (SWR-RZC-025):
 *           1. Stack canary — stack overflow detection planted
 *           2. BTS7960 GPIO toggle — R_EN/L_EN verify
 *           3. ACS723 zero-cal — current sensor calibration
 *           4. NTC range check — temperature sensor plausibility
 *           5. Encoder stuck check — quadrature encoder responds
 *           6. CAN loopback — CAN controller self-test
 *           7. MPU verify — MPU regions configured correctly
 *           8. RAM pattern — memory integrity check
 *
 * @standard AUTOSAR, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#include "Std_Types.h"
#include "Rzc_Cfg.h"

/* ==================================================================
 * BSW Module Headers
 * ================================================================== */

#include "Can.h"
#include "CanIf.h"
#include "Com.h"
#include "E2E.h"
#include "Dem.h"
#include "WdgM.h"
#include "BswM.h"
#include "Dcm.h"
#include "Rte.h"
#include "IoHwAb.h"

/* ==================================================================
 * SWC Headers
 * ================================================================== */

#include "Swc_Motor.h"
#include "Swc_CurrentMonitor.h"
#include "Swc_Encoder.h"
#include "Swc_TempMonitor.h"
#include "Swc_Battery.h"
#include "Swc_Heartbeat.h"
#include "Swc_RzcSafety.h"

/* ==================================================================
 * External Configuration (defined in cfg/ files)
 * ================================================================== */

extern const Rte_ConfigType  rzc_rte_config;
extern const Com_ConfigType  rzc_com_config;
extern const Dcm_ConfigType  rzc_dcm_config;

/* ==================================================================
 * Hardware Abstraction Externs (implemented per platform)
 * ================================================================== */

extern void           Main_Hw_SystemClockInit(void);
extern void           Main_Hw_MpuConfig(void);
extern void           Main_Hw_SysTickInit(uint32 periodUs);
extern void           Main_Hw_Wfi(void);
extern uint32         Main_Hw_GetTick(void);

/* Self-test hardware externs */
extern Std_ReturnType Main_Hw_Bts7960GpioTest(void);
extern Std_ReturnType Main_Hw_Acs723ZeroCalTest(void);
extern Std_ReturnType Main_Hw_NtcRangeTest(void);
extern Std_ReturnType Main_Hw_EncoderStuckTest(void);
extern Std_ReturnType Main_Hw_CanLoopbackTest(void);
extern Std_ReturnType Main_Hw_MpuVerifyTest(void);
extern Std_ReturnType Main_Hw_RamPatternTest(void);
extern void           Main_Hw_PlantStackCanary(void);

/* ==================================================================
 * Static Configuration Constants
 * ================================================================== */

/** CAN driver configuration — 500 kbps, controller 0 */
static const Can_ConfigType can_config = {
    .baudrate     = 500000u,
    .controllerId = 0u,
};

/** CanIf TX PDU routing: Com TX PDU → CAN ID */
static const CanIf_TxPduConfigType canif_tx_config[] = {
    /* canId,  upperPduId,                  dlc, hth */
    { 0x012u, RZC_COM_TX_HEARTBEAT,       8u, 0u },  /* RZC heartbeat        */
    { 0x300u, RZC_COM_TX_MOTOR_STATUS,    8u, 0u },  /* Motor status         */
    { 0x301u, RZC_COM_TX_MOTOR_CURRENT,   8u, 0u },  /* Motor current        */
    { 0x302u, RZC_COM_TX_MOTOR_TEMP,      8u, 0u },  /* Motor temperature    */
    { 0x303u, RZC_COM_TX_BATTERY_STATUS,  8u, 0u },  /* Battery status       */
};

/** CanIf RX PDU routing: CAN ID → Com RX PDU */
static const CanIf_RxPduConfigType canif_rx_config[] = {
    /* canId,  upperPduId,                  dlc, isExtended */
    { 0x001u, RZC_COM_RX_ESTOP,           8u, FALSE },  /* E-stop broadcast     */
    { 0x100u, RZC_COM_RX_VEHICLE_TORQUE,  8u, FALSE },  /* Vehicle_State+Torque */
};

static const CanIf_ConfigType canif_config = {
    .txPduConfig = canif_tx_config,
    .txPduCount  = (uint8)(sizeof(canif_tx_config) / sizeof(canif_tx_config[0])),
    .rxPduConfig = canif_rx_config,
    .rxPduCount  = (uint8)(sizeof(canif_rx_config) / sizeof(canif_rx_config[0])),
};

/** IoHwAb channel mapping for RZC */
static const IoHwAb_ConfigType iohwab_config = {
    .MotorPwmRpwmCh  = 0u,   /* TIM1_CH1 (PA8) — BTS7960 RPWM (forward) */
    .MotorPwmLpwmCh  = 1u,   /* TIM1_CH2 (PA9) — BTS7960 LPWM (reverse) */
    .MotorCurrentAdcCh = 0u,  /* ADC1_CH0 (PA0) — ACS723 output           */
    .MotorTempAdcCh  = 1u,   /* ADC1_CH1 (PA1) — NTC thermistor           */
    .BatteryAdcCh    = 2u,   /* ADC1_CH2 (PA2) — voltage divider          */
    .EncoderTimCh    = 0u,   /* TIM4 encoder mode — PB6/PB7               */
    .EStopDioChannel = 3u,
    .WdiDioChannel   = RZC_SAFETY_WDI_CHANNEL,  /* PB4 TPS3823 WDI */
    .MotorREnChannel = RZC_MOTOR_R_EN_CHANNEL,   /* R_EN GPIO       */
    .MotorLEnChannel = RZC_MOTOR_L_EN_CHANNEL,   /* L_EN GPIO       */
};

/** WdgM supervised entity configuration */
static const WdgM_SupervisedEntityConfigType wdgm_se_config[] = {
    { 0u, 1u, 1u, 3u },   /* SE 0: Swc_Motor           — ASIL D, 3 failures tolerated */
    { 1u, 1u, 1u, 3u },   /* SE 1: Swc_CurrentMonitor  — ASIL A */
    { 2u, 1u, 1u, 3u },   /* SE 2: Swc_Encoder         — ASIL C */
    { 3u, 1u, 1u, 5u },   /* SE 3: Swc_TempMonitor     — ASIL A, more tolerant */
    { 4u, 1u, 1u, 5u },   /* SE 4: Swc_Battery         — QM, more tolerant */
    { 5u, 1u, 1u, 3u },   /* SE 5: Swc_Heartbeat       — ASIL C */
    { 6u, 1u, 1u, 3u },   /* SE 6: Swc_RzcSafety       — ASIL D */
};

static const WdgM_ConfigType wdgm_config = {
    .seConfig      = wdgm_se_config,
    .seCount       = (uint8)(sizeof(wdgm_se_config) / sizeof(wdgm_se_config[0])),
    .wdtDioChannel = RZC_SAFETY_WDI_CHANNEL,  /* PB4 — TPS3823 WDI */
};

/* ==================================================================
 * BswM Mode Actions (placeholder callbacks)
 * ================================================================== */

static void BswM_Action_Run(void)
{
    /* Enable BTS7960 R_EN/L_EN, start motor PWM output */
}

static void BswM_Action_SafeStop(void)
{
    /* Set PWM to 0%, disable BTS7960 R_EN/L_EN, latch safe state */
}

static void BswM_Action_Shutdown(void)
{
    /* Disable all motor outputs, stop watchdog feed */
}

static const BswM_ModeActionType bswm_actions[] = {
    { BSWM_RUN,       BswM_Action_Run      },
    { BSWM_SAFE_STOP, BswM_Action_SafeStop },
    { BSWM_SHUTDOWN,  BswM_Action_Shutdown  },
};

static const BswM_ConfigType bswm_config = {
    .ModeActions = bswm_actions,
    .ActionCount = (uint8)(sizeof(bswm_actions) / sizeof(bswm_actions[0])),
};

/* ==================================================================
 * Self-Test Sequence (SWR-RZC-025)
 * ================================================================== */

/**
 * @brief  Run RZC power-on self-test sequence (8 items)
 * @return RZC_SELF_TEST_PASS if all tests pass, RZC_SELF_TEST_FAIL otherwise
 *
 * @safety_req SWR-RZC-025
 * @traces_to  SSR-RZC-013
 */
static uint8 Main_RunSelfTest(void)
{
    /* Item 1: Plant stack canary for stack overflow detection */
    Main_Hw_PlantStackCanary();

    /* Item 2: BTS7960 GPIO toggle — verify R_EN/L_EN control path */
    if (Main_Hw_Bts7960GpioTest() != E_OK)
    {
        Dem_ReportErrorStatus(RZC_DTC_SELF_TEST_FAIL, DEM_EVENT_STATUS_FAILED);
        return RZC_SELF_TEST_FAIL;
    }

    /* Item 3: ACS723 zero-current calibration */
    if (Main_Hw_Acs723ZeroCalTest() != E_OK)
    {
        Dem_ReportErrorStatus(RZC_DTC_ZERO_CAL, DEM_EVENT_STATUS_FAILED);
        return RZC_SELF_TEST_FAIL;
    }

    /* Item 4: NTC range check — temperature sensor plausibility (-30..150°C) */
    if (Main_Hw_NtcRangeTest() != E_OK)
    {
        Dem_ReportErrorStatus(RZC_DTC_SELF_TEST_FAIL, DEM_EVENT_STATUS_FAILED);
        return RZC_SELF_TEST_FAIL;
    }

    /* Item 5: Encoder stuck check — verify quadrature encoder responds */
    if (Main_Hw_EncoderStuckTest() != E_OK)
    {
        Dem_ReportErrorStatus(RZC_DTC_ENCODER, DEM_EVENT_STATUS_FAILED);
        return RZC_SELF_TEST_FAIL;
    }

    /* Item 6: CAN loopback — CAN controller self-test */
    if (Main_Hw_CanLoopbackTest() != E_OK)
    {
        Dem_ReportErrorStatus(RZC_DTC_CAN_BUS_OFF, DEM_EVENT_STATUS_FAILED);
        return RZC_SELF_TEST_FAIL;
    }

    /* Item 7: MPU verify — memory protection regions configured */
    if (Main_Hw_MpuVerifyTest() != E_OK)
    {
        Dem_ReportErrorStatus(RZC_DTC_SELF_TEST_FAIL, DEM_EVENT_STATUS_FAILED);
        return RZC_SELF_TEST_FAIL;
    }

    /* Item 8: RAM pattern — memory integrity check */
    if (Main_Hw_RamPatternTest() != E_OK)
    {
        Dem_ReportErrorStatus(RZC_DTC_SELF_TEST_FAIL, DEM_EVENT_STATUS_FAILED);
        return RZC_SELF_TEST_FAIL;
    }

    return RZC_SELF_TEST_PASS;
}

/* ==================================================================
 * Tick Counters
 * ================================================================== */

static volatile uint32 tick_ms;

/* ==================================================================
 * Main Entry Point
 * ================================================================== */

/**
 * @brief  RZC main function — init, self-test, main loop
 *
 * @safety_req SWR-RZC-025 to SWR-RZC-030
 * @traces_to  SSR-RZC-013 to SSR-RZC-017, TSR-046, TSR-047, TSR-048
 */
int main(void)
{
    uint32 last_1ms   = 0u;
    uint32 last_10ms  = 0u;
    uint32 last_100ms = 0u;
    uint8  self_test_result;

    /* ---- Step 1: Hardware initialization ---- */
    Main_Hw_SystemClockInit();
    Main_Hw_MpuConfig();

    /* ---- Step 2: BSW module initialization (order matters) ---- */
    Can_Init(&can_config);
    CanIf_Init(&canif_config);
    Com_Init(&rzc_com_config);
    E2E_Init();
    Dem_Init(NULL_PTR);
    WdgM_Init(&wdgm_config);
    BswM_Init(&bswm_config);
    Dcm_Init(&rzc_dcm_config);
    IoHwAb_Init(&iohwab_config);
    Rte_Init(&rzc_rte_config);

    /* ---- Step 3: SWC initialization ---- */
    Swc_Motor_Init();
    Swc_CurrentMonitor_Init();
    Swc_Encoder_Init();
    Swc_TempMonitor_Init();
    Swc_Battery_Init();
    Swc_Heartbeat_Init();
    Swc_RzcSafety_Init();

    /* ---- Step 4: Self-test sequence (8 items, SWR-RZC-025) ---- */
    self_test_result = Main_RunSelfTest();

    /* Write self-test result to RTE for Swc_RzcSafety */
    (void)Rte_Write(RZC_SIG_SELF_TEST_RESULT, (uint32)self_test_result);

    /* ---- Step 5: Start CAN controller ---- */
    (void)Can_SetControllerMode(0u, CAN_CS_STARTED);

    /* ---- Step 6: Request BSW RUN mode (if self-test passed) ---- */
    if (self_test_result == RZC_SELF_TEST_PASS)
    {
        (void)BswM_RequestMode(0u, BSWM_RUN);
    }

    /* ---- Step 7: Start SysTick (1ms period = 1000us) ---- */
    Main_Hw_SysTickInit(1000u);

    /* ---- Step 8: Main loop ---- */
    for (;;)
    {
        Main_Hw_Wfi();

        tick_ms = Main_Hw_GetTick();

        /* 1ms task: RTE scheduler (dispatches runnables internally) */
        if ((tick_ms - last_1ms) >= 1u)
        {
            last_1ms = tick_ms;
            Rte_MainFunction();
        }

        /* 10ms tasks: Dcm, BswM */
        if ((tick_ms - last_10ms) >= 10u)
        {
            last_10ms = tick_ms;
            Dcm_MainFunction();
            BswM_MainFunction();
        }

        /* 100ms tasks: WdgM */
        if ((tick_ms - last_100ms) >= 100u)
        {
            last_100ms = tick_ms;
            WdgM_MainFunction();
        }
    }

    /* MISRA: unreachable but satisfies compiler */
    return 0;
}
