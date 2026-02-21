/**
 * @file    Dcm.c
 * @brief   Diagnostic Communication Manager implementation
 * @date    2026-02-21
 *
 * @safety_req SWR-BSW-017
 * @traces_to  TSR-038, TSR-039, TSR-040
 *
 * Implements minimal UDS diagnostic services for physical ECUs:
 * - 0x10 DiagnosticSessionControl (Default + Extended sessions)
 * - 0x22 ReadDataByIdentifier (configurable DID table)
 * - 0x3E TesterPresent (with suppress-positive-response support)
 *
 * @standard AUTOSAR_SWS_DiagnosticCommunicationManager, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#include "Dcm.h"

/* ---- Internal State ---- */

static const Dcm_ConfigType*  dcm_config = NULL_PTR;
static boolean                dcm_initialized = FALSE;

/* Request buffer */
static uint8   dcm_rx_buf[DCM_TX_BUF_SIZE];
static uint8   dcm_rx_len;
static boolean dcm_request_pending;

/* Response buffer */
static uint8   dcm_tx_buf[DCM_TX_BUF_SIZE];

/* Session state */
static Dcm_SessionType dcm_current_session;
static uint16          dcm_s3_timer_ms;

/* ---- Private Helpers ---- */

/**
 * @brief  Send a UDS response via PduR
 * @param  data    Response data buffer
 * @param  length  Response length in bytes
 */
static void dcm_send_response(const uint8* data, uint8 length)
{
    PduInfoType pdu_info;

    if ((data == NULL_PTR) || (length == 0u)) {
        return;
    }

    pdu_info.SduDataPtr = (uint8*)data;
    pdu_info.SduLength  = length;

    (void)PduR_DcmTransmit(dcm_config->TxPduId, &pdu_info);
}

/**
 * @brief  Send UDS negative response
 * @param  sid  Rejected service ID
 * @param  nrc  Negative response code
 */
static void dcm_send_nrc(uint8 sid, uint8 nrc)
{
    dcm_tx_buf[0] = DCM_NEGATIVE_RESPONSE_SID;
    dcm_tx_buf[1] = sid;
    dcm_tx_buf[2] = nrc;

    dcm_send_response(dcm_tx_buf, 3u);
}

/**
 * @brief  Reset S3 session timer
 */
static void dcm_reset_s3_timer(void)
{
    dcm_s3_timer_ms = 0u;
}

/**
 * @brief  Handle 0x10 DiagnosticSessionControl
 * @param  data    Request data (SID + sub-function)
 * @param  length  Request length
 */
static void dcm_handle_session_control(const uint8* data, uint8 length)
{
    uint8 sub_function;

    if (length < 2u) {
        dcm_send_nrc(DCM_SID_SESSION_CTRL, DCM_NRC_INCORRECT_MSG_LENGTH);
        return;
    }

    sub_function = data[1];

    switch (sub_function) {
    case (uint8)DCM_DEFAULT_SESSION:
        dcm_current_session = DCM_DEFAULT_SESSION;
        break;

    case (uint8)DCM_EXTENDED_SESSION:
        dcm_current_session = DCM_EXTENDED_SESSION;
        dcm_reset_s3_timer();
        break;

    default:
        /* Sub-function not supported — use requestOutOfRange */
        dcm_send_nrc(DCM_SID_SESSION_CTRL, DCM_NRC_REQUEST_OUT_OF_RANGE);
        return;
    }

    /* Positive response: SID + 0x40, sub-function */
    dcm_tx_buf[0] = DCM_SID_SESSION_CTRL + DCM_POSITIVE_RESPONSE_OFFSET;
    dcm_tx_buf[1] = sub_function;

    dcm_send_response(dcm_tx_buf, 2u);
}

/**
 * @brief  Handle 0x22 ReadDataByIdentifier
 * @param  data    Request data (SID + DID_H + DID_L)
 * @param  length  Request length
 */
static void dcm_handle_read_did(const uint8* data, uint8 length)
{
    uint16 requested_did;
    uint8 i;

    if (length < 3u) {
        dcm_send_nrc(DCM_SID_READ_DID, DCM_NRC_INCORRECT_MSG_LENGTH);
        return;
    }

    requested_did = ((uint16)data[1] << 8u) | (uint16)data[2];

    /* Search DID table */
    for (i = 0u; i < dcm_config->DidCount; i++) {
        if (dcm_config->DidTable[i].Did == requested_did) {
            const Dcm_DidTableType* did_entry = &dcm_config->DidTable[i];

            /* Check response fits in buffer: 1 (SID+0x40) + 2 (DID) + DataLength */
            if ((3u + did_entry->DataLength) > DCM_TX_BUF_SIZE) {
                dcm_send_nrc(DCM_SID_READ_DID, DCM_NRC_REQUEST_OUT_OF_RANGE);
                return;
            }

            /* Build positive response header */
            dcm_tx_buf[0] = DCM_SID_READ_DID + DCM_POSITIVE_RESPONSE_OFFSET;
            dcm_tx_buf[1] = (uint8)(requested_did >> 8u);
            dcm_tx_buf[2] = (uint8)(requested_did & 0xFFu);

            /* Call DID read callback */
            if ((did_entry->ReadFunc != NULL_PTR) &&
                (did_entry->ReadFunc(&dcm_tx_buf[3], did_entry->DataLength) == E_OK)) {
                dcm_send_response(dcm_tx_buf, 3u + did_entry->DataLength);
            } else {
                dcm_send_nrc(DCM_SID_READ_DID, DCM_NRC_REQUEST_OUT_OF_RANGE);
            }
            return;
        }
    }

    /* DID not found */
    dcm_send_nrc(DCM_SID_READ_DID, DCM_NRC_REQUEST_OUT_OF_RANGE);
}

/**
 * @brief  Handle 0x3E TesterPresent
 * @param  data    Request data (SID + sub-function)
 * @param  length  Request length
 */
static void dcm_handle_tester_present(const uint8* data, uint8 length)
{
    uint8 sub_function;
    boolean suppress_response;

    if (length < 2u) {
        dcm_send_nrc(DCM_SID_TESTER_PRESENT, DCM_NRC_INCORRECT_MSG_LENGTH);
        return;
    }

    sub_function = data[1];
    suppress_response = ((sub_function & DCM_SUPPRESS_POS_RSP_BIT) != 0u) ? TRUE : FALSE;

    /* Reset S3 timer on tester present */
    dcm_reset_s3_timer();

    if (suppress_response == FALSE) {
        dcm_tx_buf[0] = DCM_SID_TESTER_PRESENT + DCM_POSITIVE_RESPONSE_OFFSET;
        dcm_tx_buf[1] = sub_function & (uint8)(~DCM_SUPPRESS_POS_RSP_BIT);
        dcm_send_response(dcm_tx_buf, 2u);
    }
}

/**
 * @brief  Process a received UDS request
 * @param  data    Request data buffer
 * @param  length  Request length in bytes
 */
static void dcm_process_request(const uint8* data, uint8 length)
{
    uint8 sid;

    if ((data == NULL_PTR) || (length == 0u)) {
        return;
    }

    sid = data[0];

    /* Reset S3 timer on any valid request */
    dcm_reset_s3_timer();

    switch (sid) {
    case DCM_SID_SESSION_CTRL:
        dcm_handle_session_control(data, length);
        break;

    case DCM_SID_READ_DID:
        dcm_handle_read_did(data, length);
        break;

    case DCM_SID_TESTER_PRESENT:
        dcm_handle_tester_present(data, length);
        break;

    default:
        dcm_send_nrc(sid, DCM_NRC_SERVICE_NOT_SUPPORTED);
        break;
    }
}

/* ---- API Implementation ---- */

void Dcm_Init(const Dcm_ConfigType* ConfigPtr)
{
    uint8 i;

    if (ConfigPtr == NULL_PTR) {
        dcm_initialized = FALSE;
        dcm_config = NULL_PTR;
        return;
    }

    dcm_config = ConfigPtr;

    /* Clear buffers */
    for (i = 0u; i < DCM_TX_BUF_SIZE; i++) {
        dcm_rx_buf[i] = 0u;
        dcm_tx_buf[i] = 0u;
    }

    dcm_rx_len           = 0u;
    dcm_request_pending  = FALSE;
    dcm_current_session  = DCM_DEFAULT_SESSION;
    dcm_s3_timer_ms      = 0u;
    dcm_initialized      = TRUE;
}

void Dcm_MainFunction(void)
{
    if ((dcm_initialized == FALSE) || (dcm_config == NULL_PTR)) {
        return;
    }

    /* Process pending request */
    if (dcm_request_pending == TRUE) {
        dcm_process_request(dcm_rx_buf, dcm_rx_len);
        dcm_request_pending = FALSE;
    }

    /* S3 timer management — only active in non-default sessions */
    if (dcm_current_session != DCM_DEFAULT_SESSION) {
        dcm_s3_timer_ms += DCM_MAIN_CYCLE_MS;

        if (dcm_s3_timer_ms >= dcm_config->S3TimeoutMs) {
            /* S3 timeout — revert to default session */
            dcm_current_session = DCM_DEFAULT_SESSION;
            dcm_s3_timer_ms = 0u;
        }
    }
}

void Dcm_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    uint8 i;

    (void)RxPduId;  /* Single-channel: PDU ID not used for routing */

    if ((dcm_initialized == FALSE) || (dcm_config == NULL_PTR)) {
        return;
    }

    if ((PduInfoPtr == NULL_PTR) || (PduInfoPtr->SduDataPtr == NULL_PTR)) {
        return;
    }

    if ((PduInfoPtr->SduLength == 0u) || (PduInfoPtr->SduLength > DCM_TX_BUF_SIZE)) {
        return;
    }

    /* Copy request to internal buffer */
    for (i = 0u; i < PduInfoPtr->SduLength; i++) {
        dcm_rx_buf[i] = PduInfoPtr->SduDataPtr[i];
    }
    dcm_rx_len = PduInfoPtr->SduLength;
    dcm_request_pending = TRUE;
}

Dcm_SessionType Dcm_GetCurrentSession(void)
{
    return dcm_current_session;
}
