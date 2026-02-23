/**
 * @file    main.c
 * @brief   FZC main entry point — BSW init, self-test, 10ms tick loop
 * @date    2026-02-23
 *
 * @safety_req SWR-FZC-025 to SWR-FZC-032
 * @traces_to  SSR-FZC-019 to SSR-FZC-024, TSR-046, TSR-047, TSR-048
 *
 * @details  Initializes system clock, MPU, all BSW modules (including UART
 *           for TFMini-S lidar), all SWCs (Steering, Brake, Lidar, Heartbeat,
 *           FzcSafety, Buzzer), runs 7-item self-test, then enters the main
 *           loop which dispatches the RTE scheduler from a 1ms SysTick.
 *
 *           Self-test items (SWR-FZC-025):
 *           1. Servo neutral — steering PWM centers, brake releases
 *           2. SPI sensor — AS5048A steering angle sensor responds
 *           3. UART lidar handshake — TFMini-S UART data arrives
 *           4. CAN loopback — CAN controller self-test
 *           5. MPU verify — MPU regions configured correctly
 *           6. Stack canary — stack overflow detection planted
 *           7. RAM pattern — memory integrity check
 *
 * @standard AUTOSAR, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#include "Std_Types.h"
#include "Fzc_Cfg.h"

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
#include "Uart.h"

/* ==================================================================
 * SWC Headers
 * ================================================================== */

#include "Swc_Steering.h"
#include "Swc_Brake.h"
#include "Swc_Lidar.h"
#include "Swc_Heartbeat.h"
#include "Swc_FzcSafety.h"
#include "Swc_Buzzer.h"

/* ==================================================================
 * External Configuration (defined in cfg/ files)
 * ================================================================== */

extern const Rte_ConfigType  fzc_rte_config;
extern const Com_ConfigType  fzc_com_config;
extern const Dcm_ConfigType  fzc_dcm_config;

/* ==================================================================
 * Hardware Abstraction Externs (implemented per platform)
 * ================================================================== */

extern void           Main_Hw_SystemClockInit(void);
extern void           Main_Hw_MpuConfig(void);
extern void           Main_Hw_SysTickInit(uint32 periodUs);
extern void           Main_Hw_Wfi(void);
extern uint32         Main_Hw_GetTick(void);

/* Self-test hardware externs */
extern Std_ReturnType Main_Hw_ServoNeutralTest(void);
extern Std_ReturnType Main_Hw_SpiSensorTest(void);
extern Std_ReturnType Main_Hw_UartLidarTest(void);
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
    { 0x011u, FZC_COM_TX_HEARTBEAT,        8u, 0u },  /* FZC heartbeat        */
    { 0x200u, FZC_COM_TX_STEER_STATUS,     8u, 0u },  /* Steering status      */
    { 0x201u, FZC_COM_TX_BRAKE_STATUS,     8u, 0u },  /* Brake status         */
    { 0x210u, FZC_COM_TX_BRAKE_FAULT,      8u, 0u },  /* Brake fault          */
    { 0x211u, FZC_COM_TX_MOTOR_CUTOFF,     8u, 0u },  /* Motor cutoff request */
    { 0x220u, FZC_COM_TX_LIDAR,            8u, 0u },  /* Lidar data           */
};

/** CanIf RX PDU routing: CAN ID → Com RX PDU */
static const CanIf_RxPduConfigType canif_rx_config[] = {
    /* canId,  upperPduId,                  dlc, isExtended */
    { 0x001u, FZC_COM_RX_ESTOP,            8u, FALSE },  /* E-stop broadcast   */
    { 0x100u, FZC_COM_RX_VEHICLE_STATE,    8u, FALSE },  /* Vehicle state      */
    { 0x102u, FZC_COM_RX_STEER_CMD,        8u, FALSE },  /* Steering command   */
    { 0x103u, FZC_COM_RX_BRAKE_CMD,        8u, FALSE },  /* Brake command      */
};

static const CanIf_ConfigType canif_config = {
    .txPduConfig = canif_tx_config,
    .txPduCount  = (uint8)(sizeof(canif_tx_config) / sizeof(canif_tx_config[0])),
    .rxPduConfig = canif_rx_config,
    .rxPduCount  = (uint8)(sizeof(canif_rx_config) / sizeof(canif_rx_config[0])),
};

/** IoHwAb channel mapping for FZC */
static const IoHwAb_ConfigType iohwab_config = {
    .SteeringSpiChannel  = 0u,   /* SPI1 for AS5048A angle sensor */
    .SteeringCsChannel   = 0u,   /* CS pin for AS5048A            */
    .SteeringSpiSequence = 0u,
    .SteeringServoPwmCh  = 0u,   /* TIM2_CH1 (PA0) steering servo */
    .BrakeServoPwmCh     = 1u,   /* TIM2_CH2 (PA1) brake servo    */
    .EStopDioChannel     = 2u,
    .BuzzerDioChannel    = 8u,   /* PB8 buzzer output             */
    .WdiDioChannel       = 0u,   /* PB0 TPS3823 WDI pin           */
};

/** UART configuration for TFMini-S lidar (115200 baud, 8N1) */
static const Uart_ConfigType uart_config = {
    .baudRate  = 115200u,
    .dataBits  = 8u,
    .stopBits  = 1u,
    .parity    = 0u,   /* none */
    .timeoutMs = FZC_LIDAR_TIMEOUT_MS,
};

/** Steering SWC configuration */
static const Swc_Steering_ConfigType steering_config = {
    .plausThreshold   = FZC_STEER_PLAUS_THRESHOLD_DEG,
    .plausDebounce    = FZC_STEER_PLAUS_DEBOUNCE,
    .rateLimitDeg10ms = FZC_STEER_RATE_LIMIT_DEG_10MS,
    .cmdTimeoutMs     = FZC_STEER_CMD_TIMEOUT_MS,
    .rtcRateDegS      = FZC_STEER_RTC_RATE_DEG_S,
    .latchClearCycles = FZC_STEER_LATCH_CLEAR_CYCLES,
};

/** Brake SWC configuration */
static const Swc_Brake_ConfigType brake_config = {
    .autoTimeoutMs     = FZC_BRAKE_AUTO_TIMEOUT_MS,
    .pwmFaultThreshold = FZC_BRAKE_PWM_FAULT_THRESH,
    .faultDebounce     = FZC_BRAKE_FAULT_DEBOUNCE,
    .latchClearCycles  = FZC_BRAKE_LATCH_CLEAR_CYCLES,
    .cutoffRepeatCount = FZC_BRAKE_CUTOFF_REPEAT_COUNT,
};

/** Lidar SWC configuration */
static const Swc_Lidar_ConfigType lidar_config = {
    .warnDistCm      = FZC_LIDAR_WARN_CM,
    .brakeDistCm     = FZC_LIDAR_BRAKE_CM,
    .emergencyDistCm = FZC_LIDAR_EMERGENCY_CM,
    .timeoutMs       = FZC_LIDAR_TIMEOUT_MS,
    .stuckCycles     = FZC_LIDAR_STUCK_CYCLES,
    .rangeMinCm      = FZC_LIDAR_RANGE_MIN_CM,
    .rangeMaxCm      = FZC_LIDAR_RANGE_MAX_CM,
    .signalMin       = FZC_LIDAR_SIGNAL_MIN,
    .degradeCycles   = FZC_LIDAR_DEGRADE_CYCLES,
};

/** WdgM supervised entity configuration */
static const WdgM_SupervisedEntityConfigType wdgm_se_config[] = {
    { 0u, 1u, 1u, 3u },   /* SE 0: Swc_Steering    — ASIL D, 3 failures tolerated */
    { 1u, 1u, 1u, 3u },   /* SE 1: Swc_Brake       — ASIL D */
    { 2u, 1u, 1u, 3u },   /* SE 2: Swc_Lidar       — ASIL C */
    { 3u, 1u, 1u, 3u },   /* SE 3: Swc_Heartbeat   — ASIL C */
    { 4u, 1u, 1u, 3u },   /* SE 4: Swc_FzcSafety   — ASIL D */
    { 5u, 1u, 1u, 5u },   /* SE 5: Swc_Buzzer      — QM/ASIL B, more tolerant */
};

static const WdgM_ConfigType wdgm_config = {
    .seConfig      = wdgm_se_config,
    .seCount       = (uint8)(sizeof(wdgm_se_config) / sizeof(wdgm_se_config[0])),
    .wdtDioChannel = 0u,   /* PB0 — TPS3823 WDI (also driven by Swc_FzcSafety) */
};

/* ==================================================================
 * BswM Mode Actions (placeholder callbacks)
 * ================================================================== */

static void BswM_Action_Run(void)
{
    /* Enable steering servo, brake servo outputs */
}

static void BswM_Action_SafeStop(void)
{
    /* Center steering, apply max brake, disable motor cutoff */
}

static void BswM_Action_Shutdown(void)
{
    /* Disable all servo outputs, stop watchdog feed */
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
 * Self-Test Sequence (SWR-FZC-025)
 * ================================================================== */

/**
 * @brief  Run FZC power-on self-test sequence (7 items)
 * @return FZC_SELF_TEST_PASS if all tests pass, FZC_SELF_TEST_FAIL otherwise
 *
 * @safety_req SWR-FZC-025
 * @traces_to  SSR-FZC-019
 */
static uint8 Main_RunSelfTest(void)
{
    /* Item 1: Plant stack canary for stack overflow detection */
    Main_Hw_PlantStackCanary();

    /* Item 2: Servo neutral — steering centers, brake releases */
    if (Main_Hw_ServoNeutralTest() != E_OK)
    {
        Dem_ReportErrorStatus(FZC_DTC_SELF_TEST_FAIL, DEM_EVENT_STATUS_FAILED);
        return FZC_SELF_TEST_FAIL;
    }

    /* Item 3: SPI sensor — AS5048A steering angle sensor responds */
    if (Main_Hw_SpiSensorTest() != E_OK)
    {
        Dem_ReportErrorStatus(FZC_DTC_STEER_SPI_FAIL, DEM_EVENT_STATUS_FAILED);
        return FZC_SELF_TEST_FAIL;
    }

    /* Item 4: UART lidar handshake — TFMini-S data arrives */
    if (Main_Hw_UartLidarTest() != E_OK)
    {
        Dem_ReportErrorStatus(FZC_DTC_LIDAR_TIMEOUT, DEM_EVENT_STATUS_FAILED);
        return FZC_SELF_TEST_FAIL;
    }

    /* Item 5: CAN loopback — CAN controller self-test */
    if (Main_Hw_CanLoopbackTest() != E_OK)
    {
        Dem_ReportErrorStatus(FZC_DTC_CAN_BUS_OFF, DEM_EVENT_STATUS_FAILED);
        return FZC_SELF_TEST_FAIL;
    }

    /* Item 6: MPU verify — memory protection regions configured */
    if (Main_Hw_MpuVerifyTest() != E_OK)
    {
        Dem_ReportErrorStatus(FZC_DTC_SELF_TEST_FAIL, DEM_EVENT_STATUS_FAILED);
        return FZC_SELF_TEST_FAIL;
    }

    /* Item 7: RAM pattern — memory integrity check */
    if (Main_Hw_RamPatternTest() != E_OK)
    {
        Dem_ReportErrorStatus(FZC_DTC_SELF_TEST_FAIL, DEM_EVENT_STATUS_FAILED);
        return FZC_SELF_TEST_FAIL;
    }

    return FZC_SELF_TEST_PASS;
}

/* ==================================================================
 * Tick Counters
 * ================================================================== */

static volatile uint32 tick_ms;

/* ==================================================================
 * Main Entry Point
 * ================================================================== */

/**
 * @brief  FZC main function — init, self-test, main loop
 *
 * @safety_req SWR-FZC-025 to SWR-FZC-032
 * @traces_to  SSR-FZC-019 to SSR-FZC-024, TSR-046, TSR-047, TSR-048
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
    Com_Init(&fzc_com_config);
    E2E_Init();
    Dem_Init(NULL_PTR);
    WdgM_Init(&wdgm_config);
    BswM_Init(&bswm_config);
    Dcm_Init(&fzc_dcm_config);
    IoHwAb_Init(&iohwab_config);
    Uart_Init(&uart_config);   /* UART for TFMini-S lidar */
    Rte_Init(&fzc_rte_config);

    /* ---- Step 3: SWC initialization ---- */
    Swc_Steering_Init(&steering_config);
    Swc_Brake_Init(&brake_config);
    Swc_Lidar_Init(&lidar_config);
    Swc_Heartbeat_Init();
    Swc_FzcSafety_Init();
    Swc_Buzzer_Init();

    /* ---- Step 4: Self-test sequence (7 items, SWR-FZC-025) ---- */
    self_test_result = Main_RunSelfTest();

    /* Write self-test result to RTE for Swc_FzcSafety */
    (void)Rte_Write(FZC_SIG_SELF_TEST_RESULT, (uint32)self_test_result);

    /* ---- Step 5: Start CAN controller ---- */
    (void)Can_SetControllerMode(0u, CAN_CS_STARTED);

    /* ---- Step 6: Request BSW RUN mode (if self-test passed) ---- */
    if (self_test_result == FZC_SELF_TEST_PASS)
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

        /* 10ms tasks: Dcm, BswM, UART timeout monitoring */
        if ((tick_ms - last_10ms) >= 10u)
        {
            last_10ms = tick_ms;
            Dcm_MainFunction();
            BswM_MainFunction();
            Uart_MainFunction();
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
