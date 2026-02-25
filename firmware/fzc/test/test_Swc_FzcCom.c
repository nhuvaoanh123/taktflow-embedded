/**
 * @file    test_Swc_FzcCom.c
 * @brief   Unit tests for Swc_FzcCom — CAN E2E protection, message RX/TX
 * @date    2026-02-24
 *
 * @verifies SWR-FZC-019, SWR-FZC-020, SWR-FZC-026, SWR-FZC-027
 *
 * Tests E2E protection (CRC-8 0x1D + alive counter + Data ID) on TX,
 * E2E verification on RX with safe defaults (brake 100%, steering center),
 * CAN message reception routing (E-stop, vehicle state, brake, steering),
 * CAN message transmission scheduling (heartbeat 50ms, event-driven).
 *
 * Mocks: Com_ReceiveSignal, Com_SendSignal, Rte_Read, Rte_Write
 */
#include "unity.h"

/* ==================================================================
 * Local type definitions (avoid BSW header mock conflicts)
 * ================================================================== */

typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned int   uint32;
typedef signed short    sint16;
typedef signed int     sint32;
typedef uint8           Std_ReturnType;

#define E_OK        0u
#define E_NOT_OK    1u
#define TRUE        1u
#define FALSE       0u
#define NULL_PTR    ((void*)0)

/* ==================================================================
 * FZC Config Constants (from Fzc_Cfg.h)
 * ================================================================== */

#define FZC_SIG_STEER_CMD          16u
#define FZC_SIG_STEER_ANGLE        17u
#define FZC_SIG_STEER_FAULT        18u
#define FZC_SIG_BRAKE_CMD          19u
#define FZC_SIG_BRAKE_POS          20u
#define FZC_SIG_BRAKE_FAULT        21u
#define FZC_SIG_LIDAR_DIST         22u
#define FZC_SIG_LIDAR_SIGNAL       23u
#define FZC_SIG_LIDAR_ZONE         24u
#define FZC_SIG_LIDAR_FAULT        25u
#define FZC_SIG_VEHICLE_STATE      26u
#define FZC_SIG_ESTOP_ACTIVE       27u
#define FZC_SIG_BUZZER_PATTERN     28u
#define FZC_SIG_MOTOR_CUTOFF       29u
#define FZC_SIG_FAULT_MASK         30u
#define FZC_SIG_STEER_PWM_DISABLE  31u
#define FZC_SIG_BRAKE_PWM_DISABLE  32u
#define FZC_SIG_SELF_TEST_RESULT   33u
#define FZC_SIG_HEARTBEAT_ALIVE    34u
#define FZC_SIG_SAFETY_STATUS      35u
#define FZC_SIG_COUNT              36u

/* Com PDU IDs */
#define FZC_COM_TX_HEARTBEAT       0u
#define FZC_COM_TX_STEER_STATUS    1u
#define FZC_COM_TX_BRAKE_STATUS    2u
#define FZC_COM_TX_BRAKE_FAULT     3u
#define FZC_COM_TX_MOTOR_CUTOFF    4u
#define FZC_COM_TX_LIDAR           5u

#define FZC_COM_RX_ESTOP           0u
#define FZC_COM_RX_VEHICLE_STATE   1u
#define FZC_COM_RX_STEER_CMD       2u
#define FZC_COM_RX_BRAKE_CMD       3u

/* E2E Data IDs */
#define FZC_E2E_HEARTBEAT_DATA_ID    0x11u
#define FZC_E2E_STEER_STATUS_DATA_ID 0x20u
#define FZC_E2E_BRAKE_STATUS_DATA_ID 0x21u
#define FZC_E2E_LIDAR_DATA_ID        0x22u
#define FZC_E2E_ESTOP_DATA_ID        0x01u
#define FZC_E2E_VEHSTATE_DATA_ID     0x10u
#define FZC_E2E_STEER_CMD_DATA_ID    0x12u
#define FZC_E2E_BRAKE_CMD_DATA_ID    0x13u

/* Brake/vehicle constants */
#define FZC_BRAKE_NO_FAULT          0u
#define FZC_LIDAR_ZONE_CLEAR        0u
#define FZC_LIDAR_ZONE_WARNING      1u
#define FZC_ECU_ID                0x02u
#define FZC_STATE_RUN               1u

/* ==================================================================
 * Swc_FzcCom API declarations
 * ================================================================== */

extern void            Swc_FzcCom_Init(void);
extern Std_ReturnType  Swc_FzcCom_E2eProtect(uint8* data, uint8 length, uint8 dataId);
extern Std_ReturnType  Swc_FzcCom_E2eCheck(const uint8* data, uint8 length, uint8 dataId);
extern void            Swc_FzcCom_Receive(void);
extern void            Swc_FzcCom_TransmitSchedule(void);

/* ==================================================================
 * Mock: Rte_Read / Rte_Write
 * ================================================================== */

#define MOCK_RTE_MAX_SIGNALS  64u

static uint32  mock_rte_signals[MOCK_RTE_MAX_SIGNALS];
static uint8   mock_rte_write_count;

Std_ReturnType Rte_Write(uint16 SignalId, uint32 Data)
{
    mock_rte_write_count++;
    if (SignalId < MOCK_RTE_MAX_SIGNALS) {
        mock_rte_signals[SignalId] = Data;
        return E_OK;
    }
    return E_NOT_OK;
}

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

/* ==================================================================
 * Mock: Com_ReceiveSignal / Com_SendSignal
 * ================================================================== */

#define MOCK_COM_MAX_PDUS  16u

static uint8  mock_com_rx_data[MOCK_COM_MAX_PDUS][8];
static uint8  mock_com_rx_available[MOCK_COM_MAX_PDUS];
static uint8  mock_com_tx_data[MOCK_COM_MAX_PDUS][8];
static uint8  mock_com_tx_count;
static uint8  mock_com_tx_last_pdu;

Std_ReturnType Com_ReceiveSignal(uint8 PduId, uint8* DataPtr)
{
    uint8 i;
    if (DataPtr == NULL_PTR) {
        return E_NOT_OK;
    }
    if (PduId >= MOCK_COM_MAX_PDUS) {
        return E_NOT_OK;
    }
    if (mock_com_rx_available[PduId] != TRUE) {
        return E_NOT_OK;
    }
    for (i = 0u; i < 8u; i++) {
        DataPtr[i] = mock_com_rx_data[PduId][i];
    }
    return E_OK;
}

Std_ReturnType Com_SendSignal(uint8 PduId, const void* DataPtr)
{
    const uint8* ptr;
    uint8 i;
    if (DataPtr == NULL_PTR) {
        return E_NOT_OK;
    }
    mock_com_tx_count++;
    mock_com_tx_last_pdu = PduId;
    ptr = (const uint8*)DataPtr;
    if (PduId < MOCK_COM_MAX_PDUS) {
        for (i = 0u; i < 8u; i++) {
            mock_com_tx_data[PduId][i] = ptr[i];
        }
    }
    return E_OK;
}

/* ==================================================================
 * Test Configuration
 * ================================================================== */

void setUp(void)
{
    uint8 i;
    uint8 j;

    mock_rte_write_count = 0u;
    for (i = 0u; i < MOCK_RTE_MAX_SIGNALS; i++) {
        mock_rte_signals[i] = 0u;
    }
    mock_rte_signals[FZC_SIG_VEHICLE_STATE] = FZC_STATE_RUN;

    mock_com_tx_count    = 0u;
    mock_com_tx_last_pdu = 0xFFu;
    for (i = 0u; i < MOCK_COM_MAX_PDUS; i++) {
        mock_com_rx_available[i] = FALSE;
        for (j = 0u; j < 8u; j++) {
            mock_com_rx_data[i][j] = 0u;
            mock_com_tx_data[i][j] = 0u;
        }
    }

    Swc_FzcCom_Init();
}

void tearDown(void) { }

/* ==================================================================
 * Helper: prepare a valid E2E-protected RX message
 * ================================================================== */

static void prepare_valid_rx(uint8 pduId, uint8 dataId, uint8 payloadByte2)
{
    uint8 buf[8];
    uint8 i;

    for (i = 0u; i < 8u; i++) {
        buf[i] = 0u;
    }
    buf[2] = payloadByte2;

    /* Use E2eProtect to create valid CRC + alive counter */
    (void)Swc_FzcCom_E2eProtect(buf, 8u, dataId);

    for (i = 0u; i < 8u; i++) {
        mock_com_rx_data[pduId][i] = buf[i];
    }
    mock_com_rx_available[pduId] = TRUE;
}

/* ==================================================================
 * SWR-FZC-019: E2E Transmit (2 tests)
 * ================================================================== */

/** @verifies SWR-FZC-019 — E2E protect computes CRC and inserts alive counter */
void test_FzcCom_e2e_protect_crc_and_alive(void)
{
    uint8 data[8] = {0u, 0u, 0xAAu, 0x55u, 0u, 0u, 0u, 0u};
    Std_ReturnType ret;

    ret = Swc_FzcCom_E2eProtect(data, 8u, FZC_E2E_HEARTBEAT_DATA_ID);

    /* assert: success */
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);

    /* assert: CRC in byte[0] is non-zero (computed over data + Data ID) */
    TEST_ASSERT_TRUE(data[0] != 0u);

    /* assert: alive counter in byte[1] bits [3:0] starts at 0 (first call after init) */
    TEST_ASSERT_EQUAL_UINT8(0u, (uint8)(data[1] & 0x0Fu));

    /* Call again: alive counter should increment to 1 */
    data[0] = 0u;
    data[1] = 0u;
    ret = Swc_FzcCom_E2eProtect(data, 8u, FZC_E2E_HEARTBEAT_DATA_ID);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1u, (uint8)(data[1] & 0x0Fu));
}

/** @verifies SWR-FZC-019 — E2E protect rejects NULL data */
void test_FzcCom_e2e_protect_null_rejected(void)
{
    Std_ReturnType ret = Swc_FzcCom_E2eProtect(NULL_PTR, 8u, 0x11u);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
}

/* ==================================================================
 * SWR-FZC-020: E2E Receive (3 tests)
 * ================================================================== */

/** @verifies SWR-FZC-020 — E2E check accepts valid protected message */
void test_FzcCom_e2e_check_valid(void)
{
    uint8 data[8] = {0u, 0u, 0xBBu, 0u, 0u, 0u, 0u, 0u};

    /* Protect it first */
    (void)Swc_FzcCom_E2eProtect(data, 8u, FZC_E2E_BRAKE_CMD_DATA_ID);

    /* Now check: should pass */
    Std_ReturnType ret = Swc_FzcCom_E2eCheck(data, 8u, FZC_E2E_BRAKE_CMD_DATA_ID);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
}

/** @verifies SWR-FZC-020 — E2E check failure on brake routes safe default 100% */
void test_FzcCom_e2e_check_fail_brake_safe_default_100pct(void)
{
    uint8 i;

    /* Prepare a corrupted brake command RX message */
    for (i = 0u; i < 8u; i++) {
        mock_com_rx_data[FZC_COM_RX_BRAKE_CMD][i] = 0xFFu;  /* Invalid CRC */
    }
    mock_com_rx_available[FZC_COM_RX_BRAKE_CMD] = TRUE;

    /* Run receive */
    Swc_FzcCom_Receive();

    /* assert: brake command routed as safe default = 100% */
    TEST_ASSERT_EQUAL_UINT32(100u, mock_rte_signals[FZC_SIG_BRAKE_CMD]);
}

/** @verifies SWR-FZC-020 — E2E check failure on steering routes safe default center */
void test_FzcCom_e2e_check_fail_steering_safe_default_center(void)
{
    uint8 i;

    /* Prepare a corrupted steering command RX message */
    for (i = 0u; i < 8u; i++) {
        mock_com_rx_data[FZC_COM_RX_STEER_CMD][i] = 0xFFu;  /* Invalid CRC */
    }
    mock_com_rx_available[FZC_COM_RX_STEER_CMD] = TRUE;

    /* Run receive */
    Swc_FzcCom_Receive();

    /* assert: steering command routed as safe default = center (0 deg) */
    TEST_ASSERT_EQUAL_UINT32(0u, mock_rte_signals[FZC_SIG_STEER_CMD]);
}

/* ==================================================================
 * SWR-FZC-026: CAN Message Reception (3 tests)
 * ================================================================== */

/** @verifies SWR-FZC-026 — E-stop message (0x001) routes to RTE */
void test_FzcCom_receive_estop_routing(void)
{
    /* Prepare valid E-stop message with E-stop active = 1 */
    prepare_valid_rx(FZC_COM_RX_ESTOP, FZC_E2E_ESTOP_DATA_ID, 1u);

    Swc_FzcCom_Receive();

    /* assert: E-stop active signal written to RTE */
    TEST_ASSERT_EQUAL_UINT32(1u, mock_rte_signals[FZC_SIG_ESTOP_ACTIVE]);
}

/** @verifies SWR-FZC-026 — Brake command (0x200) routes to RTE */
void test_FzcCom_receive_brake_cmd_routing(void)
{
    /* Prepare valid brake command with 50% braking */
    prepare_valid_rx(FZC_COM_RX_BRAKE_CMD, FZC_E2E_BRAKE_CMD_DATA_ID, 50u);

    Swc_FzcCom_Receive();

    /* assert: brake command signal = 50 */
    TEST_ASSERT_EQUAL_UINT32(50u, mock_rte_signals[FZC_SIG_BRAKE_CMD]);
}

/** @verifies SWR-FZC-026 — Steering command (0x201) routes to RTE */
void test_FzcCom_receive_steering_cmd_routing(void)
{
    uint8 buf[8];
    uint8 i;

    for (i = 0u; i < 8u; i++) {
        buf[i] = 0u;
    }

    /* Steering angle = 15 deg (sint16) in bytes [2..3] little-endian */
    buf[2] = 15u;
    buf[3] = 0u;

    (void)Swc_FzcCom_E2eProtect(buf, 8u, FZC_E2E_STEER_CMD_DATA_ID);

    for (i = 0u; i < 8u; i++) {
        mock_com_rx_data[FZC_COM_RX_STEER_CMD][i] = buf[i];
    }
    mock_com_rx_available[FZC_COM_RX_STEER_CMD] = TRUE;

    Swc_FzcCom_Receive();

    /* assert: steering command signal = 15 */
    TEST_ASSERT_EQUAL_UINT32(15u, mock_rte_signals[FZC_SIG_STEER_CMD]);
}

/* ==================================================================
 * SWR-FZC-027: CAN Message Transmission (2 tests)
 * ================================================================== */

/** @verifies SWR-FZC-027 — Heartbeat transmitted every 50ms (5 cycles) */
void test_FzcCom_transmit_heartbeat_50ms(void)
{
    uint8 i;

    /* Run 4 cycles: no heartbeat TX yet */
    for (i = 0u; i < 4u; i++) {
        Swc_FzcCom_TransmitSchedule();
    }
    TEST_ASSERT_EQUAL_UINT8(0u, mock_com_tx_count);

    /* 5th cycle: heartbeat should be transmitted */
    Swc_FzcCom_TransmitSchedule();
    TEST_ASSERT_TRUE(mock_com_tx_count > 0u);
    TEST_ASSERT_EQUAL_UINT8(FZC_COM_TX_HEARTBEAT, mock_com_tx_last_pdu);
}

/** @verifies SWR-FZC-027 — Event-driven brake fault TX within 10ms of trigger */
void test_FzcCom_transmit_event_driven_within_10ms(void)
{
    /* Set brake fault active in RTE */
    mock_rte_signals[FZC_SIG_BRAKE_FAULT] = 1u;

    /* Single cycle should trigger event-driven TX */
    Swc_FzcCom_TransmitSchedule();

    /* assert: brake fault PDU was sent */
    TEST_ASSERT_TRUE(mock_com_tx_count > 0u);

    /* Verify it was the brake fault PDU (among possibly heartbeat too) */
    /* Check that brake fault data was sent to the correct PDU */
    TEST_ASSERT_EQUAL_UINT8(1u, mock_com_tx_data[FZC_COM_TX_BRAKE_FAULT][2]);
}

/* ==================================================================
 * Test runner
 * ================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* SWR-FZC-019: E2E Transmit */
    RUN_TEST(test_FzcCom_e2e_protect_crc_and_alive);
    RUN_TEST(test_FzcCom_e2e_protect_null_rejected);

    /* SWR-FZC-020: E2E Receive */
    RUN_TEST(test_FzcCom_e2e_check_valid);
    RUN_TEST(test_FzcCom_e2e_check_fail_brake_safe_default_100pct);
    RUN_TEST(test_FzcCom_e2e_check_fail_steering_safe_default_center);

    /* SWR-FZC-026: CAN Message Reception */
    RUN_TEST(test_FzcCom_receive_estop_routing);
    RUN_TEST(test_FzcCom_receive_brake_cmd_routing);
    RUN_TEST(test_FzcCom_receive_steering_cmd_routing);

    /* SWR-FZC-027: CAN Message Transmission */
    RUN_TEST(test_FzcCom_transmit_heartbeat_50ms);
    RUN_TEST(test_FzcCom_transmit_event_driven_within_10ms);

    return UNITY_END();
}

/* ==================================================================
 * Source inclusion — link SWC under test directly into test binary
 * ================================================================== */

/* Prevent BSW headers from redefining types when source is included */
#define PLATFORM_TYPES_H
#define STD_TYPES_H
#define SWC_FZC_COM_H
#define FZC_CFG_H
#define RTE_H
#define COM_H

#include "../src/Swc_FzcCom.c"
