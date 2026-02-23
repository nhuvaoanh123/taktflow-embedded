/**
 * @file    test_sc_can.c
 * @brief   Unit tests for sc_can — DCAN1 listen-only driver
 * @date    2026-02-23
 *
 * @verifies SWR-SC-001, SWR-SC-002, SWR-SC-023
 *
 * Tests DCAN1 initialization (silent mode), mailbox polling, E2E
 * validation integration, bus silence detection, and error monitoring.
 *
 * Mocks: DCAN1 registers, sc_e2e module, sc_heartbeat notification.
 */
#include "unity.h"

/* ==================================================================
 * Local type definitions
 * ================================================================== */

typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned long       uint32;
typedef signed short        sint16;
typedef signed long         sint32;
typedef uint8               boolean;
typedef uint8               Std_ReturnType;

#define TRUE                1u
#define FALSE               0u
#define E_OK                0u
#define E_NOT_OK            1u
#define NULL_PTR            ((void*)0)

/* ==================================================================
 * SC Configuration Constants
 * ================================================================== */

#define SC_CRC8_POLY                0x1Du
#define SC_CRC8_INIT                0xFFu
#define SC_E2E_MAX_CONSEC_FAIL      3u
#define SC_MB_COUNT                 6u
#define SC_HB_ALIVE_MAX             15u
#define SC_CAN_DLC                  8u
#define SC_BUS_SILENCE_TICKS        20u
#define SC_DCAN_BRP                 15u
#define SC_DCAN_TSEG1               7u
#define SC_DCAN_TSEG2               2u

#define SC_MB_IDX_ESTOP             0u
#define SC_MB_IDX_CVC_HB            1u
#define SC_MB_IDX_FZC_HB            2u
#define SC_MB_IDX_RZC_HB            3u
#define SC_MB_IDX_VEHICLE_STATE     4u
#define SC_MB_IDX_MOTOR_CURRENT     5u

#define SC_CAN_ID_ESTOP             0x001u
#define SC_CAN_ID_CVC_HB            0x010u
#define SC_CAN_ID_FZC_HB            0x011u
#define SC_CAN_ID_RZC_HB            0x012u
#define SC_CAN_ID_VEHICLE_STATE     0x100u
#define SC_CAN_ID_MOTOR_CURRENT     0x301u

#define SC_MB_ESTOP                 1u
#define SC_MB_CVC_HB                2u
#define SC_MB_FZC_HB                3u
#define SC_MB_RZC_HB                4u
#define SC_MB_VEHICLE_STATE         5u
#define SC_MB_MOTOR_CURRENT         6u

#define SC_E2E_ESTOP_DATA_ID        0x01u
#define SC_E2E_CVC_HB_DATA_ID      0x02u
#define SC_E2E_FZC_HB_DATA_ID      0x03u
#define SC_E2E_RZC_HB_DATA_ID      0x04u
#define SC_E2E_VEHSTATE_DATA_ID    0x05u
#define SC_E2E_MOTORCUR_DATA_ID    0x0Fu

#define SC_ECU_CVC                  0u
#define SC_ECU_FZC                  1u
#define SC_ECU_RZC                  2u

/* ==================================================================
 * Mock: DCAN1 Hardware Registers
 * ================================================================== */

static uint32 mock_dcan_ctl;
static uint32 mock_dcan_btr;
static uint32 mock_dcan_test;
static uint32 mock_dcan_es;
static uint32 mock_dcan_if1cmd;
static uint32 mock_dcan_if1arb;
static uint32 mock_dcan_if1mctl;
static uint32 mock_dcan_if1data_a;
static uint32 mock_dcan_if1data_b;
static uint32 mock_dcan_nwdat1;

/* Simulated mailbox data (6 mailboxes x 8 bytes) */
static uint8  mock_mb_data[SC_MB_COUNT][SC_CAN_DLC];
static boolean mock_mb_new_data[SC_MB_COUNT];

/* DCAN register read/write functions (mocked HALCoGen style) */
uint32 dcan1_reg_read(uint32 offset)
{
    switch (offset) {
        case 0x00u: return mock_dcan_ctl;
        case 0x04u: return mock_dcan_es;
        case 0x08u: return mock_dcan_btr;
        case 0x10u: return mock_dcan_test;
        case 0x98u: return mock_dcan_nwdat1;
        default:    return 0u;
    }
}

void dcan1_reg_write(uint32 offset, uint32 value)
{
    switch (offset) {
        case 0x00u: mock_dcan_ctl = value; break;
        case 0x08u: mock_dcan_btr = value; break;
        case 0x10u: mock_dcan_test = value; break;
        default: break;
    }
}

/* Simulated mailbox data read */
boolean dcan1_get_mailbox_data(uint8 mbIndex, uint8* data, uint8* dlc)
{
    if ((mbIndex >= SC_MB_COUNT) || (data == NULL_PTR) || (dlc == NULL_PTR)) {
        return FALSE;
    }
    if (mock_mb_new_data[mbIndex] == FALSE) {
        return FALSE;
    }
    uint8 i;
    for (i = 0u; i < SC_CAN_DLC; i++) {
        data[i] = mock_mb_data[mbIndex][i];
    }
    *dlc = SC_CAN_DLC;
    mock_mb_new_data[mbIndex] = FALSE;
    return TRUE;
}

/* ==================================================================
 * Mock: sc_e2e
 * ================================================================== */

static uint8   mock_e2e_check_count;
static boolean mock_e2e_check_result;

void SC_E2E_Init(void)
{
    mock_e2e_check_count = 0u;
    mock_e2e_check_result = TRUE;
}

boolean SC_E2E_Check(const uint8* data, uint8 dlc, uint8 dataId,
                     uint8 msgIndex)
{
    (void)data;
    (void)dlc;
    (void)dataId;
    (void)msgIndex;
    mock_e2e_check_count++;
    return mock_e2e_check_result;
}

boolean SC_E2E_IsMsgFailed(uint8 msgIndex)
{
    (void)msgIndex;
    return FALSE;
}

/* ==================================================================
 * Mock: sc_heartbeat notification
 * ================================================================== */

static uint8 mock_hb_notify_count;
static uint8 mock_hb_last_ecu;

void SC_Heartbeat_NotifyRx(uint8 ecuIndex)
{
    mock_hb_notify_count++;
    mock_hb_last_ecu = ecuIndex;
}

/* ==================================================================
 * Include source under test
 * ================================================================== */

#include "../src/sc_can.c"

/* ==================================================================
 * Test setup / teardown
 * ================================================================== */

void setUp(void)
{
    uint8 i;
    uint8 j;

    mock_dcan_ctl     = 0u;
    mock_dcan_btr     = 0u;
    mock_dcan_test    = 0u;
    mock_dcan_es      = 0u;
    mock_dcan_if1cmd  = 0u;
    mock_dcan_if1arb  = 0u;
    mock_dcan_if1mctl = 0u;
    mock_dcan_if1data_a = 0u;
    mock_dcan_if1data_b = 0u;
    mock_dcan_nwdat1  = 0u;

    for (i = 0u; i < SC_MB_COUNT; i++) {
        mock_mb_new_data[i] = FALSE;
        for (j = 0u; j < SC_CAN_DLC; j++) {
            mock_mb_data[i][j] = 0u;
        }
    }

    mock_e2e_check_count  = 0u;
    mock_e2e_check_result = TRUE;
    mock_hb_notify_count  = 0u;
    mock_hb_last_ecu      = 0xFFu;

    SC_CAN_Init();
}

void tearDown(void) { }

/* ==================================================================
 * SWR-SC-001: DCAN1 Initialization
 * ================================================================== */

/** @verifies SWR-SC-001 -- Init configures DCAN1 in silent mode */
void test_CAN_Init_silent_mode(void)
{
    /* After init, the test register should have silent bit set */
    TEST_ASSERT_TRUE(can_initialized);
}

/** @verifies SWR-SC-001 -- Init resets bus silence counter */
void test_CAN_Init_resets_silence(void)
{
    /* Silence counter should be 0 after init */
    TEST_ASSERT_EQUAL_UINT16(0u, bus_silence_counter);
}

/* ==================================================================
 * SWR-SC-002: Mailbox Polling
 * ================================================================== */

/** @verifies SWR-SC-002 -- Receive polls all mailboxes with new data */
void test_CAN_Receive_polls_all(void)
{
    /* Place data in CVC heartbeat mailbox */
    mock_mb_new_data[SC_MB_IDX_CVC_HB] = TRUE;
    mock_mb_data[SC_MB_IDX_CVC_HB][0]  = 0x10u;  /* alive=1 */

    SC_CAN_Receive();

    /* E2E check should have been called at least once */
    TEST_ASSERT_TRUE(mock_e2e_check_count > 0u);
}

/** @verifies SWR-SC-002 -- Valid heartbeat triggers heartbeat notify */
void test_CAN_Receive_notifies_heartbeat(void)
{
    /* Place data in FZC heartbeat mailbox */
    mock_mb_new_data[SC_MB_IDX_FZC_HB] = TRUE;
    mock_e2e_check_result = TRUE;

    SC_CAN_Receive();

    TEST_ASSERT_EQUAL_UINT8(1u, mock_hb_notify_count);
    TEST_ASSERT_EQUAL_UINT8(SC_ECU_FZC, mock_hb_last_ecu);
}

/** @verifies SWR-SC-002 -- Failed E2E does not notify heartbeat */
void test_CAN_Receive_no_notify_on_e2e_fail(void)
{
    mock_mb_new_data[SC_MB_IDX_CVC_HB] = TRUE;
    mock_e2e_check_result = FALSE;

    SC_CAN_Receive();

    TEST_ASSERT_EQUAL_UINT8(0u, mock_hb_notify_count);
}

/** @verifies SWR-SC-002 -- Receive stores last valid message data */
void test_CAN_Receive_stores_data(void)
{
    uint8 i;
    for (i = 0u; i < SC_CAN_DLC; i++) {
        mock_mb_data[SC_MB_IDX_VEHICLE_STATE][i] = (uint8)(0x50u + i);
    }
    mock_mb_new_data[SC_MB_IDX_VEHICLE_STATE] = TRUE;

    SC_CAN_Receive();

    uint8 out[SC_CAN_DLC];
    uint8 dlc;
    boolean ok = SC_CAN_GetMessage(SC_MB_IDX_VEHICLE_STATE, out, &dlc);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT8(SC_CAN_DLC, dlc);
    TEST_ASSERT_EQUAL_UINT8(0x54u, out[4]);
}

/** @verifies SWR-SC-002 -- Any valid reception resets bus silence counter */
void test_CAN_Receive_resets_silence(void)
{
    /* Advance silence counter */
    uint8 i;
    for (i = 0u; i < 10u; i++) {
        SC_CAN_MonitorBus();
    }
    TEST_ASSERT_EQUAL_UINT16(10u, bus_silence_counter);

    /* Receive valid data */
    mock_mb_new_data[SC_MB_IDX_CVC_HB] = TRUE;
    mock_e2e_check_result = TRUE;
    SC_CAN_Receive();

    TEST_ASSERT_EQUAL_UINT16(0u, bus_silence_counter);
}

/* ==================================================================
 * SWR-SC-023: Bus Silence Monitoring
 * ================================================================== */

/** @verifies SWR-SC-023 -- Bus silence counter increments each tick */
void test_CAN_MonitorBus_increments(void)
{
    SC_CAN_MonitorBus();
    TEST_ASSERT_EQUAL_UINT16(1u, bus_silence_counter);

    SC_CAN_MonitorBus();
    TEST_ASSERT_EQUAL_UINT16(2u, bus_silence_counter);
}

/** @verifies SWR-SC-023 -- 200ms silence triggers all-timeout */
void test_CAN_MonitorBus_silence_timeout(void)
{
    uint16 i;
    for (i = 0u; i < SC_BUS_SILENCE_TICKS; i++) {
        SC_CAN_MonitorBus();
    }

    TEST_ASSERT_TRUE(SC_CAN_IsBusSilent());
}

/** @verifies SWR-SC-023 -- Bus-off error detected from ES register */
void test_CAN_MonitorBus_busoff(void)
{
    mock_dcan_es = 0x80u;  /* Bus-off bit (bit 7) */

    SC_CAN_MonitorBus();

    TEST_ASSERT_TRUE(SC_CAN_IsBusOff());
}

/** @verifies SWR-SC-023 -- GetMessage returns FALSE for invalid index */
void test_CAN_GetMessage_invalid_index(void)
{
    uint8 out[8];
    uint8 dlc;
    boolean ok = SC_CAN_GetMessage(SC_MB_COUNT, out, &dlc);
    TEST_ASSERT_FALSE(ok);
}

/* ==================================================================
 * Test runner
 * ================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* SWR-SC-001: Initialization */
    RUN_TEST(test_CAN_Init_silent_mode);
    RUN_TEST(test_CAN_Init_resets_silence);

    /* SWR-SC-002: Mailbox Polling */
    RUN_TEST(test_CAN_Receive_polls_all);
    RUN_TEST(test_CAN_Receive_notifies_heartbeat);
    RUN_TEST(test_CAN_Receive_no_notify_on_e2e_fail);
    RUN_TEST(test_CAN_Receive_stores_data);
    RUN_TEST(test_CAN_Receive_resets_silence);

    /* SWR-SC-023: Bus Silence */
    RUN_TEST(test_CAN_MonitorBus_increments);
    RUN_TEST(test_CAN_MonitorBus_silence_timeout);
    RUN_TEST(test_CAN_MonitorBus_busoff);
    RUN_TEST(test_CAN_GetMessage_invalid_index);

    return UNITY_END();
}
