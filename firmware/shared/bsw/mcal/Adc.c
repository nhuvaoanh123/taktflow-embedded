/**
 * @file    Adc.c
 * @brief   ADC MCAL driver implementation
 * @date    2026-02-21
 *
 * @details Platform-independent ADC driver logic. Hardware access is
 *          abstracted through Adc_Hw_* functions (implemented per platform).
 *          Supports 12-bit right-aligned conversion, group-based operation.
 *
 * @safety_req SWR-BSW-007
 * @traces_to  SYS-049, TSR-006, TSR-008
 *
 * @standard AUTOSAR_SWS_ADCDriver, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#include "Adc.h"

/* ---- Internal State ---- */

static Adc_StatusType adc_status = ADC_UNINIT;
static uint8          adc_num_groups = 0u;

/** Stored group configuration (pointer to caller-provided config) */
static const Adc_GroupConfigType* adc_groups = NULL_PTR;

/* ---- API Implementation ---- */

void Adc_Init(const Adc_ConfigType* ConfigPtr)
{
    if (ConfigPtr == NULL_PTR) {
        adc_status = ADC_UNINIT;
        return;
    }

    if (ConfigPtr->groups == NULL_PTR) {
        adc_status = ADC_UNINIT;
        return;
    }

    if (ConfigPtr->numGroups > ADC_MAX_GROUPS) {
        adc_status = ADC_UNINIT;
        return;
    }

    if (Adc_Hw_Init() != E_OK) {
        adc_status = ADC_UNINIT;
        return;
    }

    adc_num_groups = ConfigPtr->numGroups;
    adc_groups = ConfigPtr->groups;
    adc_status = ADC_IDLE;
}

void Adc_DeInit(void)
{
    adc_status = ADC_UNINIT;
    adc_num_groups = 0u;
    adc_groups = NULL_PTR;
}

Adc_StatusType Adc_GetStatus(void)
{
    return adc_status;
}

Std_ReturnType Adc_StartGroupConversion(uint8 Group)
{
    if (adc_status == ADC_UNINIT) {
        return E_NOT_OK;
    }

    if (Group >= adc_num_groups) {
        return E_NOT_OK;
    }

    adc_status = ADC_BUSY;

    Std_ReturnType ret = Adc_Hw_StartConversion(Group);
    if (ret != E_OK) {
        adc_status = ADC_IDLE;
        return E_NOT_OK;
    }

    return E_OK;
}

Std_ReturnType Adc_ReadGroup(uint8 Group, uint16* DataBufferPtr)
{
    if (adc_status == ADC_UNINIT) {
        return E_NOT_OK;
    }

    if (Group >= adc_num_groups) {
        return E_NOT_OK;
    }

    if (DataBufferPtr == NULL_PTR) {
        return E_NOT_OK;
    }

    if (adc_groups == NULL_PTR) {
        return E_NOT_OK;
    }

    uint8 numCh = adc_groups[Group].numChannels;
    if (numCh > ADC_MAX_CHANNELS_PER_GROUP) {
        numCh = ADC_MAX_CHANNELS_PER_GROUP;
    }

    Std_ReturnType ret = Adc_Hw_ReadResult(Group, DataBufferPtr, numCh);
    if (ret != E_OK) {
        return E_NOT_OK;
    }

    adc_status = ADC_IDLE;
    return E_OK;
}
