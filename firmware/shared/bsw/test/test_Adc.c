/**
 * @file    test_Adc.c
 * @brief   Unit tests for ADC MCAL driver
 * @date    2026-02-21
 *
 * @verifies SWR-BSW-007
 *
 * Tests ADC driver initialization, group conversion start, result
 * reading, and defensive error handling. Hardware is mocked via
 * Adc_Hw_* stub functions defined in this file.
 */
#include "unity.h"
#include "Adc.h"

/* ==================================================================
 * Mock Hardware Layer
 * ================================================================== */

static boolean      mock_hw_init_called;
static boolean      mock_hw_init_fail;
static boolean      mock_hw_start_called;
static uint8        mock_hw_start_group;
static uint8        mock_hw_status;         /* 0=IDLE, 1=BUSY, 2=COMPLETED */
static boolean      mock_hw_read_fail;

/* Mock ADC results per group per channel */
static uint16       mock_adc_results[ADC_MAX_GROUPS][ADC_MAX_CHANNELS_PER_GROUP];
static uint8        mock_result_count[ADC_MAX_GROUPS];

/* ---- Hardware mock implementations ---- */

Std_ReturnType Adc_Hw_Init(void)
{
    mock_hw_init_called = TRUE;
    if (mock_hw_init_fail) {
        return E_NOT_OK;
    }
    return E_OK;
}

Std_ReturnType Adc_Hw_StartConversion(uint8 Group)
{
    mock_hw_start_called = TRUE;
    mock_hw_start_group = Group;
    return E_OK;
}

Std_ReturnType Adc_Hw_ReadResult(uint8 Group, uint16* ResultBuffer, uint8 NumChannels)
{
    if (mock_hw_read_fail) {
        return E_NOT_OK;
    }
    for (uint8 i = 0u; i < NumChannels; i++) {
        ResultBuffer[i] = mock_adc_results[Group][i];
    }
    return E_OK;
}

uint8 Adc_Hw_GetStatus(uint8 Group)
{
    (void)Group;
    return mock_hw_status;
}

/* ==================================================================
 * Test Fixtures
 * ================================================================== */

static Adc_ConfigType test_config;
static Adc_GroupConfigType test_groups[2];

void setUp(void)
{
    mock_hw_init_called = FALSE;
    mock_hw_init_fail = FALSE;
    mock_hw_start_called = FALSE;
    mock_hw_start_group = 0xFFu;
    mock_hw_status = 0u;
    mock_hw_read_fail = FALSE;

    for (uint8 g = 0u; g < ADC_MAX_GROUPS; g++) {
        mock_result_count[g] = 0u;
        for (uint8 ch = 0u; ch < ADC_MAX_CHANNELS_PER_GROUP; ch++) {
            mock_adc_results[g][ch] = 0u;
        }
    }

    /* Group 0: current sensing (2 channels) */
    test_groups[0].numChannels = 2u;
    test_groups[0].triggerSource = 0u; /* Software trigger */

    /* Group 1: temperature (1 channel) */
    test_groups[1].numChannels = 1u;
    test_groups[1].triggerSource = 0u;

    test_config.numGroups = 2u;
    test_config.groups = test_groups;
    test_config.resolution = 12u;
}

void tearDown(void) { }

/* ==================================================================
 * SWR-BSW-007: ADC Initialization
 * ================================================================== */

/** @verifies SWR-BSW-007 */
void test_Adc_Init_success(void)
{
    Adc_Init(&test_config);

    TEST_ASSERT_TRUE(mock_hw_init_called);
    TEST_ASSERT_EQUAL(ADC_IDLE, Adc_GetStatus());
}

/** @verifies SWR-BSW-007 */
void test_Adc_Init_null_config(void)
{
    Adc_Init(NULL_PTR);

    TEST_ASSERT_EQUAL(ADC_UNINIT, Adc_GetStatus());
}

/** @verifies SWR-BSW-007 */
void test_Adc_Init_hw_failure(void)
{
    mock_hw_init_fail = TRUE;
    Adc_Init(&test_config);

    TEST_ASSERT_EQUAL(ADC_UNINIT, Adc_GetStatus());
}

/** @verifies SWR-BSW-007 */
void test_Adc_Init_null_groups(void)
{
    test_config.groups = NULL_PTR;
    Adc_Init(&test_config);

    TEST_ASSERT_EQUAL(ADC_UNINIT, Adc_GetStatus());
}

/* ==================================================================
 * SWR-BSW-007: ADC StartGroupConversion
 * ================================================================== */

/** @verifies SWR-BSW-007 */
void test_Adc_StartGroupConversion_success(void)
{
    Adc_Init(&test_config);

    Std_ReturnType ret = Adc_StartGroupConversion(0u);

    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_TRUE(mock_hw_start_called);
    TEST_ASSERT_EQUAL_UINT8(0u, mock_hw_start_group);
}

/** @verifies SWR-BSW-007 */
void test_Adc_StartGroupConversion_invalid_group(void)
{
    Adc_Init(&test_config);

    Std_ReturnType ret = Adc_StartGroupConversion(ADC_MAX_GROUPS);

    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @verifies SWR-BSW-007 */
void test_Adc_StartGroupConversion_before_init(void)
{
    Std_ReturnType ret = Adc_StartGroupConversion(0u);

    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* ==================================================================
 * SWR-BSW-007: ADC ReadGroup
 * ================================================================== */

/** @verifies SWR-BSW-007 */
void test_Adc_ReadGroup_success(void)
{
    Adc_Init(&test_config);

    /* Set mock results: 12-bit ADC values */
    mock_adc_results[0u][0] = 2048u;
    mock_adc_results[0u][1] = 4095u;
    mock_hw_status = 2u; /* COMPLETED */

    Adc_StartGroupConversion(0u);

    uint16 results[ADC_MAX_CHANNELS_PER_GROUP] = {0u};
    Std_ReturnType ret = Adc_ReadGroup(0u, results);

    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT16(2048u, results[0]);
    TEST_ASSERT_EQUAL_UINT16(4095u, results[1]);
}

/** @verifies SWR-BSW-007 */
void test_Adc_ReadGroup_null_buffer(void)
{
    Adc_Init(&test_config);
    Adc_StartGroupConversion(0u);

    Std_ReturnType ret = Adc_ReadGroup(0u, NULL_PTR);

    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @verifies SWR-BSW-007 */
void test_Adc_ReadGroup_invalid_group(void)
{
    Adc_Init(&test_config);

    uint16 results[ADC_MAX_CHANNELS_PER_GROUP] = {0u};
    Std_ReturnType ret = Adc_ReadGroup(ADC_MAX_GROUPS, results);

    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @verifies SWR-BSW-007 */
void test_Adc_ReadGroup_before_init(void)
{
    uint16 results[ADC_MAX_CHANNELS_PER_GROUP] = {0u};
    Std_ReturnType ret = Adc_ReadGroup(0u, results);

    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @verifies SWR-BSW-007 */
void test_Adc_ReadGroup_hw_read_failure(void)
{
    Adc_Init(&test_config);
    Adc_StartGroupConversion(0u);
    mock_hw_read_fail = TRUE;

    uint16 results[ADC_MAX_CHANNELS_PER_GROUP] = {0u};
    Std_ReturnType ret = Adc_ReadGroup(0u, results);

    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/* ==================================================================
 * SWR-BSW-007: ADC DeInit
 * ================================================================== */

/** @verifies SWR-BSW-007 */
void test_Adc_DeInit(void)
{
    Adc_Init(&test_config);
    TEST_ASSERT_EQUAL(ADC_IDLE, Adc_GetStatus());

    Adc_DeInit();
    TEST_ASSERT_EQUAL(ADC_UNINIT, Adc_GetStatus());
}

/* ==================================================================
 * Test Runner
 * ================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* Init tests */
    RUN_TEST(test_Adc_Init_success);
    RUN_TEST(test_Adc_Init_null_config);
    RUN_TEST(test_Adc_Init_hw_failure);
    RUN_TEST(test_Adc_Init_null_groups);

    /* StartGroupConversion tests */
    RUN_TEST(test_Adc_StartGroupConversion_success);
    RUN_TEST(test_Adc_StartGroupConversion_invalid_group);
    RUN_TEST(test_Adc_StartGroupConversion_before_init);

    /* ReadGroup tests */
    RUN_TEST(test_Adc_ReadGroup_success);
    RUN_TEST(test_Adc_ReadGroup_null_buffer);
    RUN_TEST(test_Adc_ReadGroup_invalid_group);
    RUN_TEST(test_Adc_ReadGroup_before_init);
    RUN_TEST(test_Adc_ReadGroup_hw_read_failure);

    /* DeInit test */
    RUN_TEST(test_Adc_DeInit);

    return UNITY_END();
}
