/**
 * @file    test_Com.c
 * @brief   Unit tests for Communication module
 * @date    2026-02-21
 *
 * @verifies SWR-BSW-015, SWR-BSW-016
 *
 * Tests signal send/receive, RX indication, TX main function,
 * and signal packing/unpacking.
 */
#include "unity.h"
#include "Com.h"

/* ==================================================================
 * Mock: PduR (lower layer)
 * ================================================================== */

static PduIdType      mock_pdur_tx_pdu_id;
static uint8          mock_pdur_tx_data[8];
static uint8          mock_pdur_tx_dlc;
static uint8          mock_pdur_tx_count;
static Std_ReturnType mock_pdur_tx_result;

Std_ReturnType PduR_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    mock_pdur_tx_pdu_id = TxPduId;
    if (PduInfoPtr != NULL_PTR) {
        mock_pdur_tx_dlc = PduInfoPtr->SduLength;
        for (uint8 i = 0u; i < PduInfoPtr->SduLength; i++) {
            mock_pdur_tx_data[i] = PduInfoPtr->SduDataPtr[i];
        }
    }
    mock_pdur_tx_count++;
    return mock_pdur_tx_result;
}

/* ==================================================================
 * Test Configuration
 * ================================================================== */

/* Signal buffers */
static uint8  sig_torque_buf;
static sint16 sig_steering_buf;
static uint8  sig_motor_status_buf;

/* Signal config table */
static const Com_SignalConfigType test_signals[] = {
    /* id, bitPos, bitSize, type,        pduId, shadowBuf */
    {  0u,   16u,     8u,  COM_UINT8,    0u,   &sig_torque_buf },
    {  1u,   16u,    16u,  COM_SINT16,   1u,   &sig_steering_buf },
    {  2u,   16u,     8u,  COM_UINT8,    0u,   &sig_motor_status_buf },
};

/* TX PDU config */
static const Com_TxPduConfigType test_tx_pdus[] = {
    { 0u, 8u, 10u },  /* PDU 0, DLC 8, 10ms cycle */
    { 1u, 8u, 10u },  /* PDU 1, DLC 8, 10ms cycle */
};

/* RX PDU config */
static const Com_RxPduConfigType test_rx_pdus[] = {
    { 0u, 8u, 100u },  /* PDU 0, DLC 8, 100ms timeout */
};

static Com_ConfigType test_config;

void setUp(void)
{
    mock_pdur_tx_count = 0u;
    mock_pdur_tx_result = E_OK;
    sig_torque_buf = 0u;
    sig_steering_buf = 0;
    sig_motor_status_buf = 0u;

    test_config.signalConfig = test_signals;
    test_config.signalCount  = 3u;
    test_config.txPduConfig  = test_tx_pdus;
    test_config.txPduCount   = 2u;
    test_config.rxPduConfig  = test_rx_pdus;
    test_config.rxPduCount   = 1u;

    Com_Init(&test_config);
}

void tearDown(void) { }

/* ==================================================================
 * SWR-BSW-015: Signal Send/Receive
 * ================================================================== */

/** @verifies SWR-BSW-015 */
void test_Com_SendSignal_stores_value(void)
{
    uint8 torque = 128u;
    Std_ReturnType ret = Com_SendSignal(0u, &torque);

    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(128u, sig_torque_buf);
}

/** @verifies SWR-BSW-015 */
void test_Com_ReceiveSignal_reads_value(void)
{
    sig_motor_status_buf = 42u;
    uint8 value = 0u;

    Std_ReturnType ret = Com_ReceiveSignal(2u, &value);

    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(42u, value);
}

/** @verifies SWR-BSW-015 */
void test_Com_SendSignal_null_data(void)
{
    Std_ReturnType ret = Com_SendSignal(0u, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @verifies SWR-BSW-015 */
void test_Com_SendSignal_invalid_id(void)
{
    uint8 val = 0u;
    Std_ReturnType ret = Com_SendSignal(99u, &val);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @verifies SWR-BSW-015 */
void test_Com_ReceiveSignal_null_data(void)
{
    Std_ReturnType ret = Com_ReceiveSignal(0u, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* ==================================================================
 * SWR-BSW-016: RX Indication and TX MainFunction
 * ================================================================== */

/** @verifies SWR-BSW-016 */
void test_Com_RxIndication_stores_pdu(void)
{
    uint8 data[] = {0xAA, 0xBB, 0xCC, 0xDD, 0x00, 0x00, 0x00, 0x00};
    PduInfoType pdu = { data, 8u };

    Com_RxIndication(0u, &pdu);

    /* Signal at bit 16 (byte 2), 8 bits = data[2] = 0xCC */
    uint8 val = 0u;
    Com_ReceiveSignal(2u, &val);
    TEST_ASSERT_EQUAL_HEX8(0xCC, val);
}

/** @verifies SWR-BSW-016 */
void test_Com_MainFunction_Tx_sends_pending(void)
{
    uint8 torque = 200u;
    Com_SendSignal(0u, &torque);

    Com_MainFunction_Tx();

    TEST_ASSERT_TRUE(mock_pdur_tx_count > 0u);
}

/** @verifies SWR-BSW-016 */
void test_Com_RxIndication_null_pdu(void)
{
    Com_RxIndication(0u, NULL_PTR);
    /* Should not crash */
    TEST_ASSERT_TRUE(TRUE);
}

/** @verifies SWR-BSW-016 */
void test_Com_Init_null_config(void)
{
    Com_Init(NULL_PTR);
    uint8 val = 0u;
    Std_ReturnType ret = Com_SendSignal(0u, &val);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* ==================================================================
 * Test runner
 * ================================================================== */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_Com_SendSignal_stores_value);
    RUN_TEST(test_Com_ReceiveSignal_reads_value);
    RUN_TEST(test_Com_SendSignal_null_data);
    RUN_TEST(test_Com_SendSignal_invalid_id);
    RUN_TEST(test_Com_ReceiveSignal_null_data);
    RUN_TEST(test_Com_RxIndication_stores_pdu);
    RUN_TEST(test_Com_MainFunction_Tx_sends_pending);
    RUN_TEST(test_Com_RxIndication_null_pdu);
    RUN_TEST(test_Com_Init_null_config);

    return UNITY_END();
}
