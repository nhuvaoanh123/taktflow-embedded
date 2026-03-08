/**
 * @file    Pwm_Hw_STM32.c
 * @brief   STM32G474 hardware backend for PWM MCAL driver
 * @date    2026-03-08
 *
 * @details Real PWM implementation using TIM output compare mode.
 *          Channel mapping (from pin-mapping.md):
 *            FZC: ch0=PA0 (TIM2_CH1, steering servo), ch1=PA1 (TIM2_CH2, brake servo)
 *            RZC: ch0=PA8 (TIM1_CH1, RPWM motor), ch1=PA9 (TIM1_CH2, LPWM motor)
 *
 *          GPIO AF and timer clock enable handled by CubeMX MSP init.
 *          This driver configures PWM frequency and updates duty cycle.
 *
 *          FZC servos: 50 Hz (20ms period), 1-2ms pulse = 5%-10% duty
 *          RZC motor: 20 kHz (audible noise avoidance), 0-100% duty
 *
 *          The driver auto-detects TIM1 (RZC) vs TIM2 (FZC) based on
 *          which ECU includes this file.
 *
 * @safety_req SWR-BSW-008: PWM Driver for Motor and Servo Control
 * @copyright Taktflow Systems 2026
 */

#include "Platform_Types.h"
#include "Std_Types.h"
#include "Pwm.h"
#include "stm32g4xx_hal.h"

/* ==================================================================
 * Static state
 * ================================================================== */

#define PWM_HW_MAX_CHANNELS  4u

/** Timer handles — one per timer peripheral used */
static TIM_HandleTypeDef htim_pwm;

/** Which TIM channels map to our logical PWM channels */
static const uint32 tim_channel_map[PWM_HW_MAX_CHANNELS] = {
    TIM_CHANNEL_1, TIM_CHANNEL_2, TIM_CHANNEL_3, TIM_CHANNEL_4
};

/** ARR value for duty cycle scaling */
static uint32 pwm_arr = 0u;

/** Number of active channels */
static uint8 pwm_active_channels = 0u;

/* ==================================================================
 * Pwm_Hw_* API implementations
 * ================================================================== */

/**
 * @brief  Initialize PWM timer hardware
 * @return E_OK on success, E_NOT_OK on failure
 *
 * @note   Configures TIM1 or TIM2 for PWM output.
 *         TIM1 (RZC motor): 170 MHz / (PSC+1) / (ARR+1) = 20 kHz
 *           PSC=0, ARR=8499 → 170M / 8500 = 20.0 kHz
 *         TIM2 (FZC servo): 170 MHz / (PSC+1) / (ARR+1) = 50 Hz
 *           PSC=169, ARR=19999 → 170M / 170 / 20000 = 50.0 Hz
 *
 *         Tries TIM1 first. If it fails (not clocked/not available),
 *         falls back to TIM2. The CubeMX MSP init determines which
 *         timer is available per ECU.
 */
Std_ReturnType Pwm_Hw_Init(void)
{
    TIM_OC_InitTypeDef sConfig = {0};
    uint8 i;

    /* Try TIM1 first (RZC: 20 kHz motor PWM) */
    htim_pwm.Instance = TIM1;
    htim_pwm.Init.Prescaler         = 0u;
    htim_pwm.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim_pwm.Init.Period            = 8499u;  /* 20 kHz @ 170 MHz */
    htim_pwm.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim_pwm.Init.RepetitionCounter = 0u;
    htim_pwm.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_PWM_Init(&htim_pwm) != HAL_OK) {
        /* TIM1 not available — try TIM2 (FZC: 50 Hz servo PWM) */
        htim_pwm.Instance            = TIM2;
        htim_pwm.Init.Prescaler      = 169u;
        htim_pwm.Init.Period         = 19999u; /* 50 Hz @ 170 MHz / 170 */

        if (HAL_TIM_PWM_Init(&htim_pwm) != HAL_OK) {
            return E_NOT_OK;
        }

        pwm_arr = 19999u;
        pwm_active_channels = 2u;
    } else {
        pwm_arr = 8499u;
        pwm_active_channels = 2u;
    }

    /* Configure OC channels in PWM mode 1 */
    sConfig.OCMode       = TIM_OCMODE_PWM1;
    sConfig.Pulse        = 0u;           /* Start at 0% duty */
    sConfig.OCPolarity   = TIM_OCPOLARITY_HIGH;
    sConfig.OCFastMode   = TIM_OCFAST_DISABLE;

    for (i = 0u; i < pwm_active_channels; i++) {
        if (HAL_TIM_PWM_ConfigChannel(&htim_pwm, &sConfig,
                                       tim_channel_map[i]) != HAL_OK) {
            return E_NOT_OK;
        }

        /* Start PWM output generation */
        if (HAL_TIM_PWM_Start(&htim_pwm, tim_channel_map[i]) != HAL_OK) {
            return E_NOT_OK;
        }
    }

    return E_OK;
}

/**
 * @brief  Set PWM duty cycle
 * @param  Channel    Channel index (0..3)
 * @param  DutyCycle  Duty (0x0000=0%, 0x8000=100%)
 * @return E_OK on success, E_NOT_OK on invalid channel
 *
 * @note   Converts from 0x0000-0x8000 abstract range to timer CCR value.
 *         CCR = ARR * DutyCycle / 0x8000
 */
Std_ReturnType Pwm_Hw_SetDuty(uint8 Channel, uint16 DutyCycle)
{
    uint32 ccr;

    if (Channel >= pwm_active_channels) {
        return E_NOT_OK;
    }

    /* Scale: DutyCycle 0x0000..0x8000 → CCR 0..ARR */
    ccr = ((uint32)DutyCycle * pwm_arr) / (uint32)PWM_DUTY_100;

    __HAL_TIM_SET_COMPARE(&htim_pwm, tim_channel_map[Channel], ccr);
    return E_OK;
}

/**
 * @brief  Set PWM channel to idle (0% duty)
 * @param  Channel  Channel index (0..3)
 * @return E_OK on success, E_NOT_OK on invalid channel
 */
Std_ReturnType Pwm_Hw_SetIdle(uint8 Channel)
{
    if (Channel >= pwm_active_channels) {
        return E_NOT_OK;
    }

    __HAL_TIM_SET_COMPARE(&htim_pwm, tim_channel_map[Channel], 0u);
    return E_OK;
}
