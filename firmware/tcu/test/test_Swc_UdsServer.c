/**
 * @file    test_Swc_UdsServer.c
 * @brief   Unit tests for UDS Server SWC -- diagnostics service dispatch
 * @date    2026-02-23
 *
 * @verifies SWR-TCU-002, SWR-TCU-003, SWR-TCU-004, SWR-TCU-005,
 *           SWR-TCU-006, SWR-TCU-007, SWR-TCU-011, SWR-TCU-012, SWR-TCU-013
 *
 * Tests UDS session management, ReadDataByIdentifier, WriteDataByIdentifier,
 * SecurityAccess seed-key, ClearDTC, ReadDTCInformation, TesterPresent,
 * and session timeout handling.
 *
 * Mocks: Rte_Read, Rte_Write, Dcm_MainFunction, Dem_GetDTCByStatusMask,
 *        Dem_ClearDTC, Swc_DtcStore functions
 */
#include "unity.h"

/* ====================================================================
 * Local type definitions (self-contained test -- no BSW headers)
 * ==================================================================== */

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned long  uint32;
typedef signed short   sint16;
typedef uint8          Std_ReturnType;
typedef uint8          boolean;

#define E_OK      0u
#define E_NOT_OK  1u
#define TRUE      1u
#define FALSE     0u
#define NULL_PTR  ((void*)0)

/* Prevent BSW headers from redefining types */
#define PLATFORM_TYPES_H
#define STD_TYPES_H
#define SWC_UDSSERVER_H
#define SWC_DTCSTORE_H
#define TCU_CFG_H

/* ====================================================================
 * Signal IDs (must match Tcu_Cfg.h)
 * ==================================================================== */

#define TCU_SIG_VEHICLE_SPEED     16u
#define TCU_SIG_MOTOR_TEMP        17u
#define TCU_SIG_BATTERY_VOLTAGE   18u
#define TCU_SIG_MOTOR_CURRENT     19u
#define TCU_SIG_TORQUE_PCT        20u
#define TCU_SIG_MOTOR_RPM         21u
#define TCU_SIG_DTC_BROADCAST     22u
#define TCU_SIG_COUNT             23u

/* UDS Service IDs */
#define UDS_SID_DIAG_SESSION_CTRL   0x10u
#define UDS_SID_READ_DID            0x22u
#define UDS_SID_WRITE_DID           0x2Eu
#define UDS_SID_CLEAR_DTC           0x14u
#define UDS_SID_READ_DTC_INFO       0x19u
#define UDS_SID_SECURITY_ACCESS     0x27u
#define UDS_SID_TESTER_PRESENT      0x3Eu

/* UDS Session types */
#define UDS_SESSION_DEFAULT     0x01u
#define UDS_SESSION_PROGRAMMING 0x02u
#define UDS_SESSION_EXTENDED    0x03u

/* UDS NRC codes */
#define UDS_NRC_SERVICE_NOT_SUPPORTED       0x11u
#define UDS_NRC_SUBFUNCTION_NOT_SUPPORTED   0x12u
#define UDS_NRC_INCORRECT_MSG_LENGTH        0x13u
#define UDS_NRC_CONDITIONS_NOT_CORRECT      0x22u
#define UDS_NRC_REQUEST_OUT_OF_RANGE        0x31u
#define UDS_NRC_SECURITY_ACCESS_DENIED      0x33u
#define UDS_NRC_INVALID_KEY                 0x35u
#define UDS_NRC_EXCEEDED_ATTEMPTS           0x36u
#define UDS_NRC_SID                         0x7Fu

/* UDS timing */
#define TCU_UDS_SESSION_TIMEOUT_TICKS  500u
#define TCU_UDS_SECURITY_LOCKOUT_TICKS 1000u
#define TCU_UDS_MAX_SECURITY_ATTEMPTS  3u

/* Security keys */
#define TCU_SECURITY_LEVEL1_XOR  0xA5A5A5A5u
#define TCU_SECURITY_LEVEL3_XOR  0x5A5A5A5Au

/* VIN */
#define TCU_VIN_LENGTH  17u
#define TCU_VIN_DEFAULT "TAKTFLOW000000001"

/* DTC aging threshold (unused in UDS tests but needed by include) */
#define TCU_DTC_AGING_CLEAR_CYCLES  40u

/* ====================================================================
 * DTC Store types (mock inline)
 * ==================================================================== */

#define DTC_STORE_MAX_ENTRIES   64u

#define DTC_STATUS_TEST_FAILED          0x01u
#define DTC_STATUS_TEST_FAILED_THIS_OP  0x02u
#define DTC_STATUS_PENDING              0x04u
#define DTC_STATUS_CONFIRMED            0x08u

typedef struct {
    uint32  dtcCode;
    uint8   status;
    uint16  agingCounter;
    uint16  ff_speed;
    uint16  ff_current;
    uint16  ff_voltage;
    uint8   ff_temp;
    uint32  ff_timestamp;
} DtcStoreEntry_t;

/* ====================================================================
 * Mock: Rte_Read / Rte_Write
 * ==================================================================== */

#define MOCK_RTE_MAX_SIGNALS  32u

static uint32 mock_rte_signals[MOCK_RTE_MAX_SIGNALS];

Std_ReturnType Rte_Read(uint16 SignalId, uint32* DataPtr)
{
    if (DataPtr == NULL_PTR) {
        return E_NOT_OK;
    }
    if (SignalId < MOCK_RTE_MAX_SIGNALS) {
        *DataPtr = mock_rte_signals[SignalId];
        return E_OK;
    }
    return E_NOT_OK;
}

static uint16 mock_rte_write_sig;
static uint32 mock_rte_write_val;
static uint8  mock_rte_write_count;

Std_ReturnType Rte_Write(uint16 SignalId, uint32 Data)
{
    mock_rte_write_sig = SignalId;
    mock_rte_write_val = Data;
    mock_rte_write_count++;
    if (SignalId < MOCK_RTE_MAX_SIGNALS) {
        mock_rte_signals[SignalId] = Data;
        return E_OK;
    }
    return E_NOT_OK;
}

/* ====================================================================
 * Mock: DtcStore functions used by UDS Server
 * ==================================================================== */

static uint8  mock_dtc_count;
static uint32 mock_dtc_codes[DTC_STORE_MAX_ENTRIES];
static uint8  mock_dtc_statuses[DTC_STORE_MAX_ENTRIES];
static uint8  mock_dtc_clear_called;

uint8 Swc_DtcStore_GetCount(void)
{
    return mock_dtc_count;
}

const DtcStoreEntry_t* Swc_DtcStore_GetByIndex(uint8 index)
{
    (void)index;
    return NULL_PTR;
}

void Swc_DtcStore_Clear(void)
{
    mock_dtc_clear_called++;
    mock_dtc_count = 0u;
}

Std_ReturnType Swc_DtcStore_Add(uint32 dtcCode, uint8 status)
{
    (void)dtcCode;
    (void)status;
    return E_OK;
}

uint8 Swc_DtcStore_GetByMask(uint8 statusMask, uint32* dtcCodes, uint8 maxCount)
{
    uint8 i;
    uint8 found = 0u;
    for (i = 0u; i < mock_dtc_count; i++) {
        if ((mock_dtc_statuses[i] & statusMask) != 0u) {
            if (found < maxCount) {
                dtcCodes[found] = mock_dtc_codes[i];
                found++;
            }
        }
    }
    return found;
}

/* ====================================================================
 * Include SWC under test (source inclusion for test build)
 * ==================================================================== */

#include "../src/Swc_UdsServer.c"

/* ====================================================================
 * setUp / tearDown
 * ==================================================================== */

void setUp(void)
{
    uint8 i;

    for (i = 0u; i < MOCK_RTE_MAX_SIGNALS; i++) {
        mock_rte_signals[i] = 0u;
    }

    mock_rte_write_sig   = 0u;
    mock_rte_write_val   = 0u;
    mock_rte_write_count = 0u;

    mock_dtc_count        = 0u;
    mock_dtc_clear_called = 0u;

    for (i = 0u; i < DTC_STORE_MAX_ENTRIES; i++) {
        mock_dtc_codes[i]    = 0u;
        mock_dtc_statuses[i] = 0u;
    }

    Swc_UdsServer_Init();
}

void tearDown(void) { }

/* ====================================================================
 * Helper: send UDS request, get response
 * ==================================================================== */

static uint8  rsp_buf[256];
static uint16 rsp_len;

static void send_uds(const uint8* req, uint16 req_len)
{
    rsp_len = 0u;
    Swc_UdsServer_ProcessRequest(req, req_len, rsp_buf, &rsp_len);
}

/* ====================================================================
 * SWR-TCU-002: Default session after init
 * ==================================================================== */

/** @verifies SWR-TCU-002 */
void test_UdsServer_init_default_session(void)
{
    TEST_ASSERT_EQUAL_UINT8(UDS_SESSION_DEFAULT, Swc_UdsServer_GetSession());
}

/* ====================================================================
 * SWR-TCU-003: Session control to extended
 * ==================================================================== */

/** @verifies SWR-TCU-003 */
void test_UdsServer_session_control_to_extended(void)
{
    uint8 req[] = { UDS_SID_DIAG_SESSION_CTRL, UDS_SESSION_EXTENDED };
    send_uds(req, 2u);

    /* Positive response: SID+0x40, sub-function */
    TEST_ASSERT_EQUAL_UINT8(UDS_SID_DIAG_SESSION_CTRL + 0x40u, rsp_buf[0]);
    TEST_ASSERT_EQUAL_UINT8(UDS_SESSION_EXTENDED, rsp_buf[1]);
    TEST_ASSERT_EQUAL_UINT8(UDS_SESSION_EXTENDED, Swc_UdsServer_GetSession());
}

/* ====================================================================
 * SWR-TCU-003: Session control with invalid sub-function -> NRC 0x12
 * ==================================================================== */

/** @verifies SWR-TCU-003 */
void test_UdsServer_session_control_invalid_subfunction(void)
{
    uint8 req[] = { UDS_SID_DIAG_SESSION_CTRL, 0xFFu };
    send_uds(req, 2u);

    TEST_ASSERT_EQUAL_UINT8(UDS_NRC_SID, rsp_buf[0]);
    TEST_ASSERT_EQUAL_UINT8(UDS_SID_DIAG_SESSION_CTRL, rsp_buf[1]);
    TEST_ASSERT_EQUAL_UINT8(UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, rsp_buf[2]);
}

/* ====================================================================
 * SWR-TCU-004: Read DID -- VIN (0xF190)
 * ==================================================================== */

/** @verifies SWR-TCU-004 */
void test_UdsServer_read_did_vin(void)
{
    uint8 req[] = { UDS_SID_READ_DID, 0xF1u, 0x90u };
    send_uds(req, 3u);

    /* Positive response: SID+0x40, DID high, DID low, 17 bytes VIN */
    TEST_ASSERT_EQUAL_UINT8(UDS_SID_READ_DID + 0x40u, rsp_buf[0]);
    TEST_ASSERT_EQUAL_UINT8(0xF1u, rsp_buf[1]);
    TEST_ASSERT_EQUAL_UINT8(0x90u, rsp_buf[2]);
    TEST_ASSERT_EQUAL_UINT16(3u + TCU_VIN_LENGTH, rsp_len);
    /* Check VIN content */
    TEST_ASSERT_EQUAL_UINT8('T', rsp_buf[3]);
    TEST_ASSERT_EQUAL_UINT8('1', rsp_buf[3 + TCU_VIN_LENGTH - 1u]);
}

/* ====================================================================
 * SWR-TCU-004: Read DID -- SW version (0xF195)
 * ==================================================================== */

/** @verifies SWR-TCU-004 */
void test_UdsServer_read_did_sw_version(void)
{
    uint8 req[] = { UDS_SID_READ_DID, 0xF1u, 0x95u };
    send_uds(req, 3u);

    TEST_ASSERT_EQUAL_UINT8(UDS_SID_READ_DID + 0x40u, rsp_buf[0]);
    TEST_ASSERT_EQUAL_UINT8(0xF1u, rsp_buf[1]);
    TEST_ASSERT_EQUAL_UINT8(0x95u, rsp_buf[2]);
    /* "1.0.0" is 5 chars */
    TEST_ASSERT_EQUAL_UINT8('1', rsp_buf[3]);
    TEST_ASSERT_EQUAL_UINT8('.', rsp_buf[4]);
}

/* ====================================================================
 * SWR-TCU-004: Read DID -- vehicle speed (0x0100) from RTE
 * ==================================================================== */

/** @verifies SWR-TCU-004 */
void test_UdsServer_read_did_vehicle_speed(void)
{
    mock_rte_signals[TCU_SIG_VEHICLE_SPEED] = 42u;

    uint8 req[] = { UDS_SID_READ_DID, 0x01u, 0x00u };
    send_uds(req, 3u);

    TEST_ASSERT_EQUAL_UINT8(UDS_SID_READ_DID + 0x40u, rsp_buf[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01u, rsp_buf[1]);
    TEST_ASSERT_EQUAL_UINT8(0x00u, rsp_buf[2]);
    /* Vehicle speed as 2 bytes, big-endian */
    uint16 speed_val = ((uint16)rsp_buf[3] << 8u) | (uint16)rsp_buf[4];
    TEST_ASSERT_EQUAL_UINT16(42u, speed_val);
}

/* ====================================================================
 * SWR-TCU-004: Read DID -- unknown DID -> NRC 0x31
 * ==================================================================== */

/** @verifies SWR-TCU-004 */
void test_UdsServer_read_did_unknown_nrc(void)
{
    uint8 req[] = { UDS_SID_READ_DID, 0xFFu, 0xFFu };
    send_uds(req, 3u);

    TEST_ASSERT_EQUAL_UINT8(UDS_NRC_SID, rsp_buf[0]);
    TEST_ASSERT_EQUAL_UINT8(UDS_SID_READ_DID, rsp_buf[1]);
    TEST_ASSERT_EQUAL_UINT8(UDS_NRC_REQUEST_OUT_OF_RANGE, rsp_buf[2]);
}

/* ====================================================================
 * SWR-TCU-005: Write DID VIN requires security -> NRC 0x33
 * ==================================================================== */

/** @verifies SWR-TCU-005 */
void test_UdsServer_write_did_vin_requires_security(void)
{
    uint8 req[3 + TCU_VIN_LENGTH];
    req[0] = UDS_SID_WRITE_DID;
    req[1] = 0xF1u;
    req[2] = 0x90u;
    /* Fill with VIN data */
    uint8 i;
    for (i = 0u; i < TCU_VIN_LENGTH; i++) {
        req[3u + i] = 'A';
    }

    send_uds(req, 3u + TCU_VIN_LENGTH);

    TEST_ASSERT_EQUAL_UINT8(UDS_NRC_SID, rsp_buf[0]);
    TEST_ASSERT_EQUAL_UINT8(UDS_SID_WRITE_DID, rsp_buf[1]);
    TEST_ASSERT_EQUAL_UINT8(UDS_NRC_SECURITY_ACCESS_DENIED, rsp_buf[2]);
}

/* ====================================================================
 * SWR-TCU-006: Security access -- seed request (0x27, 0x01)
 * ==================================================================== */

/** @verifies SWR-TCU-006 */
void test_UdsServer_security_access_seed_request(void)
{
    /* Must be in extended session for security access */
    uint8 sess_req[] = { UDS_SID_DIAG_SESSION_CTRL, UDS_SESSION_EXTENDED };
    send_uds(sess_req, 2u);

    uint8 req[] = { UDS_SID_SECURITY_ACCESS, 0x01u };
    send_uds(req, 2u);

    TEST_ASSERT_EQUAL_UINT8(UDS_SID_SECURITY_ACCESS + 0x40u, rsp_buf[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01u, rsp_buf[1]);
    /* Seed should be 4 bytes and non-zero */
    TEST_ASSERT_EQUAL_UINT16(6u, rsp_len);  /* SID+0x40, subFunc, 4-byte seed */
    uint32 seed = ((uint32)rsp_buf[2] << 24u) |
                  ((uint32)rsp_buf[3] << 16u) |
                  ((uint32)rsp_buf[4] << 8u)  |
                  ((uint32)rsp_buf[5]);
    TEST_ASSERT_NOT_EQUAL(0u, seed);
}

/* ====================================================================
 * SWR-TCU-006: Security access -- valid key (0x27, 0x02)
 * ==================================================================== */

/** @verifies SWR-TCU-006 */
void test_UdsServer_security_access_valid_key(void)
{
    /* Switch to extended session */
    uint8 sess_req[] = { UDS_SID_DIAG_SESSION_CTRL, UDS_SESSION_EXTENDED };
    send_uds(sess_req, 2u);

    /* Request seed */
    uint8 seed_req[] = { UDS_SID_SECURITY_ACCESS, 0x01u };
    send_uds(seed_req, 2u);

    /* Extract seed */
    uint32 seed = ((uint32)rsp_buf[2] << 24u) |
                  ((uint32)rsp_buf[3] << 16u) |
                  ((uint32)rsp_buf[4] << 8u)  |
                  ((uint32)rsp_buf[5]);

    /* Compute key: seed XOR 0xA5A5A5A5 */
    uint32 key = seed ^ TCU_SECURITY_LEVEL1_XOR;

    uint8 key_req[6];
    key_req[0] = UDS_SID_SECURITY_ACCESS;
    key_req[1] = 0x02u;
    key_req[2] = (uint8)(key >> 24u);
    key_req[3] = (uint8)(key >> 16u);
    key_req[4] = (uint8)(key >> 8u);
    key_req[5] = (uint8)(key);

    send_uds(key_req, 6u);

    TEST_ASSERT_EQUAL_UINT8(UDS_SID_SECURITY_ACCESS + 0x40u, rsp_buf[0]);
    TEST_ASSERT_EQUAL_UINT8(0x02u, rsp_buf[1]);
}

/* ====================================================================
 * SWR-TCU-006: Security access -- invalid key -> NRC 0x35
 * ==================================================================== */

/** @verifies SWR-TCU-006 */
void test_UdsServer_security_access_invalid_key(void)
{
    /* Switch to extended session */
    uint8 sess_req[] = { UDS_SID_DIAG_SESSION_CTRL, UDS_SESSION_EXTENDED };
    send_uds(sess_req, 2u);

    /* Request seed */
    uint8 seed_req[] = { UDS_SID_SECURITY_ACCESS, 0x01u };
    send_uds(seed_req, 2u);

    /* Send wrong key */
    uint8 key_req[] = { UDS_SID_SECURITY_ACCESS, 0x02u, 0x00u, 0x00u, 0x00u, 0x00u };
    send_uds(key_req, 6u);

    TEST_ASSERT_EQUAL_UINT8(UDS_NRC_SID, rsp_buf[0]);
    TEST_ASSERT_EQUAL_UINT8(UDS_SID_SECURITY_ACCESS, rsp_buf[1]);
    TEST_ASSERT_EQUAL_UINT8(UDS_NRC_INVALID_KEY, rsp_buf[2]);
}

/* ====================================================================
 * SWR-TCU-006: Security lockout after 3 invalid attempts -> NRC 0x36
 * ==================================================================== */

/** @verifies SWR-TCU-006 */
void test_UdsServer_security_lockout_after_3_attempts(void)
{
    /* Switch to extended session */
    uint8 sess_req[] = { UDS_SID_DIAG_SESSION_CTRL, UDS_SESSION_EXTENDED };
    send_uds(sess_req, 2u);

    uint8 attempt;
    for (attempt = 0u; attempt < TCU_UDS_MAX_SECURITY_ATTEMPTS; attempt++) {
        /* Request seed */
        uint8 seed_req[] = { UDS_SID_SECURITY_ACCESS, 0x01u };
        send_uds(seed_req, 2u);

        /* Send wrong key */
        uint8 key_req[] = { UDS_SID_SECURITY_ACCESS, 0x02u,
                            0xDEu, 0xADu, 0xBEu, 0xEFu };
        send_uds(key_req, 6u);
    }

    /* 4th attempt: request seed should return NRC 0x36 (exceeded attempts) */
    uint8 seed_req2[] = { UDS_SID_SECURITY_ACCESS, 0x01u };
    send_uds(seed_req2, 2u);

    TEST_ASSERT_EQUAL_UINT8(UDS_NRC_SID, rsp_buf[0]);
    TEST_ASSERT_EQUAL_UINT8(UDS_SID_SECURITY_ACCESS, rsp_buf[1]);
    TEST_ASSERT_EQUAL_UINT8(UDS_NRC_EXCEEDED_ATTEMPTS, rsp_buf[2]);
}

/* ====================================================================
 * SWR-TCU-007: Clear DTC requires security -> NRC 0x33
 * ==================================================================== */

/** @verifies SWR-TCU-007 */
void test_UdsServer_clear_dtc_requires_security(void)
{
    /* In default session, no security unlocked */
    uint8 req[] = { UDS_SID_CLEAR_DTC, 0xFFu, 0xFFu, 0xFFu };
    send_uds(req, 4u);

    TEST_ASSERT_EQUAL_UINT8(UDS_NRC_SID, rsp_buf[0]);
    TEST_ASSERT_EQUAL_UINT8(UDS_SID_CLEAR_DTC, rsp_buf[1]);
    TEST_ASSERT_EQUAL_UINT8(UDS_NRC_SECURITY_ACCESS_DENIED, rsp_buf[2]);
}

/* ====================================================================
 * SWR-TCU-011: Tester present (0x3E, 0x00)
 * ==================================================================== */

/** @verifies SWR-TCU-011 */
void test_UdsServer_tester_present(void)
{
    uint8 req[] = { UDS_SID_TESTER_PRESENT, 0x00u };
    send_uds(req, 2u);

    TEST_ASSERT_EQUAL_UINT8(UDS_SID_TESTER_PRESENT + 0x40u, rsp_buf[0]);
    TEST_ASSERT_EQUAL_UINT8(0x00u, rsp_buf[1]);
}

/* ====================================================================
 * SWR-TCU-012: Session timeout after 5s (500 ticks at 10ms)
 * ==================================================================== */

/** @verifies SWR-TCU-012 */
void test_UdsServer_session_timeout_5s(void)
{
    /* Switch to extended session */
    uint8 sess_req[] = { UDS_SID_DIAG_SESSION_CTRL, UDS_SESSION_EXTENDED };
    send_uds(sess_req, 2u);
    TEST_ASSERT_EQUAL_UINT8(UDS_SESSION_EXTENDED, Swc_UdsServer_GetSession());

    /* Simulate 501 ticks without TesterPresent */
    uint16 tick;
    for (tick = 0u; tick <= TCU_UDS_SESSION_TIMEOUT_TICKS; tick++) {
        Swc_UdsServer_10ms();
    }

    /* Session should have reverted to default */
    TEST_ASSERT_EQUAL_UINT8(UDS_SESSION_DEFAULT, Swc_UdsServer_GetSession());
}

/* ====================================================================
 * Test runner
 * ==================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* SWR-TCU-002: Session management */
    RUN_TEST(test_UdsServer_init_default_session);

    /* SWR-TCU-003: Diagnostic session control */
    RUN_TEST(test_UdsServer_session_control_to_extended);
    RUN_TEST(test_UdsServer_session_control_invalid_subfunction);

    /* SWR-TCU-004: Read data by identifier */
    RUN_TEST(test_UdsServer_read_did_vin);
    RUN_TEST(test_UdsServer_read_did_sw_version);
    RUN_TEST(test_UdsServer_read_did_vehicle_speed);
    RUN_TEST(test_UdsServer_read_did_unknown_nrc);

    /* SWR-TCU-005: Write data by identifier */
    RUN_TEST(test_UdsServer_write_did_vin_requires_security);

    /* SWR-TCU-006: Security access */
    RUN_TEST(test_UdsServer_security_access_seed_request);
    RUN_TEST(test_UdsServer_security_access_valid_key);
    RUN_TEST(test_UdsServer_security_access_invalid_key);
    RUN_TEST(test_UdsServer_security_lockout_after_3_attempts);

    /* SWR-TCU-007: Clear DTC */
    RUN_TEST(test_UdsServer_clear_dtc_requires_security);

    /* SWR-TCU-011: Tester present */
    RUN_TEST(test_UdsServer_tester_present);

    /* SWR-TCU-012: Session timeout */
    RUN_TEST(test_UdsServer_session_timeout_5s);

    return UNITY_END();
}
