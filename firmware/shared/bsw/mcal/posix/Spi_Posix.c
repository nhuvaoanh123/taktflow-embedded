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
 * @brief  SPI transmit (POSIX: no-op stub)
 * @param  Channel  SPI channel
 * @param  TxBuf    Transmit buffer
 * @param  RxBuf    Receive buffer
 * @param  Length   Transfer length
 * @return E_OK always (no actual transfer)
 */
Std_ReturnType Spi_Hw_Transmit(uint8 Channel, const uint16* TxBuf,
                                uint16* RxBuf, uint8 Length)
{
    (void)Channel;
    (void)TxBuf;
    (void)RxBuf;
    (void)Length;
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
