/**
 * @file    test_Can.c
 * @brief   Unit tests for CAN MCAL driver
 * @date    2026-02-21
 *
 * @verifies SWR-BSW-001, SWR-BSW-002, SWR-BSW-003, SWR-BSW-004, SWR-BSW-005
 *
 * Tests CAN driver state machine, transmit, receive processing,
 * bus-off recovery, and error handling. Hardware is mocked via
 * Can_Hw_* stub functions defined in this file.
 */
#include "unity.h"
#include "Can.h"

/* ==================================================================
 * Mock Hardware Layer — replaces real HAL for host testing
 * ================================================================== */

/* Mock state */
static boolean      mock_hw_init_called;
static boolean      mock_hw_init_fail;
static uint32       mock_hw_baudrate;
static boolean      mock_hw_started;
static boolean      mock_hw_bus_off;
static uint8        mock_hw_tec;
static uint8        mock_hw_rec;

/* Mock TX capture */
#define MOCK_TX_MAX 8u
static Can_IdType   mock_tx_ids[MOCK_TX_MAX];
static uint8        mock_tx_data[MOCK_TX_MAX][8];
static uint8        mock_tx_dlc[MOCK_TX_MAX];
static uint8        mock_tx_count;
static boolean      mock_tx_full;

/* Mock RX injection */
#define MOCK_RX_MAX 16u
static Can_IdType   mock_rx_ids[MOCK_RX_MAX];
static uint8        mock_rx_data[MOCK_RX_MAX][8];
static uint8        mock_rx_dlc[MOCK_RX_MAX];
static uint8        mock_rx_count;
static uint8        mock_rx_read_idx;

/* Mock CanIf callback capture */
static Can_IdType   canif_rx_id;
static uint8        canif_rx_data[8];
static uint8        canif_rx_dlc;
static uint8        canif_rx_call_count;
static boolean      canif_busoff_called;

/* ---- Hardware mock implementations ---- */

Std_ReturnType Can_Hw_Init(uint32 baudrate)
{
    mock_hw_init_called = TRUE;
    mock_hw_baudrate = baudrate;
    if (mock_hw_init_fail) {
        return E_NOT_OK;
    }
    return E_OK;
}

void Can_Hw_Start(void)
{
    mock_hw_started = TRUE;
}

void Can_Hw_Stop(void)
{
    mock_hw_started = FALSE;
}

Std_ReturnType Can_Hw_Transmit(Can_IdType id, const uint8* data, uint8 dlc)
{
    if (mock_tx_full || (mock_tx_count >= MOCK_TX_MAX)) {
        return E_NOT_OK;
    }
    mock_tx_ids[mock_tx_count] = id;
    for (uint8 i = 0u; i < dlc; i++) {
        mock_tx_data[mock_tx_count][i] = data[i];
    }
    mock_tx_dlc[mock_tx_count] = dlc;
    mock_tx_count++;
    return E_OK;
}

boolean Can_Hw_Receive(Can_IdType* id, uint8* data, uint8* dlc)
{
    if (mock_rx_read_idx >= mock_rx_count) {
        return FALSE;
    }
    *id = mock_rx_ids[mock_rx_read_idx];
    *dlc = mock_rx_dlc[mock_rx_read_idx];
    for (uint8 i = 0u; i < *dlc; i++) {
        data[i] = mock_rx_data[mock_rx_read_idx][i];
    }
    mock_rx_read_idx++;
    return TRUE;
}

boolean Can_Hw_IsBusOff(void)
{
    return mock_hw_bus_off;
}

void Can_Hw_GetErrorCounters(uint8* tec, uint8* rec)
{
    *tec = mock_hw_tec;
    *rec = mock_hw_rec;
}

/* ---- CanIf callback mocks ---- */

void CanIf_RxIndication(Can_IdType canId, const uint8* sduPtr, uint8 dlc)
{
    canif_rx_id = canId;
    canif_rx_dlc = dlc;
    for (uint8 i = 0u; i < dlc; i++) {
        canif_rx_data[i] = sduPtr[i];
    }
    canif_rx_call_count++;
}

void CanIf_ControllerBusOff(uint8 controllerId)
{
    (void)controllerId;
    canif_busoff_called = TRUE;
}

/* ---- Mock helpers ---- */

static void mock_inject_rx(Can_IdType id, const uint8* data, uint8 dlc)
{
    if (mock_rx_count < MOCK_RX_MAX) {
        mock_rx_ids[mock_rx_count] = id;
        mock_rx_dlc[mock_rx_count] = dlc;
        for (uint8 i = 0u; i < dlc; i++) {
            mock_rx_data[mock_rx_count][i] = data[i];
        }
        mock_rx_count++;
    }
}

/* ==================================================================
 * Test fixtures
 * ================================================================== */

static Can_ConfigType test_config;

void setUp(void)
{
    /* Reset all mock state */
    mock_hw_init_called = FALSE;
    mock_hw_init_fail = FALSE;
    mock_hw_baudrate = 0u;
    mock_hw_started = FALSE;
    mock_hw_bus_off = FALSE;
    mock_hw_tec = 0u;
    mock_hw_rec = 0u;
    mock_tx_count = 0u;
    mock_tx_full = FALSE;
    mock_rx_count = 0u;
    mock_rx_read_idx = 0u;
    canif_rx_call_count = 0u;
    canif_busoff_called = FALSE;

    for (uint8 i = 0u; i < MOCK_TX_MAX; i++) {
        mock_tx_ids[i] = 0u;
        mock_tx_dlc[i] = 0u;
    }

    /* Default test config */
    test_config.baudrate = 500000u;
    test_config.controllerId = 0u;
}

void tearDown(void) { }

/* ==================================================================
 * SWR-BSW-001: CAN Driver Initialization
 * ================================================================== */

/** @verifies SWR-BSW-001 */
void test_Can_Init_success(void)
{
    Can_Init(&test_config);

    TEST_ASSERT_TRUE(mock_hw_init_called);
    TEST_ASSERT_EQUAL_UINT32(500000u, mock_hw_baudrate);
    TEST_ASSERT_EQUAL(CAN_CS_STOPPED, Can_GetControllerMode(0u));
}

/** @verifies SWR-BSW-001 */
void test_Can_Init_hw_failure(void)
{
    mock_hw_init_fail = TRUE;
    Can_Init(&test_config);

    TEST_ASSERT_EQUAL(CAN_CS_UNINIT, Can_GetControllerMode(0u));
}

/** @verifies SWR-BSW-001 */
void test_Can_Init_null_config(void)
{
    Can_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(CAN_CS_UNINIT, Can_GetControllerMode(0u));
}

/** @verifies SWR-BSW-001 */
void test_Can_SetMode_start(void)
{
    Can_Init(&test_config);
    Std_ReturnType ret = Can_SetControllerMode(0u, CAN_CS_STARTED);

    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(CAN_CS_STARTED, Can_GetControllerMode(0u));
    TEST_ASSERT_TRUE(mock_hw_started);
}

/** @verifies SWR-BSW-001 */
void test_Can_SetMode_stop(void)
{
    Can_Init(&test_config);
    Can_SetControllerMode(0u, CAN_CS_STARTED);
    Std_ReturnType ret = Can_SetControllerMode(0u, CAN_CS_STOPPED);

    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(CAN_CS_STOPPED, Can_GetControllerMode(0u));
    TEST_ASSERT_FALSE(mock_hw_started);
}

/** @verifies SWR-BSW-001 */
void test_Can_SetMode_before_init_fails(void)
{
    Std_ReturnType ret = Can_SetControllerMode(0u, CAN_CS_STARTED);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* ==================================================================
 * SWR-BSW-002: CAN Driver Write
 * ================================================================== */

/** @verifies SWR-BSW-002 */
void test_Can_Write_success(void)
{
    Can_Init(&test_config);
    Can_SetControllerMode(0u, CAN_CS_STARTED);

    Can_PduType pdu;
    uint8 data[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    pdu.id = 0x100u;
    pdu.length = 8u;
    pdu.sdu = data;

    Can_ReturnType ret = Can_Write(0u, &pdu);

    TEST_ASSERT_EQUAL(CAN_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1u, mock_tx_count);
    TEST_ASSERT_EQUAL_HEX32(0x100u, mock_tx_ids[0]);
    TEST_ASSERT_EQUAL_HEX8(0x11, mock_tx_data[0][0]);
}

/** @verifies SWR-BSW-002 */
void test_Can_Write_not_started_fails(void)
{
    Can_Init(&test_config);
    /* Controller is STOPPED, not STARTED */

    Can_PduType pdu;
    uint8 data[] = {0x01};
    pdu.id = 0x100u;
    pdu.length = 1u;
    pdu.sdu = data;

    Can_ReturnType ret = Can_Write(0u, &pdu);
    TEST_ASSERT_EQUAL(CAN_NOT_OK, ret);
}

/** @verifies SWR-BSW-002 */
void test_Can_Write_null_pdu_fails(void)
{
    Can_Init(&test_config);
    Can_SetControllerMode(0u, CAN_CS_STARTED);

    Can_ReturnType ret = Can_Write(0u, NULL_PTR);
    TEST_ASSERT_EQUAL(CAN_NOT_OK, ret);
}

/** @verifies SWR-BSW-002 */
void test_Can_Write_invalid_dlc_fails(void)
{
    Can_Init(&test_config);
    Can_SetControllerMode(0u, CAN_CS_STARTED);

    Can_PduType pdu;
    uint8 data[] = {0x01};
    pdu.id = 0x100u;
    pdu.length = 9u; /* > 8 is invalid for CAN 2.0B */
    pdu.sdu = data;

    Can_ReturnType ret = Can_Write(0u, &pdu);
    TEST_ASSERT_EQUAL(CAN_NOT_OK, ret);
}

/** @verifies SWR-BSW-002 */
void test_Can_Write_tx_full_returns_busy(void)
{
    Can_Init(&test_config);
    Can_SetControllerMode(0u, CAN_CS_STARTED);

    mock_tx_full = TRUE;

    Can_PduType pdu;
    uint8 data[] = {0x01};
    pdu.id = 0x100u;
    pdu.length = 1u;
    pdu.sdu = data;

    Can_ReturnType ret = Can_Write(0u, &pdu);
    TEST_ASSERT_EQUAL(CAN_BUSY, ret);
}

/* ==================================================================
 * SWR-BSW-003: CAN MainFunction_Read
 * ================================================================== */

/** @verifies SWR-BSW-003 */
void test_Can_MainFunction_Read_processes_message(void)
{
    Can_Init(&test_config);
    Can_SetControllerMode(0u, CAN_CS_STARTED);

    uint8 rx_data[] = {0xAA, 0xBB, 0xCC, 0xDD, 0x00, 0x00, 0x00, 0x00};
    mock_inject_rx(0x200u, rx_data, 4u);

    Can_MainFunction_Read();

    TEST_ASSERT_EQUAL_UINT8(1u, canif_rx_call_count);
    TEST_ASSERT_EQUAL_HEX32(0x200u, canif_rx_id);
    TEST_ASSERT_EQUAL_UINT8(4u, canif_rx_dlc);
    TEST_ASSERT_EQUAL_HEX8(0xAA, canif_rx_data[0]);
}

/** @verifies SWR-BSW-003 */
void test_Can_MainFunction_Read_multiple_messages(void)
{
    Can_Init(&test_config);
    Can_SetControllerMode(0u, CAN_CS_STARTED);

    uint8 data1[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8 data2[] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8 data3[] = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    mock_inject_rx(0x100u, data1, 8u);
    mock_inject_rx(0x101u, data2, 8u);
    mock_inject_rx(0x102u, data3, 8u);

    Can_MainFunction_Read();

    TEST_ASSERT_EQUAL_UINT8(3u, canif_rx_call_count);
}

/** @verifies SWR-BSW-003 */
void test_Can_MainFunction_Read_empty_fifo(void)
{
    Can_Init(&test_config);
    Can_SetControllerMode(0u, CAN_CS_STARTED);

    Can_MainFunction_Read();

    TEST_ASSERT_EQUAL_UINT8(0u, canif_rx_call_count);
}

/** @verifies SWR-BSW-003 */
void test_Can_MainFunction_Read_not_started_skips(void)
{
    Can_Init(&test_config);
    /* Controller is STOPPED */

    uint8 data[] = {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    mock_inject_rx(0x100u, data, 1u);

    Can_MainFunction_Read();

    TEST_ASSERT_EQUAL_UINT8(0u, canif_rx_call_count);
}

/* ==================================================================
 * SWR-BSW-004: CAN Bus-Off Recovery
 * ================================================================== */

/** @verifies SWR-BSW-004 */
void test_Can_MainFunction_BusOff_detects_busoff(void)
{
    Can_Init(&test_config);
    Can_SetControllerMode(0u, CAN_CS_STARTED);

    mock_hw_bus_off = TRUE;
    Can_MainFunction_BusOff();

    TEST_ASSERT_TRUE(canif_busoff_called);
}

/** @verifies SWR-BSW-004 */
void test_Can_MainFunction_BusOff_no_busoff(void)
{
    Can_Init(&test_config);
    Can_SetControllerMode(0u, CAN_CS_STARTED);

    mock_hw_bus_off = FALSE;
    Can_MainFunction_BusOff();

    TEST_ASSERT_FALSE(canif_busoff_called);
}

/* ==================================================================
 * SWR-BSW-005: Error Counters
 * ================================================================== */

/** @verifies SWR-BSW-005 */
void test_Can_GetErrorCounters(void)
{
    Can_Init(&test_config);
    mock_hw_tec = 127u;
    mock_hw_rec = 64u;

    uint8 tec = 0u;
    uint8 rec = 0u;
    Std_ReturnType ret = Can_GetErrorCounters(0u, &tec, &rec);

    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(127u, tec);
    TEST_ASSERT_EQUAL_UINT8(64u, rec);
}

/** @verifies SWR-BSW-005 */
void test_Can_GetErrorCounters_null_fails(void)
{
    Can_Init(&test_config);
    Std_ReturnType ret = Can_GetErrorCounters(0u, NULL_PTR, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* ==================================================================
 * Test runner
 * ================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* Init tests (SWR-BSW-001) */
    RUN_TEST(test_Can_Init_success);
    RUN_TEST(test_Can_Init_hw_failure);
    RUN_TEST(test_Can_Init_null_config);
    RUN_TEST(test_Can_SetMode_start);
    RUN_TEST(test_Can_SetMode_stop);
    RUN_TEST(test_Can_SetMode_before_init_fails);

    /* Write tests (SWR-BSW-002) */
    RUN_TEST(test_Can_Write_success);
    RUN_TEST(test_Can_Write_not_started_fails);
    RUN_TEST(test_Can_Write_null_pdu_fails);
    RUN_TEST(test_Can_Write_invalid_dlc_fails);
    RUN_TEST(test_Can_Write_tx_full_returns_busy);

    /* Read tests (SWR-BSW-003) */
    RUN_TEST(test_Can_MainFunction_Read_processes_message);
    RUN_TEST(test_Can_MainFunction_Read_multiple_messages);
    RUN_TEST(test_Can_MainFunction_Read_empty_fifo);
    RUN_TEST(test_Can_MainFunction_Read_not_started_skips);

    /* Bus-off tests (SWR-BSW-004) */
    RUN_TEST(test_Can_MainFunction_BusOff_detects_busoff);
    RUN_TEST(test_Can_MainFunction_BusOff_no_busoff);

    /* Error counter tests (SWR-BSW-005) */
    RUN_TEST(test_Can_GetErrorCounters);
    RUN_TEST(test_Can_GetErrorCounters_null_fails);

    return UNITY_END();
}
