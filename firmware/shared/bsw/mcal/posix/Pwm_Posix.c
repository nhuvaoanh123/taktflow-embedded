/**
 * @file    Pwm_Posix.c
 * @brief   POSIX PWM stub — implements Pwm_Hw_* externs from Pwm.h
 * @date    2026-02-23
 *
 * @details Simulated PWM that stores duty cycle values in a static array.
 *          No real PWM hardware. Provides injection/readback API for the
 *          plant simulator and test harness to observe actuator commands.
 *
 * @safety_req SWR-BSW-008: PWM Driver for Motor and Servo Control
 * @traces_to  SYS-050, TSR-005, TSR-012
 *
 * @copyright Taktflow Systems 2026
 */

#include "Platform_Types.h"
#include "Std_Types.h"

/* ---- Module state ---- */

#define PWM_POSIX_MAX_CHANNELS  8u

static uint16 pwm_duty[PWM_POSIX_MAX_CHANNELS];

/* ---- Pwm_Hw_* implementations ---- */

/**
 * @brief  Initialize PWM hardware (POSIX: zero all duties)
 * @return E_OK always
 */
Std_ReturnType Pwm_Hw_Init(void)
{
    uint8 i;
    for (i = 0u; i < PWM_POSIX_MAX_CHANNELS; i++) {
        pwm_duty[i] = 0u;
    }
    return E_OK;
}

/**
 * @brief  Set PWM duty cycle
 * @param  Channel    Channel index (0..7)
 * @param  DutyCycle  Duty (0x0000=0%, 0x8000=100%)
 * @return E_OK on success, E_NOT_OK on invalid channel
 */
Std_ReturnType Pwm_Hw_SetDuty(uint8 Channel, uint16 DutyCycle)
{
    if (Channel >= PWM_POSIX_MAX_CHANNELS) {
        return E_NOT_OK;
    }
    pwm_duty[Channel] = DutyCycle;
    return E_OK;
}

/**
 * @brief  Set PWM channel to idle (0% duty)
 * @param  Channel  Channel index (0..7)
 * @return E_OK on success, E_NOT_OK on invalid channel
 */
Std_ReturnType Pwm_Hw_SetIdle(uint8 Channel)
{
    if (Channel >= PWM_POSIX_MAX_CHANNELS) {
        return E_NOT_OK;
    }
    pwm_duty[Channel] = 0u;
    return E_OK;
}

/* ---- SIL readback / injection API (plant simulator + test harness) ---- */

/**
 * @brief  Read current duty cycle for a PWM channel (SIL observation)
 * @param  Channel  Channel index (0..7)
 * @return Duty cycle value (0x0000..0x8000), or 0 if invalid channel
 *
 * @details Allows the plant simulator to observe what duty cycle the
 *          ECU application has commanded, enabling closed-loop simulation
 *          of motors, servos, and other PWM-driven actuators.
 */
uint16 Pwm_Posix_ReadDuty(uint8 Channel)
{
    if (Channel >= PWM_POSIX_MAX_CHANNELS) {
        return 0u;
    }
    return pwm_duty[Channel];
}

/**
 * @brief  Inject a duty cycle value for a PWM channel (test override)
 * @param  Channel    Channel index (0..7)
 * @param  DutyCycle  Duty value (0x0000..0x8000)
 *
 * @details Allows tests to set PWM state directly, bypassing application
 *          logic. Useful for testing IoHwAb readback of motor/servo state.
 */
void Pwm_Posix_InjectDuty(uint8 Channel, uint16 DutyCycle)
{
    if (Channel < PWM_POSIX_MAX_CHANNELS) {
        pwm_duty[Channel] = DutyCycle;
    }
}
