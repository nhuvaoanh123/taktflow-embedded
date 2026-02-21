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

/* ====================================================================
 * External BSW interfaces
 * ==================================================================== */

extern Std_ReturnType Com_SendSignal(uint8 SignalId, const void* SignalDataPtr);
extern Std_ReturnType E2E_Protect(const void* Config, void* State,
                                  uint8* DataPtr, uint16 Length);
extern Std_ReturnType Rte_Write(uint16 SignalId, uint32 Data);
extern Std_ReturnType Rte_Read(uint16 SignalId, uint32* DataPtr);
extern void Dem_ReportErrorStatus(uint8 EventId, uint8 EventStatus);

/* ====================================================================
 * DEM event status (local definition to avoid Dem.h dependency)
 * ==================================================================== */

#define DEM_EVENT_STATUS_PASSED  0u
#define DEM_EVENT_STATUS_FAILED  1u

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

    fzc_comm_status = CVC_COMM_OK;
    rzc_comm_status = CVC_COMM_OK;

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

        /* FZC monitoring */
        if (fzc_rx_flag == TRUE) {
            /* Heartbeat received — clear miss count */
            fzc_miss_count = 0u;
            fzc_rx_flag    = FALSE;

            /* Recovery: if was timed out, restore */
            if (fzc_comm_status == CVC_COMM_TIMEOUT) {
                fzc_comm_status = CVC_COMM_OK;
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
