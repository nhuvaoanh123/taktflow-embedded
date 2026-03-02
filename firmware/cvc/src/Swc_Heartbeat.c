/**
 * @file    Swc_Heartbeat.c
 * @brief   Heartbeat TX and RX timeout monitoring
 * @date    2026-02-21
 *
 * @safety_req SWR-CVC-021, SWR-CVC-022
 * @traces_to  SSR-CVC-021, SSR-CVC-022, TSR-022, TSR-046
 *
 * @details  TX: Sends CVC heartbeat on CAN every 50ms containing alive
 *           counter (4-bit, wraps at 15), ECU ID, and vehicle state.
 *           E2E protection applied before transmission.
 *
 *           RX: Monitors FZC and RZC heartbeat reception. Uses a flag-based
 *           scheme — each 50ms check period, if no RxIndication was called,
 *           the miss counter increments. After CVC_HB_MAX_MISS consecutive
 *           misses, comm status transitions to TIMEOUT and a DTC is reported.
 *           Recovery: if an RxIndication arrives after timeout, comm status
 *           is restored to OK and a PASSED DTC is reported.
 *
 * @standard AUTOSAR, ISO 26262 Part 6 (ASIL D)
 * @copyright Taktflow Systems 2026
 */
#include "Swc_Heartbeat.h"
#include "Cvc_Cfg.h"
#include "Com.h"
#include "E2E.h"
#include "Rte.h"
#include "Dem.h"

/* SIL diagnostic logging — compile with -DSIL_DIAG to enable */
#ifdef SIL_DIAG
#include <stdio.h>
#define HB_DIAG(fmt, ...) (void)fprintf(stderr, "[HB] " fmt "\n", ##__VA_ARGS__)
#else
#define HB_DIAG(fmt, ...) ((void)0)
#endif

/* ====================================================================
 * Internal constants
 * ==================================================================== */

/** @brief TX timer threshold: 50ms / 10ms = 5 cycles */
#define HB_TX_CYCLES  5u

/** @brief Heartbeat PDU length in bytes */
#define HB_PDU_LENGTH 8u

/* ====================================================================
 * Static module state
 * ==================================================================== */

static uint8   alive_counter;
static uint8   tx_timer;

static uint8   fzc_miss_count;
static uint8   rzc_miss_count;

static boolean fzc_rx_flag;
static boolean rzc_rx_flag;

static uint8   fzc_comm_status;
static uint8   rzc_comm_status;

/** Last seen alive counter values from Com shadow buffers (poll-based detection) */
static uint8   fzc_last_alive;
static uint8   rzc_last_alive;

static boolean initialized;

/* ====================================================================
 * Public functions
 * ==================================================================== */

/**
 * @brief  Initialise all heartbeat state to safe defaults
 */
void Swc_Heartbeat_Init(void)
{
    alive_counter   = 0u;
    tx_timer        = 0u;

    fzc_miss_count  = 0u;
    rzc_miss_count  = 0u;

    fzc_rx_flag     = FALSE;
    rzc_rx_flag     = FALSE;

    fzc_comm_status = CVC_COMM_TIMEOUT;
    rzc_comm_status = CVC_COMM_TIMEOUT;

    fzc_last_alive  = 0u;   /* Match Com shadow buffer init (0) — no false positive */
    rzc_last_alive  = 0u;   /* Real detection starts when alive counter changes     */

    initialized     = TRUE;
}

/**
 * @brief  10ms cyclic — heartbeat TX and RX timeout monitoring
 *
 * @details Execution flow:
 *   1. TX: count cycles, transmit at 50ms intervals
 *   2. RX check: at each TX boundary, evaluate miss flags
 *   3. Timeout: report DTC when miss count reaches CVC_HB_MAX_MISS
 *   4. Recovery: restore OK status when RX flag set after timeout
 *   5. Write comm status to RTE every cycle
 */
void Swc_Heartbeat_MainFunction(void)
{
    if (initialized == FALSE) {
        return;
    }

    /* --- 1. TX timing --------------------------------------------- */
    tx_timer++;

    if (tx_timer >= HB_TX_CYCLES) {
        uint8  pdu[HB_PDU_LENGTH] = {0u};
        uint32 vehicle_state = 0u;

        /* Read current vehicle state from RTE */
        (void)Rte_Read(CVC_SIG_VEHICLE_STATE, &vehicle_state);

        /* Build heartbeat PDU */
        pdu[0] = alive_counter;
        pdu[1] = CVC_ECU_ID_CVC;
        pdu[2] = (uint8)vehicle_state;
        /* pdu[3..7] reserved (zero) */

        /* E2E protect then transmit */
        (void)E2E_Protect(NULL_PTR, NULL_PTR, pdu, HB_PDU_LENGTH);
        (void)Com_SendSignal(CVC_COM_TX_HEARTBEAT, pdu);

        /* Increment alive counter with wrap */
        alive_counter++;
        if (alive_counter > CVC_HB_ALIVE_MAX) {
            alive_counter = 0u;
        }

        /* Reset TX timer */
        tx_timer = 0u;

        /* --- 2. RX monitoring (checked at TX boundary = 50ms) ----- */

        /* Poll Com shadow buffers for alive counter changes.
         * Com_RxIndication unpacks heartbeat PDUs into shadow buffers
         * but does not call Swc_Heartbeat_RxIndication, so we detect
         * heartbeat reception by checking if the alive counter changed. */
        {
            uint8 fzc_alive = 0u;
            uint8 rzc_alive = 0u;

            (void)Com_ReceiveSignal(9u, &fzc_alive);   /* sig_rx_fzc_hb_alive */
            (void)Com_ReceiveSignal(11u, &rzc_alive);  /* sig_rx_rzc_hb_alive */

            if (fzc_alive != fzc_last_alive) {
                fzc_rx_flag    = TRUE;
                fzc_last_alive = fzc_alive;
            }

            if (rzc_alive != rzc_last_alive) {
                rzc_rx_flag    = TRUE;
                rzc_last_alive = rzc_alive;
            }
        }

        /* FZC monitoring */
        if (fzc_rx_flag == TRUE) {
            /* Heartbeat received — clear miss count */
            fzc_miss_count = 0u;
            fzc_rx_flag    = FALSE;

            /* Recovery: if was timed out, restore */
            if (fzc_comm_status == CVC_COMM_TIMEOUT) {
                fzc_comm_status = CVC_COMM_OK;
                HB_DIAG("FZC: TIMEOUT -> OK (alive=%u)", (unsigned)fzc_last_alive);
                Dem_ReportErrorStatus(CVC_DTC_CAN_FZC_TIMEOUT,
                                      DEM_EVENT_STATUS_PASSED);
            }
        } else {
            /* No heartbeat received this period */
            if (fzc_miss_count < 255u) {
                fzc_miss_count++;
            }

            if (fzc_miss_count >= CVC_HB_MAX_MISS) {
                if (fzc_comm_status != CVC_COMM_TIMEOUT) {
                    fzc_comm_status = CVC_COMM_TIMEOUT;
                    HB_DIAG("FZC: OK -> TIMEOUT (miss=%u)", (unsigned)fzc_miss_count);
                    Dem_ReportErrorStatus(CVC_DTC_CAN_FZC_TIMEOUT,
                                          DEM_EVENT_STATUS_FAILED);
                }
            }
        }

        /* RZC monitoring */
        if (rzc_rx_flag == TRUE) {
            /* Heartbeat received — clear miss count */
            rzc_miss_count = 0u;
            rzc_rx_flag    = FALSE;

            /* Recovery: if was timed out, restore */
            if (rzc_comm_status == CVC_COMM_TIMEOUT) {
                rzc_comm_status = CVC_COMM_OK;
                HB_DIAG("RZC: TIMEOUT -> OK (alive=%u)", (unsigned)rzc_last_alive);
                Dem_ReportErrorStatus(CVC_DTC_CAN_RZC_TIMEOUT,
                                      DEM_EVENT_STATUS_PASSED);
            }
        } else {
            /* No heartbeat received this period */
            if (rzc_miss_count < 255u) {
                rzc_miss_count++;
            }

            if (rzc_miss_count >= CVC_HB_MAX_MISS) {
                if (rzc_comm_status != CVC_COMM_TIMEOUT) {
                    rzc_comm_status = CVC_COMM_TIMEOUT;
                    HB_DIAG("RZC: OK -> TIMEOUT (miss=%u)", (unsigned)rzc_miss_count);
                    Dem_ReportErrorStatus(CVC_DTC_CAN_RZC_TIMEOUT,
                                          DEM_EVENT_STATUS_FAILED);
                }
            }
        }
    }

    /* --- 5. Write comm status to RTE every cycle ------------------ */
    (void)Rte_Write(CVC_SIG_FZC_COMM_STATUS, (uint32)fzc_comm_status);
    (void)Rte_Write(CVC_SIG_RZC_COMM_STATUS, (uint32)rzc_comm_status);
}

/**
 * @brief  RX indication — called by Com layer when heartbeat received
 * @param  ecuId  Source ECU ID (CVC_ECU_ID_FZC or CVC_ECU_ID_RZC)
 */
void Swc_Heartbeat_RxIndication(uint8 ecuId)
{
    if (ecuId == CVC_ECU_ID_FZC) {
        fzc_rx_flag = TRUE;
    } else if (ecuId == CVC_ECU_ID_RZC) {
        rzc_rx_flag = TRUE;
    } else {
        /* Unknown ECU ID — ignore */
    }
}
