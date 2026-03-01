/**
 * @file    bcm_main.c
 * @brief   BCM main entry point — BSW init, SWC init, 10ms tick loop
 * @date    2026-02-23
 *
 * @safety_req SWR-BCM-001, SWR-BCM-012
 * @traces_to  SSR-BCM-001, SSR-BCM-012, TSR-046, TSR-047, TSR-048
 *
 * @details  Simulated ECU running in Docker on Linux with SocketCAN.
 *           Initializes all BSW modules and SWCs, then enters a main loop
 *           with 10ms tick via usleep. Rte_MainFunction dispatches all
 *           configured runnables at their configured periods.
 *
 *           Handles SIGINT/SIGTERM for graceful shutdown.
 *
 * @standard AUTOSAR, ISO 26262 Part 6 (QM)
 * @copyright Taktflow Systems 2026
 */
#include "Std_Types.h"
#include "Bcm_Cfg.h"

/* ==================================================================
 * BSW Module Headers
 * ================================================================== */

#include "Can.h"
#include "CanIf.h"
#include "PduR.h"
#include "Com.h"
#include "Dem.h"
#include "Rte.h"

/* ==================================================================
 * SWC Headers
 * ================================================================== */

#include "Swc_Lights.h"
#include "Swc_Indicators.h"
#include "Swc_DoorLock.h"

/* ==================================================================
 * POSIX headers for simulated ECU
 * ================================================================== */

#include <signal.h>
#include <unistd.h>

/* ==================================================================
 * External Configuration (defined in cfg/ files)
 * ================================================================== */

extern const Rte_ConfigType  bcm_rte_config;
extern const Com_ConfigType  bcm_com_config;

/* ==================================================================
 * Static Configuration Constants
 * ================================================================== */

/** CAN driver configuration — 500 kbps, controller 0 */
static const Can_ConfigType can_config = {
    .baudrate     = 500000u,
    .controllerId = 0u,
};

/** CanIf TX PDU routing: Com TX PDU -> CAN ID */
static const CanIf_TxPduConfigType canif_tx_config[] = {
    /* canId,  upperPduId,                dlc, hth */
    { 0x400u, BCM_COM_TX_LIGHT_STATUS,     8u, 0u },  /* Light status       */
    { 0x401u, BCM_COM_TX_INDICATOR_STATE,  8u, 0u },  /* Indicator state    */
    { 0x402u, BCM_COM_TX_DOOR_LOCK,        8u, 0u },  /* Door lock state    */
};

/** CanIf RX PDU routing: CAN ID -> Com RX PDU */
static const CanIf_RxPduConfigType canif_rx_config[] = {
    /* canId,  upperPduId,              dlc, isExtended */
    { 0x100u, BCM_COM_RX_VEHICLE_STATE,  8u, FALSE },  /* Vehicle state from CVC */
    { 0x301u, BCM_COM_RX_MOTOR_CURRENT,  8u, FALSE },  /* Speed from RZC         */
    { 0x350u, BCM_COM_RX_BODY_CMD,       8u, FALSE },  /* Body cmd from CVC      */
};

static const CanIf_ConfigType canif_config = {
    .txPduConfig = canif_tx_config,
    .txPduCount  = (uint8)(sizeof(canif_tx_config) / sizeof(canif_tx_config[0])),
    .rxPduConfig = canif_rx_config,
    .rxPduCount  = (uint8)(sizeof(canif_rx_config) / sizeof(canif_rx_config[0])),
};

/** PduR RX routing: CanIf RX PDU ID → Com */
static const PduR_RoutingTableType bcm_pdur_routing[] = {
    { BCM_COM_RX_VEHICLE_STATE,  PDUR_DEST_COM, BCM_COM_RX_VEHICLE_STATE },
    { BCM_COM_RX_MOTOR_CURRENT,  PDUR_DEST_COM, BCM_COM_RX_MOTOR_CURRENT },
    { BCM_COM_RX_BODY_CMD,       PDUR_DEST_COM, BCM_COM_RX_BODY_CMD      },
};

static const PduR_ConfigType bcm_pdur_config = {
    .routingTable = bcm_pdur_routing,
    .routingCount = (uint8)(sizeof(bcm_pdur_routing) / sizeof(bcm_pdur_routing[0])),
};

/* ==================================================================
 * Graceful Shutdown
 * ================================================================== */

static volatile uint8 shutdown_requested;

static void Bcm_SignalHandler(int sig)
{
    (void)sig;
    shutdown_requested = 1u;
}

/* ==================================================================
 * Main Entry Point
 * ================================================================== */

/**
 * @brief  BCM main function — init, main loop, graceful shutdown
 *
 * @safety_req SWR-BCM-001, SWR-BCM-012
 * @traces_to  SSR-BCM-001, SSR-BCM-012, TSR-046, TSR-047, TSR-048
 */
int main(void)
{
    shutdown_requested = 0u;

    /* ---- Step 1: Register signal handlers for graceful shutdown ---- */
    (void)signal(SIGINT, Bcm_SignalHandler);
    (void)signal(SIGTERM, Bcm_SignalHandler);

    /* ---- Step 2: BSW module initialization (order matters) ---- */
    Can_Init(&can_config);
    CanIf_Init(&canif_config);
    PduR_Init(&bcm_pdur_config);
    Com_Init(&bcm_com_config);
    Dem_Init(NULL_PTR);
    Rte_Init(&bcm_rte_config);

    /* ---- Step 3: SWC initialization ---- */
    Swc_Lights_Init();
    Swc_Indicators_Init();
    Swc_DoorLock_Init();

    /* ---- Step 4: Start CAN controller ---- */
    (void)Can_SetControllerMode(0u, CAN_CS_STARTED);

    /* ---- Step 5: Main loop — 10ms tick via usleep ---- */
    while (shutdown_requested == 0u) {
        /* Sleep 10ms (10000 microseconds) */
        (void)usleep(10000u);

        /* RTE scheduler dispatches all configured runnables */
        Rte_MainFunction();
    }

    /* ---- Step 6: Graceful shutdown ---- */
    (void)Can_SetControllerMode(0u, CAN_CS_STOPPED);

    return 0;
}
