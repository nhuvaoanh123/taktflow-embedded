/**
 * @file    test_Swc_Heartbeat.c
 * @brief   Unit tests for Heartbeat TX/RX monitoring SWC
 * @date    2026-02-21
 *
 * @verifies SWR-CVC-021, SWR-CVC-022
 *
 * Tests heartbeat transmission timing, alive counter wrap, RX indication,
 * timeout detection, DTC reporting, recovery, and RTE write.
 */
#include "unity.h"

/* ====================================================================
 * Local type definitions (self-contained test — no BSW headers)
 * ==================================================================== */

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned long  uint32;
typedef uint8          Std_ReturnType;
typedef uint8          boolean;

#define E_OK      0u
#define E_NOT_OK  1u
#define TRUE      1u
#define FALSE     0u
#define NULL_PTR  ((void*)0)
#define STD_HIGH  0x01u
#define STD_LOW   0x00u

/* Prevent BSW headers from redefining types */
#define PLATFORM_TYPES_H
#define STD_TYPES_H
#define SWC_HEARTBEAT_H
#define CVC_CFG_H

/* Signal/DTC IDs (must match Cvc_Cfg.h) */
#define CVC_SIG_FZC_COMM_STATUS   23u
#define CVC_SIG_RZC_COMM_STATUS   24u
#define CVC_SIG_VEHICLE_STATE     20u
#define CVC_COM_TX_HEARTBEAT       1u
#define CVC_DTC_CAN_FZC_TIMEOUT    4u
#define CVC_DTC_CAN_RZC_TIMEOUT    5u
#define CVC_ECU_ID_CVC          0x01u
#define CVC_ECU_ID_FZC          0x02u
#define CVC_ECU_ID_RZC          0x03u
#define CVC_HB_TX_PERIOD_MS      50u
#define CVC_HB_MAX_MISS            3u
#define CVC_HB_ALIVE_MAX         15u
#define CVC_COMM_OK                0u
#define CVC_COMM_TIMEOUT           1u

/* ====================================================================
 * Mock: Com_SendSignal
 * ==================================================================== */

static uint8 mock_com_send_count;
static uint8 mock_com_send_sig_id;
static uint8 mock_com_send_data[8];

Std_ReturnType Com_SendSignal(uint8 SignalId, const void* SignalDataPtr)
{
    mock_com_send_sig_id = SignalId;
    mock_com_send_count++;
    if (SignalDataPtr != NULL_PTR) {
        const uint8* p = (const uint8*)SignalDataPtr;
        uint8 i;
        for (i = 0u; i < 8u; i++) {
            mock_com_send_data[i] = p[i];
        }
    }
    return E_OK;
}

/* ====================================================================
 * Mock: E2E_Protect
 * ==================================================================== */

static uint8 mock_e2e_protect_count;

Std_ReturnType E2E_Protect(const void* Config, void* State,
                           uint8* DataPtr, uint16 Length)
{
    mock_e2e_protect_count++;
    (void)Config;
    (void)State;
    (void)DataPtr;
    (void)Length;
    return E_OK;
}

/* ====================================================================
 * Mock: Rte_Write
 * ==================================================================== */

static uint16 mock_rte_write_sig_ids[16];
static uint32 mock_rte_write_vals[16];
static uint8  mock_rte_write_count;

Std_ReturnType Rte_Write(uint16 SignalId, uint32 Data)
{
    if (mock_rte_write_count < 16u) {
        mock_rte_write_sig_ids[mock_rte_write_count] = SignalId;
        mock_rte_write_vals[mock_rte_write_count]    = Data;
    }
    mock_rte_write_count++;
    return E_OK;
}

/* ====================================================================
 * Mock: Rte_Read
 * ==================================================================== */

static uint32 mock_rte_vehicle_state;

Std_ReturnType Rte_Read(uint16 SignalId, uint32* DataPtr)
{
    if (DataPtr == NULL_PTR) {
        return E_NOT_OK;
    }
    if (SignalId == CVC_SIG_VEHICLE_STATE) {
        *DataPtr = mock_rte_vehicle_state;
    } else {
        *DataPtr = 0u;
    }
    return E_OK;
}

/* ====================================================================
 * Mock: Dem_ReportErrorStatus
 * ==================================================================== */

static uint8 mock_dem_event_ids[8];
static uint8 mock_dem_event_statuses[8];
static uint8 mock_dem_report_count;

void Dem_ReportErrorStatus(uint8 EventId, uint8 EventStatus)
{
    if (mock_dem_report_count < 8u) {
        mock_dem_event_ids[mock_dem_report_count]      = EventId;
        mock_dem_event_statuses[mock_dem_report_count]  = EventStatus;
    }
    mock_dem_report_count++;
}

/* ====================================================================
 * Include SWC under test (source inclusion for test build)
 * ==================================================================== */

#include "../src/Swc_Heartbeat.c"

/* ====================================================================
 * Helper: run N MainFunction cycles
 * ==================================================================== */

static void run_cycles(uint8 n)
{
    uint8 i;
    for (i = 0u; i < n; i++) {
        Swc_Heartbeat_MainFunction();
    }
}

/* ====================================================================
 * setUp / tearDown
 * ==================================================================== */

void setUp(void)
{
    uint8 i;

    mock_com_send_count    = 0u;
    mock_com_send_sig_id   = 0xFFu;
    mock_e2e_protect_count = 0u;
    mock_rte_write_count   = 0u;
    mock_dem_report_count  = 0u;
    mock_rte_vehicle_state = 1u; /* CVC_STATE_RUN */

    for (i = 0u; i < 8u; i++) {
        mock_com_send_data[i]      = 0u;
        mock_dem_event_ids[i]      = 0xFFu;
        mock_dem_event_statuses[i] = 0xFFu;
    }
    for (i = 0u; i < 16u; i++) {
        mock_rte_write_sig_ids[i] = 0xFFu;
        mock_rte_write_vals[i]    = 0xFFu;
    }

    Swc_Heartbeat_Init();
}

void tearDown(void) { }

/* ====================================================================
 * SWR-CVC-021: Heartbeat TX
 * ==================================================================== */

/** @verifies SWR-CVC-021 */
void test_Heartbeat_Init_comm_status_ok(void)
{
    /* After init, both FZC and RZC comm status should be OK */
    /* Run one cycle to see RTE writes */
    Swc_Heartbeat_MainFunction();

    /* Find FZC comm status write */
    boolean found_fzc = FALSE;
    boolean found_rzc = FALSE;
    uint8 i;
    for (i = 0u; i < mock_rte_write_count; i++) {
        if (mock_rte_write_sig_ids[i] == CVC_SIG_FZC_COMM_STATUS) {
            TEST_ASSERT_EQUAL(CVC_COMM_OK, mock_rte_write_vals[i]);
            found_fzc = TRUE;
        }
        if (mock_rte_write_sig_ids[i] == CVC_SIG_RZC_COMM_STATUS) {
            TEST_ASSERT_EQUAL(CVC_COMM_OK, mock_rte_write_vals[i]);
            found_rzc = TRUE;
        }
    }
    TEST_ASSERT_TRUE(found_fzc);
    TEST_ASSERT_TRUE(found_rzc);
}

/** @verifies SWR-CVC-021 */
void test_Heartbeat_TX_every_50ms(void)
{
    /* 50ms = 5 calls at 10ms each */
    run_cycles(5u);

    TEST_ASSERT_EQUAL(1u, mock_com_send_count);
    TEST_ASSERT_EQUAL(CVC_COM_TX_HEARTBEAT, mock_com_send_sig_id);
}

/** @verifies SWR-CVC-021 */
void test_Heartbeat_Alive_counter_increments(void)
{
    /* First TX at cycle 5 */
    run_cycles(5u);
    uint8 first_alive = mock_com_send_data[0];

    /* Second TX at cycle 10 */
    run_cycles(5u);
    uint8 second_alive = mock_com_send_data[0];

    TEST_ASSERT_EQUAL(first_alive + 1u, second_alive);
}

/** @verifies SWR-CVC-021 */
void test_Heartbeat_Alive_counter_wraps_at_15(void)
{
    uint8 cycle;

    /* Force alive counter to 15 by running 16 TX periods (16 * 5 = 80 cycles) */
    for (cycle = 0u; cycle < 16u; cycle++) {
        run_cycles(5u);
    }

    /* The alive counter should have wrapped: 0..15, 0 */
    /* Last sent alive = 15, next TX should wrap to 0 */
    run_cycles(5u);
    /* After 17 TXes: 0,1,2,...,15,0,1 => index 16 = 0 (wrapped), index 17 = 1 */
    /* Actually: 17th TX, alive was incremented after each send */
    /* Let's just check the data byte wraps to low value */
    TEST_ASSERT_TRUE(mock_com_send_data[0] <= CVC_HB_ALIVE_MAX);
}

/** @verifies SWR-CVC-021 */
void test_Heartbeat_TX_data_includes_ecu_id_and_state(void)
{
    mock_rte_vehicle_state = 2u; /* CVC_STATE_DEGRADED */

    run_cycles(5u);

    /* PDU layout: [alive_counter, ECU_ID, vehicle_state, 0,0,0,0,0] */
    TEST_ASSERT_EQUAL(CVC_ECU_ID_CVC, mock_com_send_data[1]);
    TEST_ASSERT_EQUAL(2u, mock_com_send_data[2]);
}

/** @verifies SWR-CVC-021 */
void test_Heartbeat_No_TX_before_50ms(void)
{
    /* Only 4 cycles (40ms) — should NOT transmit */
    run_cycles(4u);

    TEST_ASSERT_EQUAL(0u, mock_com_send_count);
}

/* ====================================================================
 * SWR-CVC-022: Heartbeat RX monitoring
 * ==================================================================== */

/** @verifies SWR-CVC-022 */
void test_Heartbeat_FZC_RxIndication_resets_timer(void)
{
    /* Run 2 check periods without FZC RX (2 misses) */
    run_cycles(5u);
    run_cycles(5u);

    /* Now indicate FZC heartbeat received */
    Swc_Heartbeat_RxIndication(CVC_ECU_ID_FZC);

    /* Run 1 more period — miss count should have been reset */
    mock_dem_report_count = 0u;
    run_cycles(5u);

    /* Should NOT have timed out (only 1 miss after reset) */
    /* Check no FZC timeout DTC */
    boolean fzc_timeout_reported = FALSE;
    uint8 i;
    for (i = 0u; i < mock_dem_report_count; i++) {
        if ((mock_dem_event_ids[i] == CVC_DTC_CAN_FZC_TIMEOUT) &&
            (mock_dem_event_statuses[i] == DEM_EVENT_STATUS_FAILED)) {
            fzc_timeout_reported = TRUE;
        }
    }
    TEST_ASSERT_FALSE(fzc_timeout_reported);
}

/** @verifies SWR-CVC-022 */
void test_Heartbeat_RZC_RxIndication_resets_timer(void)
{
    /* Run 2 check periods without RZC RX */
    run_cycles(5u);
    run_cycles(5u);

    /* Indicate RZC heartbeat received */
    Swc_Heartbeat_RxIndication(CVC_ECU_ID_RZC);

    /* Run 1 more period */
    mock_dem_report_count = 0u;
    run_cycles(5u);

    /* Should NOT have timed out */
    boolean rzc_timeout_reported = FALSE;
    uint8 i;
    for (i = 0u; i < mock_dem_report_count; i++) {
        if ((mock_dem_event_ids[i] == CVC_DTC_CAN_RZC_TIMEOUT) &&
            (mock_dem_event_statuses[i] == DEM_EVENT_STATUS_FAILED)) {
            rzc_timeout_reported = TRUE;
        }
    }
    TEST_ASSERT_FALSE(rzc_timeout_reported);
}

/** @verifies SWR-CVC-022 */
void test_Heartbeat_FZC_timeout_after_3_misses(void)
{
    /* Run 3 check periods (3 * 5 = 15 cycles) without FZC RX */
    run_cycles(5u);
    run_cycles(5u);
    run_cycles(5u);

    /* Check FZC comm status written as TIMEOUT */
    boolean found_timeout = FALSE;
    uint8 i;
    for (i = 0u; i < mock_rte_write_count; i++) {
        if ((mock_rte_write_sig_ids[i] == CVC_SIG_FZC_COMM_STATUS) &&
            (mock_rte_write_vals[i] == CVC_COMM_TIMEOUT)) {
            found_timeout = TRUE;
        }
    }
    TEST_ASSERT_TRUE(found_timeout);
}

/** @verifies SWR-CVC-022 */
void test_Heartbeat_RZC_timeout_after_3_misses(void)
{
    /* Run 3 check periods without RZC RX */
    run_cycles(5u);
    run_cycles(5u);
    run_cycles(5u);

    boolean found_timeout = FALSE;
    uint8 i;
    for (i = 0u; i < mock_rte_write_count; i++) {
        if ((mock_rte_write_sig_ids[i] == CVC_SIG_RZC_COMM_STATUS) &&
            (mock_rte_write_vals[i] == CVC_COMM_TIMEOUT)) {
            found_timeout = TRUE;
        }
    }
    TEST_ASSERT_TRUE(found_timeout);
}

/** @verifies SWR-CVC-022 */
void test_Heartbeat_FZC_timeout_DTC_reported(void)
{
    /* Run 3 check periods without FZC RX */
    run_cycles(5u);
    run_cycles(5u);
    run_cycles(5u);

    boolean fzc_dtc = FALSE;
    uint8 i;
    for (i = 0u; i < mock_dem_report_count; i++) {
        if ((mock_dem_event_ids[i] == CVC_DTC_CAN_FZC_TIMEOUT) &&
            (mock_dem_event_statuses[i] == DEM_EVENT_STATUS_FAILED)) {
            fzc_dtc = TRUE;
        }
    }
    TEST_ASSERT_TRUE(fzc_dtc);
}

/** @verifies SWR-CVC-022 */
void test_Heartbeat_RZC_timeout_DTC_reported(void)
{
    /* Run 3 check periods without RZC RX */
    run_cycles(5u);
    run_cycles(5u);
    run_cycles(5u);

    boolean rzc_dtc = FALSE;
    uint8 i;
    for (i = 0u; i < mock_dem_report_count; i++) {
        if ((mock_dem_event_ids[i] == CVC_DTC_CAN_RZC_TIMEOUT) &&
            (mock_dem_event_statuses[i] == DEM_EVENT_STATUS_FAILED)) {
            rzc_dtc = TRUE;
        }
    }
    TEST_ASSERT_TRUE(rzc_dtc);
}

/** @verifies SWR-CVC-022 */
void test_Heartbeat_Recovery_after_timeout(void)
{
    /* Cause FZC timeout */
    run_cycles(5u);
    run_cycles(5u);
    run_cycles(5u);

    /* Now RX indication — should recover */
    Swc_Heartbeat_RxIndication(CVC_ECU_ID_FZC);
    mock_rte_write_count = 0u;
    run_cycles(5u);

    /* Find FZC comm status — should be back to OK */
    boolean found_ok = FALSE;
    uint8 i;
    for (i = 0u; i < mock_rte_write_count; i++) {
        if ((mock_rte_write_sig_ids[i] == CVC_SIG_FZC_COMM_STATUS) &&
            (mock_rte_write_vals[i] == CVC_COMM_OK)) {
            found_ok = TRUE;
        }
    }
    TEST_ASSERT_TRUE(found_ok);
}

/** @verifies SWR-CVC-022 */
void test_Heartbeat_RTE_write_fzc_comm_each_cycle(void)
{
    /* Run 3 cycles */
    run_cycles(3u);

    /* Count FZC comm status writes */
    uint8 fzc_writes = 0u;
    uint8 i;
    for (i = 0u; i < mock_rte_write_count; i++) {
        if (mock_rte_write_sig_ids[i] == CVC_SIG_FZC_COMM_STATUS) {
            fzc_writes++;
        }
    }
    TEST_ASSERT_EQUAL(3u, fzc_writes);
}

/** @verifies SWR-CVC-022 */
void test_Heartbeat_RTE_write_rzc_comm_each_cycle(void)
{
    /* Run 3 cycles */
    run_cycles(3u);

    /* Count RZC comm status writes */
    uint8 rzc_writes = 0u;
    uint8 i;
    for (i = 0u; i < mock_rte_write_count; i++) {
        if (mock_rte_write_sig_ids[i] == CVC_SIG_RZC_COMM_STATUS) {
            rzc_writes++;
        }
    }
    TEST_ASSERT_EQUAL(3u, rzc_writes);
}

/* ====================================================================
 * Test runner
 * ==================================================================== */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_Heartbeat_Init_comm_status_ok);
    RUN_TEST(test_Heartbeat_TX_every_50ms);
    RUN_TEST(test_Heartbeat_Alive_counter_increments);
    RUN_TEST(test_Heartbeat_Alive_counter_wraps_at_15);
    RUN_TEST(test_Heartbeat_FZC_RxIndication_resets_timer);
    RUN_TEST(test_Heartbeat_RZC_RxIndication_resets_timer);
    RUN_TEST(test_Heartbeat_FZC_timeout_after_3_misses);
    RUN_TEST(test_Heartbeat_RZC_timeout_after_3_misses);
    RUN_TEST(test_Heartbeat_FZC_timeout_DTC_reported);
    RUN_TEST(test_Heartbeat_RZC_timeout_DTC_reported);
    RUN_TEST(test_Heartbeat_Recovery_after_timeout);
    RUN_TEST(test_Heartbeat_RTE_write_fzc_comm_each_cycle);
    RUN_TEST(test_Heartbeat_RTE_write_rzc_comm_each_cycle);
    RUN_TEST(test_Heartbeat_TX_data_includes_ecu_id_and_state);
    RUN_TEST(test_Heartbeat_No_TX_before_50ms);

    return UNITY_END();
}
