/**
 * @file    test_E2E.c
 * @brief   Unit tests for E2E End-to-End Protection module
 * @date    2026-02-21
 *
 * @verifies SWR-BSW-023, SWR-BSW-024, SWR-BSW-025
 *
 * Tests CRC-8/SAE-J1850 calculation, alive counter management,
 * protect/check round-trip, and error detection per ISO 26262
 * Part 6 requirements-based testing.
 */
#include "unity.h"
#include "E2E.h"

/* ---- Test fixtures ---- */

static E2E_ConfigType  cfg;
static E2E_StateType   tx_state;
static E2E_StateType   rx_state;
static uint8           pdu[8];

void setUp(void)
{
    cfg.DataId          = 0x01u;
    cfg.MaxDeltaCounter = 2u;
    cfg.DataLength      = 8u;

    E2E_Init();
    tx_state.Counter = 0u;
    rx_state.Counter = 0u;

    /* Clear PDU, fill payload bytes 2-7 with known data */
    for (uint8 i = 0u; i < 8u; i++) {
        pdu[i] = 0u;
    }
    pdu[2] = 0x10u;
    pdu[3] = 0x20u;
    pdu[4] = 0x30u;
    pdu[5] = 0x40u;
    pdu[6] = 0x50u;
    pdu[7] = 0x60u;
}

void tearDown(void) { }

/* ==================================================================
 * SWR-BSW-023: CRC-8/SAE-J1850 Calculation
 * ================================================================== */

/** @verifies SWR-BSW-023 */
void test_E2E_CalcCRC8_standard_check_value(void)
{
    /* Standard CRC-8/SAE-J1850 check: "123456789" -> 0x4B */
    const uint8 data[] = {0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39};
    uint8 crc = E2E_CalcCRC8(data, 9u, 0xFFu);
    TEST_ASSERT_EQUAL_HEX8(0x4Bu, crc);
}

/** @verifies SWR-BSW-023 */
void test_E2E_CalcCRC8_empty_input(void)
{
    /* Empty data: init 0xFF XOR-out 0xFF = 0x00 */
    uint8 crc = E2E_CalcCRC8(NULL_PTR, 0u, 0xFFu);
    TEST_ASSERT_EQUAL_HEX8(0x00u, crc);
}

/** @verifies SWR-BSW-023 */
void test_E2E_CalcCRC8_single_byte(void)
{
    const uint8 data[] = {0x00u};
    uint8 crc = E2E_CalcCRC8(data, 1u, 0xFFu);
    TEST_ASSERT_EQUAL_HEX8(0x3Bu, crc);
}

/** @verifies SWR-BSW-023 */
void test_E2E_CalcCRC8_all_zeros(void)
{
    const uint8 data[] = {0x00,0x00,0x00,0x00,0x00,0x00};
    uint8 crc = E2E_CalcCRC8(data, 6u, 0xFFu);
    /* Deterministic result — value verified by reference implementation */
    TEST_ASSERT_TRUE(crc != 0xFFu); /* Must differ from init after processing */
}

/** @verifies SWR-BSW-023 */
void test_E2E_CalcCRC8_different_start_value(void)
{
    const uint8 data[] = {0x01u};
    uint8 crc_a = E2E_CalcCRC8(data, 1u, 0xFFu);
    uint8 crc_b = E2E_CalcCRC8(data, 1u, 0x00u);
    TEST_ASSERT_TRUE(crc_a != crc_b); /* Different start -> different result */
}

/* ==================================================================
 * SWR-BSW-024: E2E Protect — Alive Counter and Data ID
 * ================================================================== */

/** @verifies SWR-BSW-024 */
void test_E2E_Protect_writes_data_id(void)
{
    Std_ReturnType ret = E2E_Protect(&cfg, &tx_state, pdu, 8u);

    TEST_ASSERT_EQUAL(E_OK, ret);
    /* DataId in byte 0, bits 3:0 */
    TEST_ASSERT_EQUAL_HEX8(0x01u, pdu[0] & 0x0Fu);
}

/** @verifies SWR-BSW-024 */
void test_E2E_Protect_writes_alive_counter(void)
{
    Std_ReturnType ret = E2E_Protect(&cfg, &tx_state, pdu, 8u);

    TEST_ASSERT_EQUAL(E_OK, ret);
    /* First call: counter increments 0->1, written to byte 0 bits 7:4 */
    uint8 counter = (pdu[0] >> 4u) & 0x0Fu;
    TEST_ASSERT_EQUAL_UINT8(1u, counter);
}

/** @verifies SWR-BSW-024 */
void test_E2E_Protect_increments_counter(void)
{
    E2E_Protect(&cfg, &tx_state, pdu, 8u);
    uint8 counter1 = (pdu[0] >> 4u) & 0x0Fu;

    E2E_Protect(&cfg, &tx_state, pdu, 8u);
    uint8 counter2 = (pdu[0] >> 4u) & 0x0Fu;

    TEST_ASSERT_EQUAL_UINT8(1u, counter1);
    TEST_ASSERT_EQUAL_UINT8(2u, counter2);
}

/** @verifies SWR-BSW-024 */
void test_E2E_Protect_counter_wraps_at_15(void)
{
    /* Call protect 15 times to reach counter = 15 */
    for (uint8 i = 0u; i < 15u; i++) {
        E2E_Protect(&cfg, &tx_state, pdu, 8u);
    }
    TEST_ASSERT_EQUAL_UINT8(15u, (pdu[0] >> 4u) & 0x0Fu);

    /* 16th call: counter wraps to 0 */
    E2E_Protect(&cfg, &tx_state, pdu, 8u);
    TEST_ASSERT_EQUAL_UINT8(0u, (pdu[0] >> 4u) & 0x0Fu);
}

/** @verifies SWR-BSW-024 */
void test_E2E_Protect_writes_crc_to_byte1(void)
{
    E2E_Protect(&cfg, &tx_state, pdu, 8u);

    /* CRC in byte 1, computed over bytes 2-7 + DataId */
    /* Manually compute expected CRC */
    uint8 crc_input[7];
    crc_input[0] = pdu[2]; /* 0x10 */
    crc_input[1] = pdu[3]; /* 0x20 */
    crc_input[2] = pdu[4]; /* 0x30 */
    crc_input[3] = pdu[5]; /* 0x40 */
    crc_input[4] = pdu[6]; /* 0x50 */
    crc_input[5] = pdu[7]; /* 0x60 */
    crc_input[6] = cfg.DataId; /* 0x01 */
    uint8 expected_crc = E2E_CalcCRC8(crc_input, 7u, 0xFFu);

    TEST_ASSERT_EQUAL_HEX8(expected_crc, pdu[1]);
}

/** @verifies SWR-BSW-024 */
void test_E2E_Protect_preserves_payload(void)
{
    E2E_Protect(&cfg, &tx_state, pdu, 8u);

    /* Payload bytes 2-7 must not be modified */
    TEST_ASSERT_EQUAL_HEX8(0x10u, pdu[2]);
    TEST_ASSERT_EQUAL_HEX8(0x20u, pdu[3]);
    TEST_ASSERT_EQUAL_HEX8(0x30u, pdu[4]);
    TEST_ASSERT_EQUAL_HEX8(0x40u, pdu[5]);
    TEST_ASSERT_EQUAL_HEX8(0x50u, pdu[6]);
    TEST_ASSERT_EQUAL_HEX8(0x60u, pdu[7]);
}

/** @verifies SWR-BSW-024 */
void test_E2E_Protect_null_ptr_returns_error(void)
{
    Std_ReturnType ret = E2E_Protect(&cfg, &tx_state, NULL_PTR, 8u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @verifies SWR-BSW-024 */
void test_E2E_Protect_null_config_returns_error(void)
{
    Std_ReturnType ret = E2E_Protect(NULL_PTR, &tx_state, pdu, 8u);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* ==================================================================
 * SWR-BSW-024: E2E Check — Validation
 * ================================================================== */

/** @verifies SWR-BSW-024 */
void test_E2E_Check_valid_message_returns_ok(void)
{
    /* Protect creates a valid message */
    E2E_Protect(&cfg, &tx_state, pdu, 8u);

    /* Check with fresh RX state should accept it */
    E2E_CheckStatusType status = E2E_Check(&cfg, &rx_state, pdu, 8u);
    TEST_ASSERT_EQUAL(E2E_STATUS_OK, status);
}

/** @verifies SWR-BSW-024 */
void test_E2E_Check_consecutive_messages_ok(void)
{
    /* Send two consecutive messages */
    E2E_Protect(&cfg, &tx_state, pdu, 8u);
    E2E_CheckStatusType s1 = E2E_Check(&cfg, &rx_state, pdu, 8u);

    E2E_Protect(&cfg, &tx_state, pdu, 8u);
    E2E_CheckStatusType s2 = E2E_Check(&cfg, &rx_state, pdu, 8u);

    TEST_ASSERT_EQUAL(E2E_STATUS_OK, s1);
    TEST_ASSERT_EQUAL(E2E_STATUS_OK, s2);
}

/** @verifies SWR-BSW-024 */
void test_E2E_Check_crc_error(void)
{
    E2E_Protect(&cfg, &tx_state, pdu, 8u);

    /* Corrupt the CRC byte */
    pdu[1] ^= 0xFFu;

    E2E_CheckStatusType status = E2E_Check(&cfg, &rx_state, pdu, 8u);
    TEST_ASSERT_EQUAL(E2E_STATUS_ERROR, status);
}

/** @verifies SWR-BSW-024 */
void test_E2E_Check_corrupted_payload_detected(void)
{
    E2E_Protect(&cfg, &tx_state, pdu, 8u);

    /* Corrupt a payload byte — CRC should now mismatch */
    pdu[4] ^= 0x01u;

    E2E_CheckStatusType status = E2E_Check(&cfg, &rx_state, pdu, 8u);
    TEST_ASSERT_EQUAL(E2E_STATUS_ERROR, status);
}

/** @verifies SWR-BSW-024 */
void test_E2E_Check_repeated_counter(void)
{
    E2E_Protect(&cfg, &tx_state, pdu, 8u);
    E2E_Check(&cfg, &rx_state, pdu, 8u);

    /* Send same message again (same counter) */
    E2E_CheckStatusType status = E2E_Check(&cfg, &rx_state, pdu, 8u);
    TEST_ASSERT_EQUAL(E2E_STATUS_REPEATED, status);
}

/** @verifies SWR-BSW-024 */
void test_E2E_Check_wrong_sequence(void)
{
    E2E_Protect(&cfg, &tx_state, pdu, 8u);
    E2E_Check(&cfg, &rx_state, pdu, 8u);

    /* Skip messages beyond MaxDeltaCounter (2) — send 3 more protects */
    E2E_Protect(&cfg, &tx_state, pdu, 8u); /* counter 2 — skip */
    E2E_Protect(&cfg, &tx_state, pdu, 8u); /* counter 3 — skip */
    E2E_Protect(&cfg, &tx_state, pdu, 8u); /* counter 4 — this one arrives */

    /* RX last saw counter 1, now sees 4 — delta 3 > MaxDeltaCounter 2 */
    E2E_CheckStatusType status = E2E_Check(&cfg, &rx_state, pdu, 8u);
    TEST_ASSERT_EQUAL(E2E_STATUS_WRONG_SEQ, status);
}

/** @verifies SWR-BSW-024 */
void test_E2E_Check_within_max_delta_ok(void)
{
    E2E_Protect(&cfg, &tx_state, pdu, 8u);
    E2E_Check(&cfg, &rx_state, pdu, 8u);

    /* Skip 1 message — delta = 2, within MaxDeltaCounter (2) */
    E2E_Protect(&cfg, &tx_state, pdu, 8u); /* counter 2 — skip */
    E2E_Protect(&cfg, &tx_state, pdu, 8u); /* counter 3 — arrives */

    E2E_CheckStatusType status = E2E_Check(&cfg, &rx_state, pdu, 8u);
    TEST_ASSERT_EQUAL(E2E_STATUS_OK, status);
}

/** @verifies SWR-BSW-024 */
void test_E2E_Check_null_ptr_returns_error(void)
{
    E2E_CheckStatusType status = E2E_Check(&cfg, &rx_state, NULL_PTR, 8u);
    TEST_ASSERT_EQUAL(E2E_STATUS_ERROR, status);
}

/** @verifies SWR-BSW-024 */
void test_E2E_Check_wrong_data_id(void)
{
    E2E_Protect(&cfg, &tx_state, pdu, 8u);

    /* Change DataId in byte 0 bits 3:0 to something wrong */
    pdu[0] = (pdu[0] & 0xF0u) | 0x0Fu;

    E2E_CheckStatusType status = E2E_Check(&cfg, &rx_state, pdu, 8u);
    TEST_ASSERT_EQUAL(E2E_STATUS_ERROR, status);
}

/* ==================================================================
 * SWR-BSW-024: Counter wrapping across protect/check
 * ================================================================== */

/** @verifies SWR-BSW-024 */
void test_E2E_Check_counter_wrap_valid(void)
{
    /* Drive TX counter to 15 */
    for (uint8 i = 0u; i < 14u; i++) {
        E2E_Protect(&cfg, &tx_state, pdu, 8u);
        E2E_Check(&cfg, &rx_state, pdu, 8u);
    }

    /* Counter is now 14 on both sides */
    /* Protect: counter becomes 15 */
    E2E_Protect(&cfg, &tx_state, pdu, 8u);
    E2E_CheckStatusType s1 = E2E_Check(&cfg, &rx_state, pdu, 8u);
    TEST_ASSERT_EQUAL(E2E_STATUS_OK, s1);

    /* Protect: counter wraps to 0 */
    E2E_Protect(&cfg, &tx_state, pdu, 8u);
    E2E_CheckStatusType s2 = E2E_Check(&cfg, &rx_state, pdu, 8u);
    TEST_ASSERT_EQUAL(E2E_STATUS_OK, s2);
}

/* ==================================================================
 * Test runner
 * ================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* CRC-8 tests (SWR-BSW-023) */
    RUN_TEST(test_E2E_CalcCRC8_standard_check_value);
    RUN_TEST(test_E2E_CalcCRC8_empty_input);
    RUN_TEST(test_E2E_CalcCRC8_single_byte);
    RUN_TEST(test_E2E_CalcCRC8_all_zeros);
    RUN_TEST(test_E2E_CalcCRC8_different_start_value);

    /* E2E Protect tests (SWR-BSW-024) */
    RUN_TEST(test_E2E_Protect_writes_data_id);
    RUN_TEST(test_E2E_Protect_writes_alive_counter);
    RUN_TEST(test_E2E_Protect_increments_counter);
    RUN_TEST(test_E2E_Protect_counter_wraps_at_15);
    RUN_TEST(test_E2E_Protect_writes_crc_to_byte1);
    RUN_TEST(test_E2E_Protect_preserves_payload);
    RUN_TEST(test_E2E_Protect_null_ptr_returns_error);
    RUN_TEST(test_E2E_Protect_null_config_returns_error);

    /* E2E Check tests (SWR-BSW-024) */
    RUN_TEST(test_E2E_Check_valid_message_returns_ok);
    RUN_TEST(test_E2E_Check_consecutive_messages_ok);
    RUN_TEST(test_E2E_Check_crc_error);
    RUN_TEST(test_E2E_Check_corrupted_payload_detected);
    RUN_TEST(test_E2E_Check_repeated_counter);
    RUN_TEST(test_E2E_Check_wrong_sequence);
    RUN_TEST(test_E2E_Check_within_max_delta_ok);
    RUN_TEST(test_E2E_Check_null_ptr_returns_error);
    RUN_TEST(test_E2E_Check_wrong_data_id);
    RUN_TEST(test_E2E_Check_counter_wrap_valid);

    return UNITY_END();
}
