/**
 * @file    tcu_main.c
 * @brief   TCU main entry point -- BSW init, SWC init, 10ms main loop
 * @date    2026-02-23
 *
 * @safety_req SWR-TCU-001: TCU initialization and cyclic execution
 * @safety_req SWR-TCU-015: Graceful shutdown on SIGINT/SIGTERM
 * @traces_to  TSR-035, TSR-038, TSR-039, TSR-040, TSR-046, TSR-047
 *
 * @standard AUTOSAR BSW init sequence, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */

#include <signal.h>
#include <unistd.h>
#include <stdio.h>

#include "Std_Types.h"
#include "Can.h"
#include "CanIf.h"
#include "PduR.h"
#include "Com.h"
#include "Dcm.h"
#include "Dem.h"
#include "Rte.h"

#include "Swc_UdsServer.h"
#include "Swc_DtcStore.h"
#include "Swc_Obd2Pids.h"
#include "Tcu_Cfg.h"

/* ---- External Configuration ---- */

extern const Rte_ConfigType  tcu_rte_config;
extern const Com_ConfigType  tcu_com_config;
extern const Dcm_ConfigType  tcu_dcm_config;

/* ---- Shutdown Flag ---- */

static volatile boolean tcu_shutdown_requested = FALSE;

/**
 * @brief  Signal handler for SIGINT and SIGTERM
 */
static void tcu_signal_handler(int sig)
{
    (void)sig;
    tcu_shutdown_requested = TRUE;
}

/* ---- Main ---- */

int main(void)
{
    /* Install signal handlers for graceful shutdown */
    struct sigaction sa;
    sa.sa_handler = tcu_signal_handler;
    sa.sa_flags   = 0;
    sigemptyset(&sa.sa_mask);
    (void)sigaction(SIGINT,  &sa, NULL_PTR);
    (void)sigaction(SIGTERM, &sa, NULL_PTR);

    (void)printf("[TCU] Telematics Control Unit starting...\n");

    /* ---- BSW Initialization (AUTOSAR order) ---- */

    /* MCAL */
    Can_ConfigType can_cfg = {
        .baudrate     = 500000u,
        .controllerId = 0u,
    };
    Can_Init(&can_cfg);
    (void)Can_SetControllerMode(0u, CAN_CS_STARTED);

    /* ECUAL */
    CanIf_Init(NULL_PTR); /* TODO:POST-BETA -- provide full CanIf config */
    PduR_Init(NULL_PTR);  /* TODO:POST-BETA -- provide full PduR config  */

    /* Services */
    Com_Init(&tcu_com_config);
    Dcm_Init(&tcu_dcm_config);
    Dem_Init(NULL_PTR);

    /* RTE */
    Rte_Init(&tcu_rte_config);

    (void)printf("[TCU] BSW stack initialized\n");

    /* ---- SWC Initialization ---- */

    Swc_UdsServer_Init();
    Swc_DtcStore_Init();
    Swc_Obd2Pids_Init();

    (void)printf("[TCU] SWC layer initialized\n");
    (void)printf("[TCU] Entering main loop (10ms tick)\n");

    /* ---- Main Loop: 10ms tick ---- */

    while (tcu_shutdown_requested == FALSE) {
        Rte_MainFunction();
        usleep(10000u);  /* 10ms */
    }

    /* ---- Shutdown ---- */

    (void)Can_SetControllerMode(0u, CAN_CS_STOPPED);
    (void)printf("[TCU] Shutdown complete\n");

    return 0;
}
