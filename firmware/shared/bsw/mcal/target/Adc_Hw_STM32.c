/**
 * @file    Adc_Hw_STM32.c
 * @brief   STM32G474 hardware backend for ADC MCAL driver
 * @date    2026-03-08
 *
 * @details Real ADC implementation using HAL ADC with DMA circular mode.
 *          ADC1 is configured by CubeMX MSP init (pin, clock, DMA channel).
 *          This driver starts conversions and reads DMA buffer results.
 *
 *          RZC pin assignments (from cuberzccfg):
 *            ADC1_IN1 (PA0) = motor current (ACS723)
 *            ADC1_IN2 (PA1) = motor temperature (NTC)
 *            ADC1_IN3 (PA2) = board temperature (NTC)
 *            ADC1_IN4 (PA3) = battery voltage (resistor divider)
 *
 *          12-bit resolution, 3.3V VREF.
 *          DMA circular mode: conversions run continuously once started,
 *          results always fresh in adc_dma_buf[].
 *
 * @safety_req SWR-BSW-007: ADC Driver for Analog Sensing
 * @copyright Taktflow Systems 2026
 */

#include "Platform_Types.h"
#include "Std_Types.h"
#include "Adc.h"
#include "stm32g4xx_hal.h"

/* ==================================================================
 * Static state
 * ================================================================== */

#define ADC_HW_MAX_CHANNELS  8u

/** DMA destination buffer — written by DMA in circular mode */
static volatile uint16 adc_dma_buf[ADC_HW_MAX_CHANNELS];

/** HAL ADC handle — initialized by CubeMX HAL_ADC_MspInit() */
static ADC_HandleTypeDef hadc1;

/** DMA handle for ADC1 — initialized by CubeMX HAL_ADC_MspInit() */
static DMA_HandleTypeDef hdma_adc1;

/** Number of channels configured for current conversion group */
static uint8 adc_num_channels = 0u;

/** Conversion status: 0=idle, 3=completed */
static uint8 adc_conv_status = 0u;

/* ==================================================================
 * Static helpers
 * ================================================================== */

/**
 * @brief  Configure ADC1 for the target ECU channels
 * @return E_OK on success, E_NOT_OK on failure
 *
 * @note   Channel selection and sequencing configured here.
 *         GPIO pin mode and ADC clock are handled by CubeMX MSP init.
 *         RZC uses 4 channels on ADC1 (PA0-PA3).
 */
static Std_ReturnType Adc_Hw_ConfigureChannels(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    /* Channel 1: PA0 = motor current */
    sConfig.Channel      = ADC_CHANNEL_1;
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_47CYCLES_5;
    sConfig.SingleDiff   = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset       = 0u;

    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        return E_NOT_OK;
    }

    /* Channel 2: PA1 = motor temperature */
    sConfig.Channel = ADC_CHANNEL_2;
    sConfig.Rank    = ADC_REGULAR_RANK_2;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        return E_NOT_OK;
    }

    /* Channel 3: PA2 = board temperature */
    sConfig.Channel = ADC_CHANNEL_3;
    sConfig.Rank    = ADC_REGULAR_RANK_3;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        return E_NOT_OK;
    }

    /* Channel 4: PA3 = battery voltage */
    sConfig.Channel = ADC_CHANNEL_4;
    sConfig.Rank    = ADC_REGULAR_RANK_4;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        return E_NOT_OK;
    }

    adc_num_channels = 4u;
    return E_OK;
}

/* ==================================================================
 * Adc_Hw_* API implementations
 * ================================================================== */

/**
 * @brief  Initialize ADC1 hardware with DMA circular mode
 * @return E_OK on success, E_NOT_OK on failure
 *
 * @note   CubeMX MSP init enables GPIOA pins (analog mode), ADC1 clock,
 *         and DMA1_Channel1. This function configures ADC parameters
 *         and channel sequencing.
 */
Std_ReturnType Adc_Hw_Init(void)
{
    uint8 i;

    for (i = 0u; i < ADC_HW_MAX_CHANNELS; i++) {
        adc_dma_buf[i] = 0u;
    }

    /* ADC1 configuration: 12-bit, scan mode, DMA circular */
    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode          = ADC_SCAN_ENABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SEQ_CONV;
    hadc1.Init.LowPowerAutoWait      = DISABLE;
    hadc1.Init.ContinuousConvMode    = ENABLE;
    hadc1.Init.NbrOfConversion       = 4u;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = ENABLE;
    hadc1.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;
    hadc1.Init.OversamplingMode      = DISABLE;

    /* Link DMA to ADC */
    hadc1.DMA_Handle = &hdma_adc1;

    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        return E_NOT_OK;
    }

    /* Run ADC calibration (single-ended) */
    if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK) {
        return E_NOT_OK;
    }

    /* Configure channel sequencing */
    if (Adc_Hw_ConfigureChannels() != E_OK) {
        return E_NOT_OK;
    }

    /* Start ADC with DMA in circular mode — conversions run continuously */
    if (HAL_ADC_Start_DMA(&hadc1, (uint32*)adc_dma_buf,
                           (uint32)adc_num_channels) != HAL_OK) {
        return E_NOT_OK;
    }

    adc_conv_status = 3u; /* ADC_COMPLETED — DMA always has fresh data */
    return E_OK;
}

/**
 * @brief  Start ADC conversion for a group (DMA circular: already running)
 * @param  Group  Group index (only group 0 supported)
 * @return E_OK on success, E_NOT_OK on invalid group
 */
Std_ReturnType Adc_Hw_StartConversion(uint8 Group)
{
    if (Group != 0u) {
        return E_NOT_OK;
    }

    /* DMA circular mode: conversions always running, data always fresh */
    adc_conv_status = 3u; /* ADC_COMPLETED */
    return E_OK;
}

/**
 * @brief  Read ADC conversion results from DMA buffer
 * @param  Group         Group index (only group 0 supported)
 * @param  ResultBuffer  Output buffer for 12-bit ADC values
 * @param  NumChannels   Number of channels to read
 * @return E_OK on success, E_NOT_OK on invalid params
 */
Std_ReturnType Adc_Hw_ReadResult(uint8 Group, uint16* ResultBuffer,
                                  uint8 NumChannels)
{
    uint8 c;

    if (Group != 0u) {
        return E_NOT_OK;
    }
    if (ResultBuffer == NULL_PTR) {
        return E_NOT_OK;
    }
    if (NumChannels > adc_num_channels) {
        NumChannels = adc_num_channels;
    }

    /* Copy from DMA buffer — volatile read ensures fresh data */
    for (c = 0u; c < NumChannels; c++) {
        ResultBuffer[c] = adc_dma_buf[c];
    }

    return E_OK;
}

/**
 * @brief  Get ADC conversion status
 * @param  Group  Group index (only group 0 supported)
 * @return 3=ADC_COMPLETED (DMA always has data), 0=idle for invalid group
 */
uint8 Adc_Hw_GetStatus(uint8 Group)
{
    if (Group != 0u) {
        return 0u;
    }
    return adc_conv_status;
}
