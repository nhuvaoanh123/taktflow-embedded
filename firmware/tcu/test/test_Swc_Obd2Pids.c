/**
 * @file    test_Swc_Obd2Pids.c
 * @brief   Unit tests for OBD-II PID handler SWC
 * @date    2026-02-23
 *
 * @verifies SWR-TCU-010
 *
 * Tests OBD-II Mode 01 PIDs (supported bitmap, engine load, coolant temp,
 * engine RPM, vehicle speed, control voltage, unsupported PID), Mode 03
 * (confirmed DTCs), Mode 04 (clear DTCs), and Mode 09 (VIN).
 *
 * Mocks: Rte_Read, Rte_Write, Swc_DtcStore functions
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
#define SWC_OBD2PIDS_H
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

/* OBD-II modes */
#define OBD_MODE_CURRENT_DATA   0x01u
#define OBD_MODE_CONFIRMED_DTC  0x03u
#define OBD_MODE_CLEAR_DTC      0x04u
#define OBD_MODE_VEHICLE_INFO   0x09u

/* Supported PIDs */
#define OBD_PID_SUPPORTED_00    0x00u
#define OBD_PID_ENGINE_LOAD     0x04u
#define OBD_PID_COOLANT_TEMP    0x05u
#define OBD_PID_ENGINE_RPM      0x0Cu
#define OBD_PID_VEHICLE_SPEED   0x0Du
#define OBD_PID_CONTROL_VOLTAGE 0x42u
#define OBD_PID_AMBIENT_TEMP    0x46u
#define OBD_PID_VIN             0x02u

/* VIN */
#define TCU_VIN_LENGTH  17u
#define TCU_VIN_DEFAULT "TAKTFLOW000000001"

/* DTC Store types */
#define DTC_STORE_MAX_ENTRIES   64u

#define DTC_STATUS_TEST_FAILED          0x01u
#define DTC_STATUS_TEST_FAILED_THIS_OP  0x02u
#define DTC_STATUS_PENDING              0x04u
#define DTC_STATUS_CONFIRMED            0x08u

/* DTC aging threshold */
#define TCU_DTC_AGING_CLEAR_CYCLES  40u

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

Std_ReturnType Rte_Write(uint16 SignalId, uint32 Data)
{
    if (SignalId < MOCK_RTE_MAX_SIGNALS) {
        mock_rte_signals[SignalId] = Data;
        return E_OK;
    }
    return E_NOT_OK;
}

/* ====================================================================
 * Mock: DtcStore functions used by OBD-II handler
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
    static DtcStoreEntry_t mock_entry;
    if (index < mock_dtc_count) {
        mock_entry.dtcCode = mock_dtc_codes[index];
        mock_entry.status  = mock_dtc_statuses[index];
        return &mock_entry;
    }
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

#include "../src/Swc_Obd2Pids.c"

/* ====================================================================
 * setUp / tearDown
 * ==================================================================== */

void setUp(void)
{
    uint8 i;

    for (i = 0u; i < MOCK_RTE_MAX_SIGNALS; i++) {
        mock_rte_signals[i] = 0u;
    }

    mock_dtc_count        = 0u;
    mock_dtc_clear_called = 0u;

    for (i = 0u; i < DTC_STORE_MAX_ENTRIES; i++) {
        mock_dtc_codes[i]    = 0u;
        mock_dtc_statuses[i] = 0u;
    }

    Swc_Obd2Pids_Init();
}

void tearDown(void) { }

/* ====================================================================
 * Helper
 * ==================================================================== */

static uint8  obd_rsp[64];
static uint16 obd_rsp_len;

/* ====================================================================
 * SWR-TCU-010: Mode 01, PID 0x00 -- supported PIDs bitmap
 * ==================================================================== */

/** @verifies SWR-TCU-010 */
void test_Obd2_mode01_pid00_supported_bitmap(void)
{
    obd_rsp_len = 0u;
    Std_ReturnType ret = Swc_Obd2Pids_HandleRequest(OBD_MODE_CURRENT_DATA,
                                                      OBD_PID_SUPPORTED_00,
                                                      obd_rsp, &obd_rsp_len);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    /* Response should be 4 bytes (bitmap) */
    TEST_ASSERT_EQUAL_UINT16(4u, obd_rsp_len);

    /* Build expected bitmap:
     * PID 0x04 -> bit 28 (bit 31-4 = 27) -- wait, standard is:
     * Bit 31 = PID 0x01, Bit 30 = PID 0x02, ..., Bit 0 = PID 0x20
     * PID N -> bit (32 - N)
     * PID 0x04 -> bit 28
     * PID 0x05 -> bit 27
     * PID 0x0C -> bit 20
     * PID 0x0D -> bit 19
     */
    uint32 bitmap = ((uint32)obd_rsp[0] << 24u) |
                    ((uint32)obd_rsp[1] << 16u) |
                    ((uint32)obd_rsp[2] << 8u)  |
                    ((uint32)obd_rsp[3]);

    /* PID 0x04: bit 28 set */
    TEST_ASSERT_TRUE((bitmap & (1u << 28u)) != 0u);
    /* PID 0x05: bit 27 set */
    TEST_ASSERT_TRUE((bitmap & (1u << 27u)) != 0u);
    /* PID 0x0C: bit 20 set */
    TEST_ASSERT_TRUE((bitmap & (1u << 20u)) != 0u);
    /* PID 0x0D: bit 19 set */
    TEST_ASSERT_TRUE((bitmap & (1u << 19u)) != 0u);
}

/* ====================================================================
 * SWR-TCU-010: Mode 01, PID 0x04 -- engine load
 * ==================================================================== */

/** @verifies SWR-TCU-010 */
void test_Obd2_mode01_pid04_engine_load(void)
{
    mock_rte_signals[TCU_SIG_TORQUE_PCT] = 50u;

    obd_rsp_len = 0u;
    Std_ReturnType ret = Swc_Obd2Pids_HandleRequest(OBD_MODE_CURRENT_DATA,
                                                      OBD_PID_ENGINE_LOAD,
                                                      obd_rsp, &obd_rsp_len);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT16(1u, obd_rsp_len);
    /* engine load = (torque_pct * 255) / 100 = (50 * 255) / 100 = 127 */
    TEST_ASSERT_EQUAL_UINT8(127u, obd_rsp[0]);
}

/* ====================================================================
 * SWR-TCU-010: Mode 01, PID 0x05 -- coolant temp (motor_temp + 40)
 * ==================================================================== */

/** @verifies SWR-TCU-010 */
void test_Obd2_mode01_pid05_coolant_temp(void)
{
    mock_rte_signals[TCU_SIG_MOTOR_TEMP] = 60u;

    obd_rsp_len = 0u;
    Std_ReturnType ret = Swc_Obd2Pids_HandleRequest(OBD_MODE_CURRENT_DATA,
                                                      OBD_PID_COOLANT_TEMP,
                                                      obd_rsp, &obd_rsp_len);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT16(1u, obd_rsp_len);
    /* coolant temp = motor_temp + 40 = 60 + 40 = 100 */
    TEST_ASSERT_EQUAL_UINT8(100u, obd_rsp[0]);
}

/* ====================================================================
 * SWR-TCU-010: Mode 01, PID 0x0C -- engine RPM (motor_rpm * 4)
 * ==================================================================== */

/** @verifies SWR-TCU-010 */
void test_Obd2_mode01_pid0C_engine_rpm(void)
{
    mock_rte_signals[TCU_SIG_MOTOR_RPM] = 3000u;

    obd_rsp_len = 0u;
    Std_ReturnType ret = Swc_Obd2Pids_HandleRequest(OBD_MODE_CURRENT_DATA,
                                                      OBD_PID_ENGINE_RPM,
                                                      obd_rsp, &obd_rsp_len);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT16(2u, obd_rsp_len);
    /* RPM encoding: motor_rpm * 4 = 3000 * 4 = 12000, big-endian */
    uint16 rpm_val = ((uint16)obd_rsp[0] << 8u) | (uint16)obd_rsp[1];
    TEST_ASSERT_EQUAL_UINT16(12000u, rpm_val);
}

/* ====================================================================
 * SWR-TCU-010: Mode 01, PID 0x0D -- vehicle speed
 * ==================================================================== */

/** @verifies SWR-TCU-010 */
void test_Obd2_mode01_pid0D_vehicle_speed(void)
{
    mock_rte_signals[TCU_SIG_MOTOR_RPM] = 2000u;

    obd_rsp_len = 0u;
    Std_ReturnType ret = Swc_Obd2Pids_HandleRequest(OBD_MODE_CURRENT_DATA,
                                                      OBD_PID_VEHICLE_SPEED,
                                                      obd_rsp, &obd_rsp_len);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT16(1u, obd_rsp_len);
    /* speed = motor_rpm * 60 / 1000 = 2000 * 60 / 1000 = 120 */
    TEST_ASSERT_EQUAL_UINT8(120u, obd_rsp[0]);
}

/* ====================================================================
 * SWR-TCU-010: Mode 01, PID 0x42 -- control module voltage
 * ==================================================================== */

/** @verifies SWR-TCU-010 */
void test_Obd2_mode01_pid42_voltage(void)
{
    /* battery_voltage in mV */
    mock_rte_signals[TCU_SIG_BATTERY_VOLTAGE] = 48000u;

    obd_rsp_len = 0u;
    Std_ReturnType ret = Swc_Obd2Pids_HandleRequest(OBD_MODE_CURRENT_DATA,
                                                      OBD_PID_CONTROL_VOLTAGE,
                                                      obd_rsp, &obd_rsp_len);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT16(2u, obd_rsp_len);
    /* Voltage as 2-byte big-endian mV */
    uint16 voltage = ((uint16)obd_rsp[0] << 8u) | (uint16)obd_rsp[1];
    TEST_ASSERT_EQUAL_UINT16(48000u, voltage);
}

/* ====================================================================
 * SWR-TCU-010: Mode 01, unsupported PID -> E_NOT_OK
 * ==================================================================== */

/** @verifies SWR-TCU-010 */
void test_Obd2_mode01_unsupported_pid(void)
{
    obd_rsp_len = 0u;
    Std_ReturnType ret = Swc_Obd2Pids_HandleRequest(OBD_MODE_CURRENT_DATA,
                                                      0xFFu,
                                                      obd_rsp, &obd_rsp_len);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
}

/* ====================================================================
 * SWR-TCU-010: Mode 03 -- confirmed DTCs
 * ==================================================================== */

/** @verifies SWR-TCU-010 */
void test_Obd2_mode03_confirmed_dtcs(void)
{
    /* Set up 2 confirmed DTCs in mock store */
    mock_dtc_count        = 2u;
    mock_dtc_codes[0]     = 0x001234u;
    mock_dtc_statuses[0]  = DTC_STATUS_CONFIRMED;
    mock_dtc_codes[1]     = 0x005678u;
    mock_dtc_statuses[1]  = DTC_STATUS_CONFIRMED;

    obd_rsp_len = 0u;
    Std_ReturnType ret = Swc_Obd2Pids_HandleRequest(OBD_MODE_CONFIRMED_DTC,
                                                      0x00u,
                                                      obd_rsp, &obd_rsp_len);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    /* First byte should be count of DTCs */
    TEST_ASSERT_EQUAL_UINT8(2u, obd_rsp[0]);
}

/* ====================================================================
 * SWR-TCU-010: Mode 04 -- clear DTCs
 * ==================================================================== */

/** @verifies SWR-TCU-010 */
void test_Obd2_mode04_clear_dtcs(void)
{
    mock_dtc_count = 1u;
    mock_dtc_codes[0] = 0x001234u;
    mock_dtc_statuses[0] = DTC_STATUS_CONFIRMED;

    obd_rsp_len = 0u;
    Std_ReturnType ret = Swc_Obd2Pids_HandleRequest(OBD_MODE_CLEAR_DTC,
                                                      0x00u,
                                                      obd_rsp, &obd_rsp_len);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1u, mock_dtc_clear_called);
}

/* ====================================================================
 * SWR-TCU-010: Mode 09, PID 0x02 -- VIN
 * ==================================================================== */

/** @verifies SWR-TCU-010 */
void test_Obd2_mode09_vin(void)
{
    obd_rsp_len = 0u;
    Std_ReturnType ret = Swc_Obd2Pids_HandleRequest(OBD_MODE_VEHICLE_INFO,
                                                      OBD_PID_VIN,
                                                      obd_rsp, &obd_rsp_len);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT16(TCU_VIN_LENGTH, obd_rsp_len);
    TEST_ASSERT_EQUAL_UINT8('T', obd_rsp[0]);
    TEST_ASSERT_EQUAL_UINT8('1', obd_rsp[TCU_VIN_LENGTH - 1u]);
}

/* ====================================================================
 * Test runner
 * ==================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* SWR-TCU-010: OBD-II PID responses */
    RUN_TEST(test_Obd2_mode01_pid00_supported_bitmap);
    RUN_TEST(test_Obd2_mode01_pid04_engine_load);
    RUN_TEST(test_Obd2_mode01_pid05_coolant_temp);
    RUN_TEST(test_Obd2_mode01_pid0C_engine_rpm);
    RUN_TEST(test_Obd2_mode01_pid0D_vehicle_speed);
    RUN_TEST(test_Obd2_mode01_pid42_voltage);
    RUN_TEST(test_Obd2_mode01_unsupported_pid);
    RUN_TEST(test_Obd2_mode03_confirmed_dtcs);
    RUN_TEST(test_Obd2_mode04_clear_dtcs);
    RUN_TEST(test_Obd2_mode09_vin);

    return UNITY_END();
}
