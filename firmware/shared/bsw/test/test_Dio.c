/**
 * @file    test_Dio.c
 * @brief   Unit tests for DIO MCAL driver
 * @date    2026-02-21
 *
 * @verifies SWR-BSW-009
 *
 * Tests DIO driver read, write, flip channel operations and
 * defensive error handling. Hardware is mocked via Dio_Hw_*
 * stub functions defined in this file.
 */
#include "unity.h"
#include "Dio.h"

/* ==================================================================
 * Mock Hardware Layer
 * ================================================================== */

/** Simulated GPIO pin states (one per channel) */
static uint8 mock_pin_state[DIO_MAX_CHANNELS];

/** Track write calls */
static boolean mock_write_called[DIO_MAX_CHANNELS];
static uint8   mock_write_level[DIO_MAX_CHANNELS];

/* ---- Hardware mock implementations ---- */

uint8 Dio_Hw_ReadPin(uint8 ChannelId)
{
    if (ChannelId >= DIO_MAX_CHANNELS) {
        return STD_LOW;
    }
    return mock_pin_state[ChannelId];
}

void Dio_Hw_WritePin(uint8 ChannelId, uint8 Level)
{
    if (ChannelId >= DIO_MAX_CHANNELS) {
        return;
    }
    mock_write_called[ChannelId] = TRUE;
    mock_write_level[ChannelId] = Level;
    mock_pin_state[ChannelId] = Level;
}

/* ==================================================================
 * Test Fixtures
 * ================================================================== */

void setUp(void)
{
    for (uint8 i = 0u; i < DIO_MAX_CHANNELS; i++) {
        mock_pin_state[i] = STD_LOW;
        mock_write_called[i] = FALSE;
        mock_write_level[i] = STD_LOW;
    }

    /* Re-initialize DIO driver for each test */
    Dio_Init();
}

void tearDown(void) { }

/* ==================================================================
 * SWR-BSW-009: DIO ReadChannel
 * ================================================================== */

/** @verifies SWR-BSW-009 */
void test_Dio_ReadChannel_low(void)
{
    mock_pin_state[0] = STD_LOW;

    uint8 level = Dio_ReadChannel(0u);

    TEST_ASSERT_EQUAL_UINT8(STD_LOW, level);
}

/** @verifies SWR-BSW-009 */
void test_Dio_ReadChannel_high(void)
{
    mock_pin_state[0] = STD_HIGH;

    uint8 level = Dio_ReadChannel(0u);

    TEST_ASSERT_EQUAL_UINT8(STD_HIGH, level);
}

/** @verifies SWR-BSW-009 */
void test_Dio_ReadChannel_multiple_channels(void)
{
    mock_pin_state[0] = STD_HIGH;
    mock_pin_state[1] = STD_LOW;
    mock_pin_state[2] = STD_HIGH;

    TEST_ASSERT_EQUAL_UINT8(STD_HIGH, Dio_ReadChannel(0u));
    TEST_ASSERT_EQUAL_UINT8(STD_LOW,  Dio_ReadChannel(1u));
    TEST_ASSERT_EQUAL_UINT8(STD_HIGH, Dio_ReadChannel(2u));
}

/** @verifies SWR-BSW-009 */
void test_Dio_ReadChannel_invalid_returns_low(void)
{
    uint8 level = Dio_ReadChannel(DIO_MAX_CHANNELS);

    TEST_ASSERT_EQUAL_UINT8(STD_LOW, level);
}

/* ==================================================================
 * SWR-BSW-009: DIO WriteChannel
 * ================================================================== */

/** @verifies SWR-BSW-009 */
void test_Dio_WriteChannel_high(void)
{
    Dio_WriteChannel(0u, STD_HIGH);

    TEST_ASSERT_TRUE(mock_write_called[0]);
    TEST_ASSERT_EQUAL_UINT8(STD_HIGH, mock_write_level[0]);
}

/** @verifies SWR-BSW-009 */
void test_Dio_WriteChannel_low(void)
{
    mock_pin_state[0] = STD_HIGH;

    Dio_WriteChannel(0u, STD_LOW);

    TEST_ASSERT_TRUE(mock_write_called[0]);
    TEST_ASSERT_EQUAL_UINT8(STD_LOW, mock_write_level[0]);
}

/** @verifies SWR-BSW-009 */
void test_Dio_WriteChannel_invalid_channel(void)
{
    Dio_WriteChannel(DIO_MAX_CHANNELS, STD_HIGH);

    /* No mock should have been called */
    for (uint8 i = 0u; i < DIO_MAX_CHANNELS; i++) {
        TEST_ASSERT_FALSE(mock_write_called[i]);
    }
}

/** @verifies SWR-BSW-009 */
void test_Dio_WriteChannel_invalid_level_ignored(void)
{
    /* Level value 0x42 is invalid — shall be treated as STD_LOW */
    Dio_WriteChannel(0u, 0x42u);

    TEST_ASSERT_TRUE(mock_write_called[0]);
    TEST_ASSERT_EQUAL_UINT8(STD_LOW, mock_write_level[0]);
}

/* ==================================================================
 * SWR-BSW-009: DIO FlipChannel
 * ================================================================== */

/** @verifies SWR-BSW-009 */
void test_Dio_FlipChannel_low_to_high(void)
{
    mock_pin_state[0] = STD_LOW;

    uint8 result = Dio_FlipChannel(0u);

    TEST_ASSERT_EQUAL_UINT8(STD_HIGH, result);
    TEST_ASSERT_EQUAL_UINT8(STD_HIGH, mock_write_level[0]);
}

/** @verifies SWR-BSW-009 */
void test_Dio_FlipChannel_high_to_low(void)
{
    mock_pin_state[0] = STD_HIGH;

    uint8 result = Dio_FlipChannel(0u);

    TEST_ASSERT_EQUAL_UINT8(STD_LOW, result);
    TEST_ASSERT_EQUAL_UINT8(STD_LOW, mock_write_level[0]);
}

/** @verifies SWR-BSW-009 */
void test_Dio_FlipChannel_invalid_returns_low(void)
{
    uint8 result = Dio_FlipChannel(DIO_MAX_CHANNELS);

    TEST_ASSERT_EQUAL_UINT8(STD_LOW, result);
}

/** @verifies SWR-BSW-009 */
void test_Dio_FlipChannel_double_flip_restores(void)
{
    mock_pin_state[1] = STD_LOW;

    Dio_FlipChannel(1u);
    TEST_ASSERT_EQUAL_UINT8(STD_HIGH, mock_pin_state[1]);

    Dio_FlipChannel(1u);
    TEST_ASSERT_EQUAL_UINT8(STD_LOW, mock_pin_state[1]);
}

/* ==================================================================
 * Test Runner
 * ================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* ReadChannel tests */
    RUN_TEST(test_Dio_ReadChannel_low);
    RUN_TEST(test_Dio_ReadChannel_high);
    RUN_TEST(test_Dio_ReadChannel_multiple_channels);
    RUN_TEST(test_Dio_ReadChannel_invalid_returns_low);

    /* WriteChannel tests */
    RUN_TEST(test_Dio_WriteChannel_high);
    RUN_TEST(test_Dio_WriteChannel_low);
    RUN_TEST(test_Dio_WriteChannel_invalid_channel);
    RUN_TEST(test_Dio_WriteChannel_invalid_level_ignored);

    /* FlipChannel tests */
    RUN_TEST(test_Dio_FlipChannel_low_to_high);
    RUN_TEST(test_Dio_FlipChannel_high_to_low);
    RUN_TEST(test_Dio_FlipChannel_invalid_returns_low);
    RUN_TEST(test_Dio_FlipChannel_double_flip_restores);

    return UNITY_END();
}
