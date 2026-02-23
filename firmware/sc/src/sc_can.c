/**
 * @file    sc_can.c
 * @brief   DCAN1 listen-only CAN driver for Safety Controller
 * @date    2026-02-23
 *
 * @safety_req SWR-SC-001, SWR-SC-002, SWR-SC-023
 * @traces_to  SSR-SC-001, SSR-SC-002
 * @note    Safety level: ASIL D
 * @standard ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#include "sc_can.h"
#include "sc_cfg.h"
#include "sc_e2e.h"

/* ==================================================================
 * External: HALCoGen-style register access (provided by platform or mock)
 * ================================================================== */

extern uint32  dcan1_reg_read(uint32 offset);
extern void    dcan1_reg_write(uint32 offset, uint32 value);
extern boolean dcan1_get_mailbox_data(uint8 mbIndex, uint8* data, uint8* dlc);

/* ==================================================================
 * External: Heartbeat notification
 * ================================================================== */

extern void SC_Heartbeat_NotifyRx(uint8 ecuIndex);

/* ==================================================================
 * DCAN1 Register Offsets
 * ================================================================== */

#define DCAN_CTL_OFFSET         0x00u
#define DCAN_ES_OFFSET          0x04u
#define DCAN_BTR_OFFSET         0x08u
#define DCAN_TEST_OFFSET        0x10u
#define DCAN_NWDAT1_OFFSET      0x98u

/* DCAN Error Status bits */
#define DCAN_ES_BUSOFF          0x80u   /* Bit 7: Bus-off */
#define DCAN_ES_EPASS           0x20u   /* Bit 5: Error passive */

/* ==================================================================
 * Module State
 * ================================================================== */

/** Per-mailbox last received data */
static uint8  can_msg_data[SC_MB_COUNT][SC_CAN_DLC];

/** Per-mailbox DLC */
static uint8  can_msg_dlc[SC_MB_COUNT];

/** Per-mailbox data-valid flag */
static boolean can_msg_valid[SC_MB_COUNT];

/** Bus silence counter (10ms ticks) */
static uint16 bus_silence_counter;

/** Bus-off flag */
static boolean bus_off;

/** Initialization flag */
static boolean can_initialized;

/* ==================================================================
 * E2E Data ID lookup by mailbox index
 * ================================================================== */

static const uint8 mb_data_ids[SC_MB_COUNT] = {
    SC_E2E_ESTOP_DATA_ID,
    SC_E2E_CVC_HB_DATA_ID,
    SC_E2E_FZC_HB_DATA_ID,
    SC_E2E_RZC_HB_DATA_ID,
    SC_E2E_VEHSTATE_DATA_ID,
    SC_E2E_MOTORCUR_DATA_ID
};

/* ==================================================================
 * Public API
 * ================================================================== */

void SC_CAN_Init(void)
{
    uint8 i;
    uint8 j;

    /* Reset module state */
    for (i = 0u; i < SC_MB_COUNT; i++) {
        can_msg_valid[i] = FALSE;
        can_msg_dlc[i]   = 0u;
        for (j = 0u; j < SC_CAN_DLC; j++) {
            can_msg_data[i][j] = 0u;
        }
    }

    bus_silence_counter = 0u;
    bus_off             = FALSE;

    /* Configure DCAN1: enter init mode, set baud, set silent, exit init */
    /* Note: actual register writes are platform-specific; these calls
     * go through the extern abstraction for testability */
    dcan1_reg_write(DCAN_CTL_OFFSET, 0x01u);    /* Init mode */
    dcan1_reg_write(DCAN_BTR_OFFSET,
                    ((uint32)SC_DCAN_BRP) |
                    ((uint32)SC_DCAN_TSEG1 << 8u) |
                    ((uint32)SC_DCAN_TSEG2 << 12u));
    dcan1_reg_write(DCAN_TEST_OFFSET, 0x08u);   /* Silent bit */
    dcan1_reg_write(DCAN_CTL_OFFSET, 0x00u);    /* Exit init mode */

    can_initialized = TRUE;
}

void SC_CAN_Receive(void)
{
    uint8 mb;
    uint8 rx_data[SC_CAN_DLC];
    uint8 dlc;
    boolean has_data;
    boolean e2e_ok;
    boolean any_valid = FALSE;

    if (can_initialized == FALSE) {
        return;
    }

    for (mb = 0u; mb < SC_MB_COUNT; mb++) {
        has_data = dcan1_get_mailbox_data(mb, rx_data, &dlc);

        if (has_data == TRUE) {
            /* E2E validation */
            e2e_ok = SC_E2E_Check(rx_data, dlc, mb_data_ids[mb], mb);

            if (e2e_ok == TRUE) {
                /* Store validated data */
                uint8 i;
                for (i = 0u; i < SC_CAN_DLC; i++) {
                    can_msg_data[mb][i] = rx_data[i];
                }
                can_msg_dlc[mb]   = dlc;
                can_msg_valid[mb] = TRUE;
                any_valid         = TRUE;

                /* Notify heartbeat module for heartbeat mailboxes */
                if (mb == SC_MB_IDX_CVC_HB) {
                    SC_Heartbeat_NotifyRx(SC_ECU_CVC);
                } else if (mb == SC_MB_IDX_FZC_HB) {
                    SC_Heartbeat_NotifyRx(SC_ECU_FZC);
                } else if (mb == SC_MB_IDX_RZC_HB) {
                    SC_Heartbeat_NotifyRx(SC_ECU_RZC);
                } else {
                    /* Non-heartbeat mailbox — no heartbeat notification */
                }
            }
        }
    }

    /* Reset bus silence counter on any valid reception */
    if (any_valid == TRUE) {
        bus_silence_counter = 0u;
    }
}

void SC_CAN_MonitorBus(void)
{
    uint32 es;

    if (can_initialized == FALSE) {
        return;
    }

    /* Increment bus silence counter */
    if (bus_silence_counter < 0xFFFFu) {
        bus_silence_counter++;
    }

    /* Check DCAN error status register */
    es = dcan1_reg_read(DCAN_ES_OFFSET);
    if ((es & DCAN_ES_BUSOFF) != 0u) {
        bus_off = TRUE;
    } else {
        bus_off = FALSE;
    }
}

boolean SC_CAN_GetMessage(uint8 mbIndex, uint8* data, uint8* dlc)
{
    uint8 i;

    if ((mbIndex >= SC_MB_COUNT) || (data == NULL_PTR) || (dlc == NULL_PTR)) {
        return FALSE;
    }

    if (can_msg_valid[mbIndex] == FALSE) {
        return FALSE;
    }

    for (i = 0u; i < SC_CAN_DLC; i++) {
        data[i] = can_msg_data[mbIndex][i];
    }
    *dlc = can_msg_dlc[mbIndex];

    return TRUE;
}

boolean SC_CAN_IsBusSilent(void)
{
    return (bus_silence_counter >= SC_BUS_SILENCE_TICKS) ? TRUE : FALSE;
}

boolean SC_CAN_IsBusOff(void)
{
    return bus_off;
}
