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
 * BSW Module Headers (MISRA 20.1: all #includes before any code)
 * ================================================================== */

#include "Adc.h"
#include "Can.h"
#include "CanIf.h"
#include "Com.h"
#include "PduR.h"
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
#include "Swc_RzcCom.h"
#include "Swc_RzcSafety.h"
#include "Swc_RzcSensorFeeder.h"

/* ==================================================================
 * Debug Logging (STM32 UART — compiled out on POSIX)
 * ================================================================== */

#ifdef PLATFORM_STM32
extern void Dbg_Uart_Print(const char *str);
#define DBG_LOG(msg)  Dbg_Uart_Print(msg)

/**
 * @brief  Print decimal uint32 to debug UART (for periodic status)
 * @param  val  Value to print
 */
static void Dbg_PrintU32(uint32 val)
{
    char buf[11]; /* max "4294967295\0" */
    char *p = &buf[10];
    *p = '\0';
    if (val == 0u)
    {
        p--;
        *p = '0';
    }
    else
    {
        while (val > 0u)
        {
            p--;
            *p = (char)('0' + (char)(val % 10u));
            val /= 10u;
        }
    }
    Dbg_Uart_Print(p);
}
#else
#define DBG_LOG(msg)  ((void)0)
#endif

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

/* Debug diagnostic externs */
extern uint8          Main_Hw_GetCanHalState(void);
extern void           Main_Hw_DumpCanDiag(void);

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
    { 0x500u, RZC_COM_TX_DTC_BROADCAST,  8u, 0u },  /* DTC broadcast        */
};

/** CanIf RX PDU routing: CAN ID → Com RX PDU */
static const CanIf_RxPduConfigType canif_rx_config[] = {
    /* canId,  upperPduId,                  dlc, isExtended */
    { 0x001u, RZC_COM_RX_ESTOP,           8u, FALSE },  /* E-stop broadcast     */
    { 0x100u, RZC_COM_RX_VEHICLE_TORQUE,  8u, FALSE },  /* Vehicle_State+Torque */
    { 0x601u, RZC_COM_RX_VIRT_SENSORS,   8u, FALSE },  /* Virtual sensors (SIL)*/
};

static const CanIf_ConfigType canif_config = {
    .txPduConfig = canif_tx_config,
    .txPduCount  = (uint8)(sizeof(canif_tx_config) / sizeof(canif_tx_config[0])),
    .rxPduConfig = canif_rx_config,
    .rxPduCount  = (uint8)(sizeof(canif_rx_config) / sizeof(canif_rx_config[0])),
    .e2eRxCheck  = Rzc_E2eRxCheck,
};

/** PduR RX routing: CanIf RX PDU ID → Com */
static const PduR_RoutingTableType rzc_pdur_routing[] = {
    { RZC_COM_RX_ESTOP,          PDUR_DEST_COM, RZC_COM_RX_ESTOP          },
    { RZC_COM_RX_VEHICLE_TORQUE, PDUR_DEST_COM, RZC_COM_RX_VEHICLE_TORQUE },
    { RZC_COM_RX_VIRT_SENSORS,   PDUR_DEST_COM, RZC_COM_RX_VIRT_SENSORS   },
};

static const PduR_ConfigType rzc_pdur_config = {
    .routingTable = rzc_pdur_routing,
    .routingCount = (uint8)(sizeof(rzc_pdur_routing) / sizeof(rzc_pdur_routing[0])),
};

/** ADC group configuration — motor current, motor temp, battery voltage */
static const Adc_GroupConfigType adc_groups[] = {
    { .numChannels = 1u, .triggerSource = 0u },  /* Group 0: motor current */
    { .numChannels = 1u, .triggerSource = 0u },  /* Group 1: motor temp    */
    { .numChannels = 1u, .triggerSource = 0u },  /* Group 2: battery volt  */
};

static const Adc_ConfigType adc_config = {
    .numGroups  = 3u,
    .groups     = adc_groups,
    .resolution = 12u,
};

/** IoHwAb channel mapping for RZC */
static const IoHwAb_ConfigType iohwab_config = {
    .MotorCurrentAdcGroup = 0u,  /* ADC group for motor current (ACS723)    */
    .MotorTempAdcGroup    = 1u,  /* ADC group for motor temperature (NTC)   */
    .BatteryVoltAdcGroup  = 2u,  /* ADC group for battery voltage (divider) */
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

static volatile uint32 tick_us;

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
    uint32 last_1ms_us   = 0u;
    uint32 last_10ms_us  = 0u;
    uint32 last_100ms_us = 0u;
#ifdef PLATFORM_STM32
    uint32 last_5s_us    = 0u;
#endif
    uint8  self_test_result;

    /* ---- Step 1: Hardware initialization ---- */
    Main_Hw_SystemClockInit();
    Main_Hw_MpuConfig();

    /* ---- Step 2: BSW module initialization (order matters) ---- */
    DBG_LOG("BSW init start\r\n");
    Can_Init(&can_config);
    DBG_LOG("CAN: FDCAN1 init OK\r\n");
    CanIf_Init(&canif_config);
    PduR_Init(&rzc_pdur_config);
    Com_Init(&rzc_com_config);
    E2E_Init();
    Dem_Init(NULL_PTR);
    Dem_SetEcuId(RZC_ECU_ID);                              /* 0x03 — RZC ECU ID */
    Dem_SetBroadcastPduId(RZC_COM_TX_DTC_BROADCAST);       /* CanIf TX for 0x500 */

    /* Remap DTC codes from CVC-centric defaults to RZC-specific codes */
    Dem_SetDtcCode(RZC_DTC_OVERCURRENT,    0x00E301u);     /* SG-001 overcurrent */
    Dem_SetDtcCode(RZC_DTC_OVERTEMP,       0x00E302u);     /* Motor overtemp */
    Dem_SetDtcCode(RZC_DTC_BATTERY,        0x00E401u);     /* SG-006 battery */
    Dem_SetDtcCode(RZC_DTC_STALL,          0x00E303u);     /* Motor stall */
    Dem_SetDtcCode(RZC_DTC_DIRECTION,      0x00E304u);     /* Direction mismatch */
    Dem_SetDtcCode(RZC_DTC_SHOOT_THROUGH,  0x00E305u);     /* H-bridge shoot-through */
    Dem_SetDtcCode(RZC_DTC_CAN_BUS_OFF,    0x00E601u);     /* CAN bus-off */
    Dem_SetDtcCode(RZC_DTC_CMD_TIMEOUT,    0x00E602u);     /* Command timeout */
    Dem_SetDtcCode(RZC_DTC_SELF_TEST_FAIL, 0x00E801u);     /* Self-test fail */
    Dem_SetDtcCode(RZC_DTC_WATCHDOG_FAIL,  0x00E802u);     /* Watchdog fail */
    Dem_SetDtcCode(RZC_DTC_ENCODER,        0x00E501u);     /* Encoder fault */
    Dem_SetDtcCode(RZC_DTC_ZERO_CAL,       0x00E502u);     /* Zero-cal fail */

    WdgM_Init(&wdgm_config);
    BswM_Init(&bswm_config);
    Dcm_Init(&rzc_dcm_config);
    Adc_Init(&adc_config);
    IoHwAb_Init(&iohwab_config);
    Rte_Init(&rzc_rte_config);

    /* ---- Step 3: SWC initialization ---- */
    Swc_Motor_Init();
    Swc_CurrentMonitor_Init();
    Swc_Encoder_Init();
    Swc_TempMonitor_Init();
    Swc_Battery_Init();
    Swc_Heartbeat_Init();
    Swc_RzcCom_Init();
    Swc_RzcSafety_Init();
    Swc_RzcSensorFeeder_Init();

    /* ---- Step 4: Self-test sequence (8 items, SWR-RZC-025) ---- */
    DBG_LOG("Self-test start\r\n");
    self_test_result = Main_RunSelfTest();
    DBG_LOG(self_test_result == RZC_SELF_TEST_PASS ? "Self-test: PASS\r\n" : "Self-test: FAIL\r\n");

    /* Write self-test result to RTE for Swc_RzcSafety */
    (void)Rte_Write(RZC_SIG_SELF_TEST_RESULT, (uint32)self_test_result);

    /* ---- Step 5: Start CAN controller ---- */
    (void)Can_SetControllerMode(0u, CAN_CS_STARTED);
    DBG_LOG("CAN: controller STARTED\r\n");

#ifdef PLATFORM_STM32
    /* Debug: test CAN TX immediately after start */
    {
        uint8 test_data[8] = {0xDE, 0xAD, 0xBE, 0xEF, 0x03, 0x00, 0x00, 0x00};
        Can_PduType test_pdu;
        test_pdu.id     = 0x012u;
        test_pdu.length = 8u;
        test_pdu.sdu    = test_data;
        Can_ReturnType tx_result = Can_Write(0u, &test_pdu);
        Dbg_Uart_Print("CAN TX test: ");
        Dbg_Uart_Print((tx_result == CAN_OK) ? "OK\r\n" : "FAIL\r\n");
        Main_Hw_DumpCanDiag();
    }
#endif

    /* ---- Step 6: Request BSW RUN mode (if self-test passed) ---- */
    if (self_test_result == RZC_SELF_TEST_PASS)
    {
        (void)BswM_RequestMode(0u, BSWM_RUN);
        DBG_LOG("BswM: STARTUP -> RUN\r\n");
    }

    /* ---- Step 7: Start SysTick (1ms period = 1000us) ---- */
    Main_Hw_SysTickInit(1000u);
    DBG_LOG("SysTick: 1ms — entering main loop\r\n");

    /* ---- Step 8: Main loop ---- */
    for (;;)
    {
        Main_Hw_Wfi();

        tick_us = Main_Hw_GetTick();

        /* 1ms task: RTE scheduler (dispatches runnables internally)
         * Main_Hw_GetTick() returns microseconds; 1ms = 1000us */
        if ((tick_us - last_1ms_us) >= 1000u)
        {
            last_1ms_us = tick_us;
            Rte_MainFunction();
        }

        /* 10ms tasks: Dcm, BswM */
        if ((tick_us - last_10ms_us) >= 10000u)
        {
            last_10ms_us = tick_us;
            Dcm_MainFunction();
            BswM_MainFunction();
        }

        /* 100ms tasks: WdgM, Dem (DTC broadcast) */
        if ((tick_us - last_100ms_us) >= 100000u)
        {
            last_100ms_us = tick_us;
            WdgM_MainFunction();
            Dem_MainFunction();
        }

#ifdef PLATFORM_STM32
        /* 5s debug task: CAN error counter + heartbeat alive print */
        if ((tick_us - last_5s_us) >= 5000000u)
        {
            /* g_can_tx_busy_count declared extern in Can.h (included at file scope) */
            uint8 tec = 0u;
            uint8 rec = 0u;
            uint8 err_state = 0u;
            uint32 hb_alive = 0u;

            last_5s_us = tick_us;

            (void)Can_GetErrorCounters(0u, &tec, &rec);
            (void)Can_GetControllerErrorState(0u, &err_state);
            (void)Rte_Read(RZC_SIG_HEARTBEAT_ALIVE, &hb_alive);

            Dbg_Uart_Print("[");
            Dbg_PrintU32(tick_us / 1000000u);
            Dbg_Uart_Print("s] RZC: TEC=");
            Dbg_PrintU32((uint32)tec);
            Dbg_Uart_Print(" REC=");
            Dbg_PrintU32((uint32)rec);
            Dbg_Uart_Print(" ERR=");
            Dbg_PrintU32((uint32)err_state);
            Dbg_Uart_Print(" HB=");
            Dbg_PrintU32(hb_alive);
            Dbg_Uart_Print(" HAL=");
            Dbg_PrintU32((uint32)Main_Hw_GetCanHalState());
            Dbg_Uart_Print(" TXbusy=");
            Dbg_PrintU32(g_can_tx_busy_count);
            Dbg_Uart_Print("\r\n");
        }
#endif
    }

    /* MISRA: unreachable but satisfies compiler */
    return 0;
}
