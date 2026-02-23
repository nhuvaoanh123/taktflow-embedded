/**
 * @file    Swc_TempMonitor.c
 * @brief   RZC temperature monitoring — NTC readout, stepped derating curve,
 *          hysteresis recovery, CAN broadcast
 * @date    2026-02-23
 *
 * @safety_req SWR-RZC-009, SWR-RZC-010, SWR-RZC-011
 * @traces_to  SSR-RZC-009, SSR-RZC-010, SSR-RZC-011
 *
 * @details  Implements the RZC temperature monitoring SWC (ASIL A):
 *
 *           SWR-RZC-009 — Temperature Measurement
 *           1. Reads motor temperature from IoHwAb (deci-degrees C).
 *              IoHwAb_ReadMotorTemp performs ADC->temperature conversion
 *              internally (Steinhart-Hart / lookup), so no ln() is needed
 *              in this SWC.
 *           2. Range-checks against plausible NTC bounds (-30..150 degC).
 *           3. Writes measured temperature to RTE.
 *
 *           SWR-RZC-010 — Derating Curve
 *           4. Computes stepped power derating from temperature:
 *              <  60 degC  -> 100%
 *              60..79 degC -> 75%
 *              80..99 degC -> 50%
 *              >= 100 degC -> 0% (shutdown, overtemp DTC)
 *
 *           SWR-RZC-011 — Hysteresis Recovery
 *           5. Derating may freely DECREASE (get worse) with rising temp.
 *              Recovery (increase) requires the temperature to drop below
 *              the next-lower threshold minus RZC_TEMP_HYSTERESIS_C (10 degC):
 *              - From  0% -> 50%: need temp < 90 degC  (100 - 10)
 *              - From 50% -> 75%: need temp < 70 degC  ( 80 - 10)
 *              - From 75% ->100%: need temp < 50 degC  ( 60 - 10)
 *
 *           6. Broadcasts [temp_hi, temp_lo, derating, alive, 0,0,0,0]
 *              on CAN via Com_SendSignal every 100ms.
 *
 *           All variables are static file-scope. No dynamic memory.
 *
 * @standard AUTOSAR SWC pattern, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */

#include "Swc_TempMonitor.h"
#include "Rzc_Cfg.h"

/* ==================================================================
 * External Dependencies
 * ================================================================== */

extern Std_ReturnType IoHwAb_ReadMotorTemp(sint16* Temp_dC);
extern Std_ReturnType Rte_Read(uint16 SignalId, uint32* DataPtr);
extern Std_ReturnType Rte_Write(uint16 SignalId, uint32 Data);
extern Std_ReturnType Com_SendSignal(uint16 SignalId,
                                     const uint8* DataPtr, uint8 Length);
extern void           Dem_ReportErrorStatus(uint8 EventId, uint8 EventStatus);

/* ==================================================================
 * Constants
 * ================================================================== */

/** CAN TX payload size (8 bytes) */
#define TM_CAN_PAYLOAD_LEN   8u

/** Alive counter maximum (4-bit, wraps at 15) */
#define TM_ALIVE_MAX         15u

/** CAN payload byte offsets */
#define TM_CAN_BYTE_TEMP_HI  0u
#define TM_CAN_BYTE_TEMP_LO  1u
#define TM_CAN_BYTE_DERATE   2u
#define TM_CAN_BYTE_ALIVE    3u

/* ==================================================================
 * Module State (static file-scope)
 * ================================================================== */

/** Module initialization flag */
static uint8   TM_Initialized;

/** Current temperature in deci-degrees Celsius (e.g. 250 = 25.0 degC) */
static sint16  TM_CurrentTemp_dC;

/** Current derating percentage: 0, 50, 75, or 100 */
static uint8   TM_DeratingPct;

/** Previous cycle derating (for hysteresis tracking) */
static uint8   TM_PrevDeratingPct;

/** Temperature sensor fault flag */
static uint8   TM_TempFault;

/** CAN alive counter (0..15) */
static uint8   TM_AliveCounter;

/* ==================================================================
 * Private: Compute raw derating from temperature (no hysteresis)
 * ================================================================== */

/**
 * @brief   Map whole-degrees temperature to raw derating percentage.
 * @param   temp_C  Temperature in whole degrees Celsius.
 * @return  Raw derating: 100, 75, 50, or 0.
 */
static uint8 TM_ComputeRawDerating(sint16 temp_C)
{
    uint8 raw;

    if (temp_C < (sint16)RZC_TEMP_DERATE_NONE_C) {
        raw = RZC_TEMP_DERATE_100_PCT;
    } else if (temp_C < (sint16)RZC_TEMP_DERATE_75_C) {
        raw = RZC_TEMP_DERATE_75_PCT;
    } else if (temp_C < (sint16)RZC_TEMP_DERATE_50_C) {
        raw = RZC_TEMP_DERATE_50_PCT;
    } else {
        raw = RZC_TEMP_DERATE_0_PCT;
    }

    return raw;
}

/* ==================================================================
 * Private: Apply hysteresis to derating recovery
 * ================================================================== */

/**
 * @brief   Apply hysteresis: derating may freely decrease (worsen) but
 *          requires temperature to drop below a lower threshold to
 *          increase (improve / recover).
 * @param   raw_derating   Derating computed from the current temperature
 *                         without hysteresis.
 * @param   cur_derating   Currently active derating percentage.
 * @param   temp_C         Current temperature in whole degrees Celsius.
 * @return  New derating percentage after hysteresis logic.
 */
static uint8 TM_ApplyHysteresis(uint8 raw_derating, uint8 cur_derating,
                                sint16 temp_C)
{
    uint8 result;

    /* Derating getting worse (value decreasing): apply immediately */
    if (raw_derating <= cur_derating) {
        result = raw_derating;
    } else {
        /* Recovery requested (raw > current). Check hysteresis threshold.
         * Only allow one step at a time. */
        result = cur_derating;

        if (cur_derating == RZC_TEMP_DERATE_0_PCT) {
            /* From 0% -> 50%: need temp < (DERATE_50_C - HYSTERESIS) = 90 */
            if (temp_C < (sint16)(RZC_TEMP_DERATE_50_C - RZC_TEMP_HYSTERESIS_C)) {
                result = RZC_TEMP_DERATE_50_PCT;
            }
        } else if (cur_derating == RZC_TEMP_DERATE_50_PCT) {
            /* From 50% -> 75%: need temp < (DERATE_75_C - HYSTERESIS) = 70 */
            if (temp_C < (sint16)(RZC_TEMP_DERATE_75_C - RZC_TEMP_HYSTERESIS_C)) {
                result = RZC_TEMP_DERATE_75_PCT;
            }
        } else if (cur_derating == RZC_TEMP_DERATE_75_PCT) {
            /* From 75% -> 100%: need temp < (DERATE_NONE_C - HYSTERESIS) = 50 */
            if (temp_C < (sint16)(RZC_TEMP_DERATE_NONE_C - RZC_TEMP_HYSTERESIS_C)) {
                result = RZC_TEMP_DERATE_100_PCT;
            }
        } else {
            /* Already at 100%, nothing to recover to */
            result = cur_derating;
        }
    }

    return result;
}

/* ==================================================================
 * API: Swc_TempMonitor_Init
 * ================================================================== */

void Swc_TempMonitor_Init(void)
{
    TM_CurrentTemp_dC  = 0;
    TM_DeratingPct     = RZC_TEMP_DERATE_100_PCT;
    TM_PrevDeratingPct = RZC_TEMP_DERATE_100_PCT;
    TM_TempFault       = FALSE;
    TM_AliveCounter    = 0u;
    TM_Initialized     = TRUE;
}

/* ==================================================================
 * API: Swc_TempMonitor_MainFunction (100ms cyclic)
 * ================================================================== */

void Swc_TempMonitor_MainFunction(void)
{
    sint16 temp_dC;
    sint16 temp_C;
    uint8  raw_derating;
    uint8  new_derating;
    uint8  tx_data[TM_CAN_PAYLOAD_LEN];
    uint16 temp_unsigned;
    uint8  i;

    /* ------------------------------------------------------------ */
    /* 1. Guard: not initialized                                     */
    /* ------------------------------------------------------------ */
    if (TM_Initialized != TRUE) {
        return;
    }

    /* ------------------------------------------------------------ */
    /* 2. Read temperature from IoHwAb                               */
    /* ------------------------------------------------------------ */
    temp_dC = 0;
    (void)IoHwAb_ReadMotorTemp(&temp_dC);

    /* ------------------------------------------------------------ */
    /* 3. Range check: plausible NTC bounds                          */
    /* ------------------------------------------------------------ */
    if ((temp_dC < (sint16)RZC_TEMP_MIN_DDC) ||
        (temp_dC > (sint16)RZC_TEMP_MAX_DDC)) {
        TM_TempFault = TRUE;
        Dem_ReportErrorStatus((uint8)RZC_DTC_OVERTEMP,
                              DEM_EVENT_STATUS_FAILED);

        /* Write fault flag to RTE and return */
        (void)Rte_Write(RZC_SIG_TEMP_FAULT, (uint32)TM_TempFault);
        return;
    }

    /* ------------------------------------------------------------ */
    /* 4. Store current temperature                                  */
    /* ------------------------------------------------------------ */
    TM_CurrentTemp_dC = temp_dC;

    /* ------------------------------------------------------------ */
    /* 5. Convert to whole degrees for derating computation          */
    /* ------------------------------------------------------------ */
    temp_C = temp_dC / (sint16)10;

    /* ------------------------------------------------------------ */
    /* 6. Derating with hysteresis                                   */
    /* ------------------------------------------------------------ */
    raw_derating = TM_ComputeRawDerating(temp_C);
    new_derating = TM_ApplyHysteresis(raw_derating, TM_DeratingPct, temp_C);

    TM_PrevDeratingPct = TM_DeratingPct;
    TM_DeratingPct     = new_derating;

    /* ------------------------------------------------------------ */
    /* 7. If derating = 0%: report overtemp DTC                     */
    /* ------------------------------------------------------------ */
    if (TM_DeratingPct == RZC_TEMP_DERATE_0_PCT) {
        Dem_ReportErrorStatus((uint8)RZC_DTC_OVERTEMP,
                              DEM_EVENT_STATUS_FAILED);
    }

    /* ------------------------------------------------------------ */
    /* 8. Write to RTE                                               */
    /* ------------------------------------------------------------ */
    (void)Rte_Write(RZC_SIG_TEMP1_DC,      (uint32)TM_CurrentTemp_dC);
    (void)Rte_Write(RZC_SIG_DERATING_PCT,  (uint32)TM_DeratingPct);
    (void)Rte_Write(RZC_SIG_TEMP_FAULT,    (uint32)TM_TempFault);

    /* ------------------------------------------------------------ */
    /* 9. CAN broadcast via Com                                      */
    /* ------------------------------------------------------------ */
    for (i = 0u; i < TM_CAN_PAYLOAD_LEN; i++) {
        tx_data[i] = 0u;
    }

    /* Temperature as unsigned 16-bit for CAN (big-endian) */
    temp_unsigned = (uint16)TM_CurrentTemp_dC;
    tx_data[TM_CAN_BYTE_TEMP_HI] = (uint8)(temp_unsigned >> 8u);
    tx_data[TM_CAN_BYTE_TEMP_LO] = (uint8)(temp_unsigned & 0xFFu);
    tx_data[TM_CAN_BYTE_DERATE]  = TM_DeratingPct;
    tx_data[TM_CAN_BYTE_ALIVE]   = TM_AliveCounter;

    (void)Com_SendSignal((uint16)RZC_COM_TX_MOTOR_TEMP,
                         tx_data, TM_CAN_PAYLOAD_LEN);

    /* ------------------------------------------------------------ */
    /* 10. Increment alive counter (wrap at 15)                      */
    /* ------------------------------------------------------------ */
    TM_AliveCounter++;
    if (TM_AliveCounter > TM_ALIVE_MAX) {
        TM_AliveCounter = 0u;
    }
}
