/**
 * @file    Spi_Posix.c
 * @brief   POSIX SPI stub — implements Spi_Hw_* externs from Spi.h
 * @date    2026-02-23
 *
 * @details No-op stub. Simulated ECUs (BCM, ICU, TCU) do not use SPI.
 *          Provided for API completeness so the BSW stack links cleanly.
 *
 * @safety_req SWR-BSW-006: SPI Driver
 * @traces_to  SYS-047, TSR-001, TSR-010
 *
 * @copyright Taktflow Systems 2026
 */

#include "Platform_Types.h"
#include "Std_Types.h"

/* ---- Spi_Hw_* implementations ---- */

/**
 * @brief  Initialize SPI hardware (POSIX: no-op)
 * @return E_OK always
 */
Std_ReturnType Spi_Hw_Init(void)
{
    return E_OK;
}

/**
 * @brief  Simulated sensor angle — oscillates to avoid stuck detection
 *
 * Range 200-800 of 14-bit (0-16383) keeps pedal position in the torque
 * dead zone (position < 67 → torque = 0), so simulated vehicle stays
 * stationary.  Step 7 per Spi_Hw_Transmit call.
 * Pedal SWC reads 2 sensors per 10ms cycle = 2 Hw_Transmit calls.
 * Sensor-to-sensor delta = 7 (< plausibility threshold 819).
 * Cycle-to-cycle delta per sensor = 14 (>= stuck threshold 10).
 */
static uint16 spi_sim_angle = 400u;
static uint8  spi_sim_up    = 1u;

/**
 * @brief  SPI transmit (POSIX: returns simulated AS5048A angle sensor data)
 * @param  Channel  SPI channel
 * @param  TxBuf    Transmit buffer (ignored)
 * @param  RxBuf    Receive buffer — filled with simulated sensor response
 * @param  Length   Transfer length in words
 * @return E_OK always
 */
Std_ReturnType Spi_Hw_Transmit(uint8 Channel, const uint16* TxBuf,
                                uint16* RxBuf, uint8 Length)
{
    (void)Channel;
    (void)TxBuf;

    if ((RxBuf != NULL_PTR) && (Length > 0u))
    {
        /* Store simulated angle directly as uint16 — IoHwAb reads
         * rx_data[0] & 0x3FFF from the uint16 word buffer */
        RxBuf[0] = spi_sim_angle & 0x3FFFu;

        /* Advance simulated angle — oscillate within safe bounds */
        if (spi_sim_up != 0u)
        {
            spi_sim_angle += 7u;
            if (spi_sim_angle > 800u)
            {
                spi_sim_up = 0u;
            }
        }
        else
        {
            spi_sim_angle -= 7u;
            if (spi_sim_angle < 200u)
            {
                spi_sim_up = 1u;
            }
        }
    }

    return E_OK;
}

/**
 * @brief  Get SPI status (POSIX: always idle)
 * @return SPI_IDLE (1)
 */
uint8 Spi_Hw_GetStatus(void)
{
    return 1u; /* SPI_IDLE */
}
