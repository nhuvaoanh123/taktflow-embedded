/**
 * @file    test_Dcm.c
 * @brief   Unit tests for Diagnostic Communication Manager
 * @date    2026-02-21
 *
 * @verifies SWR-BSW-017
 *
 * Tests UDS service dispatch (0x10 DiagnosticSessionControl,
 * 0x22 ReadDataByIdentifier, 0x3E TesterPresent), session management,
 * S3 timer timeout, and NRC generation.
 */
#include "unity.h"
#include "Dcm.h"

/* ==================================================================
 * Mock: PduR_DcmTransmit (lower layer transmit)
 * ================================================================== */

static PduIdType      mock_tx_pdu_id;
static uint8          mock_tx_data[8];
static uint8          mock_tx_dlc;
static uint8          mock_tx_count;
static Std_ReturnType mock_tx_result;

Std_ReturnType PduR_DcmTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    mock_tx_pdu_id = TxPduId;
    if (PduInfoPtr != NULL_PTR) {
        mock_tx_dlc = PduInfoPtr->SduLength;
        for (uint8 i = 0u; (i < PduInfoPtr->SduLength) && (i < 8u); i++) {
            mock_tx_data[i] = PduInfoPtr->SduDataPtr[i];
        }
    }
    mock_tx_count++;
    return mock_tx_result;
}

/* ==================================================================
 * DID Read Callbacks (test DIDs)
 * ================================================================== */

static uint8 did_ecu_id_data[4] = {0x01u, 0x02u, 0x03u, 0x04u};
static uint8 did_sw_ver_data[2] = {0x01u, 0x00u};

static Std_ReturnType DID_ReadEcuId(uint8* Data, uint8 Length)
{
    uint8 i;
    if (Data == NULL_PTR) {
        return E_NOT_OK;
    }
    for (i = 0u; (i < Length) && (i < 4u); i++) {
        Data[i] = did_ecu_id_data[i];
    }
    return E_OK;
}

static Std_ReturnType DID_ReadSwVer(uint8* Data, uint8 Length)
{
    uint8 i;
    if (Data == NULL_PTR) {
        return E_NOT_OK;
    }
    for (i = 0u; (i < Length) && (i < 2u); i++) {
        Data[i] = did_sw_ver_data[i];
    }
    return E_OK;
}

/* ==================================================================
 * Test Configuration
 * ================================================================== */

static const Dcm_DidTableType test_did_table[] = {
    { 0xF190u, DID_ReadEcuId, 4u },   /* VIN / ECU ID */
    { 0xF195u, DID_ReadSwVer, 2u },   /* SW version   */
};

static Dcm_ConfigType test_config;

void setUp(void)
{
    mock_tx_count  = 0u;
    mock_tx_result = E_OK;
    mock_tx_dlc    = 0u;

    uint8 i;
    for (i = 0u; i < 8u; i++) {
        mock_tx_data[i] = 0u;
    }

    test_config.DidTable     = test_did_table;
    test_config.DidCount     = 2u;
    test_config.TxPduId      = 0u;
    test_config.S3TimeoutMs  = 5000u;

    Dcm_Init(&test_config);
}

void tearDown(void) { }

/* ==================================================================
 * SWR-BSW-017: DiagnosticSessionControl (SID 0x10)
 * ================================================================== */

/** @verifies SWR-BSW-017 — switch to extended session */
void test_Dcm_SessionControl_extended(void)
{
    uint8 req[] = {0x10u, 0x03u};  /* SID 0x10, sub=ExtendedSession */
    PduInfoType pdu = { req, 2u };

    Dcm_RxIndication(0u, &pdu);
    Dcm_MainFunction();

    /* Positive response: 0x50, 0x03 */
    TEST_ASSERT_EQUAL_UINT8(1u, mock_tx_count);
    TEST_ASSERT_EQUAL_HEX8(0x50u, mock_tx_data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x03u, mock_tx_data[1]);
}

/** @verifies SWR-BSW-017 — switch to default session */
void test_Dcm_SessionControl_default(void)
{
    uint8 req[] = {0x10u, 0x01u};  /* SID 0x10, sub=DefaultSession */
    PduInfoType pdu = { req, 2u };

    Dcm_RxIndication(0u, &pdu);
    Dcm_MainFunction();

    TEST_ASSERT_EQUAL_HEX8(0x50u, mock_tx_data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01u, mock_tx_data[1]);
}

/** @verifies SWR-BSW-017 — session control wrong length */
void test_Dcm_SessionControl_wrong_length(void)
{
    uint8 req[] = {0x10u};  /* Too short — needs 2 bytes */
    PduInfoType pdu = { req, 1u };

    Dcm_RxIndication(0u, &pdu);
    Dcm_MainFunction();

    /* NRC 0x13: incorrectMessageLengthOrInvalidFormat */
    TEST_ASSERT_EQUAL_HEX8(0x7Fu, mock_tx_data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x10u, mock_tx_data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x13u, mock_tx_data[2]);
}

/* ==================================================================
 * SWR-BSW-017: ReadDataByIdentifier (SID 0x22)
 * ================================================================== */

/** @verifies SWR-BSW-017 — read DID 0xF190 (ECU ID) */
void test_Dcm_ReadDID_EcuId(void)
{
    uint8 req[] = {0x22u, 0xF1u, 0x90u};  /* SID 0x22, DID 0xF190 */
    PduInfoType pdu = { req, 3u };

    Dcm_RxIndication(0u, &pdu);
    Dcm_MainFunction();

    /* Positive response: 0x62, DID_H, DID_L, data... */
    TEST_ASSERT_EQUAL_HEX8(0x62u, mock_tx_data[0]);
    TEST_ASSERT_EQUAL_HEX8(0xF1u, mock_tx_data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x90u, mock_tx_data[2]);
    TEST_ASSERT_EQUAL_HEX8(0x01u, mock_tx_data[3]);
}

/** @verifies SWR-BSW-017 — read DID 0xF195 (SW version) */
void test_Dcm_ReadDID_SwVersion(void)
{
    uint8 req[] = {0x22u, 0xF1u, 0x95u};
    PduInfoType pdu = { req, 3u };

    Dcm_RxIndication(0u, &pdu);
    Dcm_MainFunction();

    TEST_ASSERT_EQUAL_HEX8(0x62u, mock_tx_data[0]);
    TEST_ASSERT_EQUAL_HEX8(0xF1u, mock_tx_data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x95u, mock_tx_data[2]);
    TEST_ASSERT_EQUAL_HEX8(0x01u, mock_tx_data[3]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, mock_tx_data[4]);
}

/** @verifies SWR-BSW-017 — read unsupported DID -> NRC 0x31 */
void test_Dcm_ReadDID_unsupported(void)
{
    uint8 req[] = {0x22u, 0xFFu, 0xFFu};  /* Non-existent DID */
    PduInfoType pdu = { req, 3u };

    Dcm_RxIndication(0u, &pdu);
    Dcm_MainFunction();

    /* NRC 0x31: requestOutOfRange */
    TEST_ASSERT_EQUAL_HEX8(0x7Fu, mock_tx_data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x22u, mock_tx_data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x31u, mock_tx_data[2]);
}

/** @verifies SWR-BSW-017 — read DID wrong length */
void test_Dcm_ReadDID_wrong_length(void)
{
    uint8 req[] = {0x22u, 0xF1u};  /* Too short — needs 3 bytes */
    PduInfoType pdu = { req, 2u };

    Dcm_RxIndication(0u, &pdu);
    Dcm_MainFunction();

    TEST_ASSERT_EQUAL_HEX8(0x7Fu, mock_tx_data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x22u, mock_tx_data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x13u, mock_tx_data[2]);
}

/* ==================================================================
 * SWR-BSW-017: TesterPresent (SID 0x3E)
 * ================================================================== */

/** @verifies SWR-BSW-017 — tester present positive response */
void test_Dcm_TesterPresent(void)
{
    uint8 req[] = {0x3Eu, 0x00u};  /* SID 0x3E, sub=0x00 */
    PduInfoType pdu = { req, 2u };

    Dcm_RxIndication(0u, &pdu);
    Dcm_MainFunction();

    /* Positive response: 0x7E, 0x00 */
    TEST_ASSERT_EQUAL_HEX8(0x7Eu, mock_tx_data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, mock_tx_data[1]);
}

/** @verifies SWR-BSW-017 — tester present suppress response (sub=0x80) */
void test_Dcm_TesterPresent_suppress(void)
{
    uint8 req[] = {0x3Eu, 0x80u};  /* suppressPosRspMsgIndicationBit */
    PduInfoType pdu = { req, 2u };

    Dcm_RxIndication(0u, &pdu);
    Dcm_MainFunction();

    /* No response should be sent */
    TEST_ASSERT_EQUAL_UINT8(0u, mock_tx_count);
}

/* ==================================================================
 * SWR-BSW-017: NRC and Error Handling
 * ================================================================== */

/** @verifies SWR-BSW-017 — unknown SID -> NRC 0x11 */
void test_Dcm_unknown_SID(void)
{
    uint8 req[] = {0xFFu, 0x00u};  /* Unknown SID */
    PduInfoType pdu = { req, 2u };

    Dcm_RxIndication(0u, &pdu);
    Dcm_MainFunction();

    /* NRC 0x11: serviceNotSupported */
    TEST_ASSERT_EQUAL_HEX8(0x7Fu, mock_tx_data[0]);
    TEST_ASSERT_EQUAL_HEX8(0xFFu, mock_tx_data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x11u, mock_tx_data[2]);
}

/** @verifies SWR-BSW-017 — null PDU info */
void test_Dcm_RxIndication_null_pdu(void)
{
    Dcm_RxIndication(0u, NULL_PTR);
    Dcm_MainFunction();

    /* Should not crash, no response */
    TEST_ASSERT_EQUAL_UINT8(0u, mock_tx_count);
}

/** @verifies SWR-BSW-017 — init null config */
void test_Dcm_Init_null_config(void)
{
    Dcm_Init(NULL_PTR);

    uint8 req[] = {0x3Eu, 0x00u};
    PduInfoType pdu = { req, 2u };

    Dcm_RxIndication(0u, &pdu);
    Dcm_MainFunction();

    /* Not initialized — no response */
    TEST_ASSERT_EQUAL_UINT8(0u, mock_tx_count);
}

/** @verifies SWR-BSW-017 — S3 session timeout */
void test_Dcm_S3_timeout_resets_session(void)
{
    /* Enter extended session */
    uint8 req[] = {0x10u, 0x03u};
    PduInfoType pdu = { req, 2u };

    Dcm_RxIndication(0u, &pdu);
    Dcm_MainFunction();
    mock_tx_count = 0u;

    /* Simulate S3 timeout: call MainFunction enough times (5000ms / 10ms = 500 ticks) */
    uint16 i;
    for (i = 0u; i < 501u; i++) {
        Dcm_MainFunction();
    }

    /* Session should revert to DEFAULT */
    TEST_ASSERT_EQUAL(DCM_DEFAULT_SESSION, Dcm_GetCurrentSession());
}

/** @verifies SWR-BSW-017 — zero-length request */
void test_Dcm_empty_request(void)
{
    uint8 req[] = {0x00u};
    PduInfoType pdu = { req, 0u };

    Dcm_RxIndication(0u, &pdu);
    Dcm_MainFunction();

    /* Empty request — no response */
    TEST_ASSERT_EQUAL_UINT8(0u, mock_tx_count);
}

/* ==================================================================
 * Test runner
 * ================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* Session control */
    RUN_TEST(test_Dcm_SessionControl_extended);
    RUN_TEST(test_Dcm_SessionControl_default);
    RUN_TEST(test_Dcm_SessionControl_wrong_length);

    /* ReadDataByIdentifier */
    RUN_TEST(test_Dcm_ReadDID_EcuId);
    RUN_TEST(test_Dcm_ReadDID_SwVersion);
    RUN_TEST(test_Dcm_ReadDID_unsupported);
    RUN_TEST(test_Dcm_ReadDID_wrong_length);

    /* TesterPresent */
    RUN_TEST(test_Dcm_TesterPresent);
    RUN_TEST(test_Dcm_TesterPresent_suppress);

    /* NRC / error handling */
    RUN_TEST(test_Dcm_unknown_SID);
    RUN_TEST(test_Dcm_RxIndication_null_pdu);
    RUN_TEST(test_Dcm_Init_null_config);
    RUN_TEST(test_Dcm_S3_timeout_resets_session);
    RUN_TEST(test_Dcm_empty_request);

    return UNITY_END();
}
