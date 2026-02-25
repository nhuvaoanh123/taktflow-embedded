/**
 * @file    Swc_Heartbeat.c
 * @brief   FZC heartbeat — 50ms CAN TX, alive counter, ECU ID, fault bitmask
 * @date    2026-02-23
 *
 * @safety_req SWR-FZC-021, SWR-FZC-022
 * @traces_to  SSR-FZC-021, SSR-FZC-022, TSR-022
 *
 * @details  Implements the FZC heartbeat SWC:
 *           1. Transmits heartbeat on CAN 0x011 every 50ms
 *           2. Alive counter 0-15, wraps around
 *           3. Includes ECU ID (0x02), vehicle state, fault bitmask
 *           4. Suppresses TX during CAN bus-off condition
 *
 *           All variables are static file-scope. No dynamic memory.
 *
 * @standard AUTOSAR SWC pattern, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */

#include "Swc_Heartbeat.h"
#include "Fzc_Cfg.h"

/* ==================================================================
 * BSW Includes
 * ================================================================== */

#include "Rte.h"
#include "Com.h"
#include "Dem.h"

/* ==================================================================
 * Constants
 * ================================================================== */

/** Heartbeat period in 1ms cycles: 50ms / 1ms = 50 */
#define HB_PERIOD_CYCLES    50u

/** Heartbeat TX data layout (8 bytes) */
#define HB_BYTE_ALIVE       0u
#define HB_BYTE_ECU_ID      1u
#define HB_BYTE_STATE       2u
#define HB_BYTE_FAULT_LO    3u
#define HB_BYTE_FAULT_HI    4u

/* ==================================================================
 * Module State
 * ================================================================== */

static uint8   Hb_Initialized;
static uint8   Hb_CycleCounter;
static uint8   Hb_AliveCounter;

/* ==================================================================
 * API: Swc_Heartbeat_Init
 * ================================================================== */

void Swc_Heartbeat_Init(void)
{
    Hb_CycleCounter = 0u;
    Hb_AliveCounter = 0u;
    Hb_Initialized  = TRUE;
}

/* ==================================================================
 * API: Swc_Heartbeat_MainFunction (1ms cyclic)
 * ================================================================== */

void Swc_Heartbeat_MainFunction(void)
{
    uint32 vehicle_state;
    uint32 fault_mask;
    uint8  tx_data[8];
    uint8  i;

    if (Hb_Initialized != TRUE) {
        return;
    }

    /* Increment cycle counter */
    Hb_CycleCounter++;

    if (Hb_CycleCounter < HB_PERIOD_CYCLES) {
        return;
    }

    /* Reset cycle counter */
    Hb_CycleCounter = 0u;

    /* Read current state from RTE */
    vehicle_state = FZC_STATE_INIT;
    (void)Rte_Read(FZC_SIG_VEHICLE_STATE, &vehicle_state);

    fault_mask = 0u;
    (void)Rte_Read(FZC_SIG_FAULT_MASK, &fault_mask);

    /* Suppress TX during bus-off (indicated by CAN bus-off bit in mask) */
    if ((fault_mask & (uint32)FZC_FAULT_CAN_BUS_OFF) != 0u) {
        return;
    }

    /* Build heartbeat payload */
    for (i = 0u; i < 8u; i++) {
        tx_data[i] = 0u;
    }

    tx_data[HB_BYTE_ALIVE]      = Hb_AliveCounter;
    tx_data[HB_BYTE_ECU_ID]     = FZC_ECU_ID;
    tx_data[HB_BYTE_STATE]      = (uint8)vehicle_state;
    tx_data[HB_BYTE_FAULT_LO]   = (uint8)(fault_mask & 0xFFu);
    tx_data[HB_BYTE_FAULT_HI]   = (uint8)((fault_mask >> 8u) & 0xFFu);

    /* Send via Com */
    (void)Com_SendSignal(FZC_COM_TX_HEARTBEAT, tx_data);

    /* Write alive counter to RTE for diagnostics */
    (void)Rte_Write(FZC_SIG_HEARTBEAT_ALIVE, (uint32)Hb_AliveCounter);

    /* Increment alive counter with wrap */
    Hb_AliveCounter++;
    if (Hb_AliveCounter > FZC_HB_ALIVE_MAX) {
        Hb_AliveCounter = 0u;
    }
}
