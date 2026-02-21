/**
 * @file    test_Spi.c
 * @brief   Unit tests for SPI MCAL driver
 * @date    2026-02-21
 *
 * @verifies SWR-BSW-006
 *
 * Tests SPI driver initialization, write/read internal buffers,
 * synchronous transmit, and defensive error handling. Hardware is
 * mocked via Spi_Hw_* stub functions defined in this file.
 */
#include "unity.h"
#include "Spi.h"

/* ==================================================================
 * Mock Hardware Layer
 * ================================================================== */

static boolean      mock_hw_init_called;
static boolean      mock_hw_init_fail;
static uint8        mock_hw_status;       /* 0=IDLE, 1=BUSY, 2=ERROR */
static boolean      mock_hw_transmit_called;
static boolean      mock_hw_transmit_fail;

/* Capture what was transmitted */
static uint16       mock_tx_data[SPI_MAX_CHANNELS][SPI_IB_SIZE];
static uint16       mock_rx_data[SPI_MAX_CHANNELS][SPI_IB_SIZE];
static uint8        mock_tx_len[SPI_MAX_CHANNELS];

/* ---- Hardware mock implementations ---- */

Std_ReturnType Spi_Hw_Init(void)
{
    mock_hw_init_called = TRUE;
    if (mock_hw_init_fail) {
        return E_NOT_OK;
    }
    return E_OK;
}

Std_ReturnType Spi_Hw_Transmit(uint8 Channel, const uint16* TxBuf, uint16* RxBuf, uint8 Length)
{
    mock_hw_transmit_called = TRUE;
    if (mock_hw_transmit_fail) {
        return E_NOT_OK;
    }
    /* Copy TX data for verification */
    for (uint8 i = 0u; i < Length; i++) {
        if (TxBuf != NULL_PTR) {
            mock_tx_data[Channel][i] = TxBuf[i];
        }
        /* Provide mock RX data */
        if (RxBuf != NULL_PTR) {
            RxBuf[i] = mock_rx_data[Channel][i];
        }
    }
    mock_tx_len[Channel] = Length;
    return E_OK;
}

uint8 Spi_Hw_GetStatus(void)
{
    return mock_hw_status;
}

/* ==================================================================
 * Test Fixtures
 * ================================================================== */

static Spi_ConfigType test_config;

void setUp(void)
{
    mock_hw_init_called = FALSE;
    mock_hw_init_fail = FALSE;
    mock_hw_status = 0u;
    mock_hw_transmit_called = FALSE;
    mock_hw_transmit_fail = FALSE;

    for (uint8 ch = 0u; ch < SPI_MAX_CHANNELS; ch++) {
        mock_tx_len[ch] = 0u;
        for (uint8 i = 0u; i < SPI_IB_SIZE; i++) {
            mock_tx_data[ch][i] = 0u;
            mock_rx_data[ch][i] = 0u;
        }
    }

    /* Default config: CPOL=0 CPHA=1, 1 MHz, 16-bit */
    test_config.clockSpeed = 1000000u;
    test_config.cpol = 0u;
    test_config.cpha = 1u;
    test_config.dataWidth = 16u;
    test_config.numChannels = 2u;
}

void tearDown(void) { }

/* ==================================================================
 * SWR-BSW-006: SPI Initialization
 * ================================================================== */

/** @verifies SWR-BSW-006 */
void test_Spi_Init_success(void)
{
    Spi_Init(&test_config);

    TEST_ASSERT_TRUE(mock_hw_init_called);
    TEST_ASSERT_EQUAL(SPI_IDLE, Spi_GetStatus());
}

/** @verifies SWR-BSW-006 */
void test_Spi_Init_null_config(void)
{
    Spi_Init(NULL_PTR);

    TEST_ASSERT_EQUAL(SPI_UNINIT, Spi_GetStatus());
}

/** @verifies SWR-BSW-006 */
void test_Spi_Init_hw_failure(void)
{
    mock_hw_init_fail = TRUE;
    Spi_Init(&test_config);

    TEST_ASSERT_EQUAL(SPI_UNINIT, Spi_GetStatus());
}

/* ==================================================================
 * SWR-BSW-006: SPI WriteIB
 * ================================================================== */

/** @verifies SWR-BSW-006 */
void test_Spi_WriteIB_success(void)
{
    Spi_Init(&test_config);

    uint16 data[] = {0x3FFFu, 0x0000u};
    Std_ReturnType ret = Spi_WriteIB(0u, data);

    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @verifies SWR-BSW-006 */
void test_Spi_WriteIB_null_data(void)
{
    Spi_Init(&test_config);

    Std_ReturnType ret = Spi_WriteIB(0u, NULL_PTR);

    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @verifies SWR-BSW-006 */
void test_Spi_WriteIB_invalid_channel(void)
{
    Spi_Init(&test_config);

    uint16 data[] = {0x1234u};
    Std_ReturnType ret = Spi_WriteIB(SPI_MAX_CHANNELS, data);

    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @verifies SWR-BSW-006 */
void test_Spi_WriteIB_before_init(void)
{
    uint16 data[] = {0x1234u};
    Std_ReturnType ret = Spi_WriteIB(0u, data);

    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* ==================================================================
 * SWR-BSW-006: SPI ReadIB
 * ================================================================== */

/** @verifies SWR-BSW-006 */
void test_Spi_ReadIB_success(void)
{
    Spi_Init(&test_config);

    /* Pre-load mock RX data */
    mock_rx_data[0u][0] = 0xABCDu;

    /* Write and transmit to populate RX buffer */
    uint16 tx[] = {0xFFFFu};
    Spi_WriteIB(0u, tx);
    Spi_SyncTransmit(0u);

    uint16 rx_buf[SPI_IB_SIZE] = {0u};
    Std_ReturnType ret = Spi_ReadIB(0u, rx_buf);

    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_HEX16(0xABCDu, rx_buf[0]);
}

/** @verifies SWR-BSW-006 */
void test_Spi_ReadIB_null_buffer(void)
{
    Spi_Init(&test_config);

    Std_ReturnType ret = Spi_ReadIB(0u, NULL_PTR);

    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @verifies SWR-BSW-006 */
void test_Spi_ReadIB_invalid_channel(void)
{
    Spi_Init(&test_config);

    uint16 rx_buf[SPI_IB_SIZE] = {0u};
    Std_ReturnType ret = Spi_ReadIB(SPI_MAX_CHANNELS, rx_buf);

    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* ==================================================================
 * SWR-BSW-006: SPI SyncTransmit
 * ================================================================== */

/** @verifies SWR-BSW-006 */
void test_Spi_SyncTransmit_success(void)
{
    Spi_Init(&test_config);

    uint16 tx[] = {0x3FFFu};
    Spi_WriteIB(0u, tx);

    Std_ReturnType ret = Spi_SyncTransmit(0u);

    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_TRUE(mock_hw_transmit_called);
    TEST_ASSERT_EQUAL_HEX16(0x3FFFu, mock_tx_data[0u][0]);
}

/** @verifies SWR-BSW-006 */
void test_Spi_SyncTransmit_hw_failure(void)
{
    Spi_Init(&test_config);

    uint16 tx[] = {0x1234u};
    Spi_WriteIB(0u, tx);
    mock_hw_transmit_fail = TRUE;

    Std_ReturnType ret = Spi_SyncTransmit(0u);

    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @verifies SWR-BSW-006 */
void test_Spi_SyncTransmit_before_init(void)
{
    Std_ReturnType ret = Spi_SyncTransmit(0u);

    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @verifies SWR-BSW-006 */
void test_Spi_SyncTransmit_invalid_sequence(void)
{
    Spi_Init(&test_config);

    Std_ReturnType ret = Spi_SyncTransmit(SPI_MAX_CHANNELS);

    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* ==================================================================
 * Test Runner
 * ================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* Init tests */
    RUN_TEST(test_Spi_Init_success);
    RUN_TEST(test_Spi_Init_null_config);
    RUN_TEST(test_Spi_Init_hw_failure);

    /* WriteIB tests */
    RUN_TEST(test_Spi_WriteIB_success);
    RUN_TEST(test_Spi_WriteIB_null_data);
    RUN_TEST(test_Spi_WriteIB_invalid_channel);
    RUN_TEST(test_Spi_WriteIB_before_init);

    /* ReadIB tests */
    RUN_TEST(test_Spi_ReadIB_success);
    RUN_TEST(test_Spi_ReadIB_null_buffer);
    RUN_TEST(test_Spi_ReadIB_invalid_channel);

    /* SyncTransmit tests */
    RUN_TEST(test_Spi_SyncTransmit_success);
    RUN_TEST(test_Spi_SyncTransmit_hw_failure);
    RUN_TEST(test_Spi_SyncTransmit_before_init);
    RUN_TEST(test_Spi_SyncTransmit_invalid_sequence);

    return UNITY_END();
}
