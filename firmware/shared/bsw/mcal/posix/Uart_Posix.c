/**
 * @file    Uart_Posix.c
 * @brief   POSIX UART stub — implements Uart_Hw_* externs from Uart.h
 * @date    2026-02-23
 *
 * @details Simulated UART with injectable RX ring buffer. FZC uses UART
 *          for TFMini-S lidar — in SIL the plant simulator feeds lidar
 *          frames via Uart_Posix_InjectRx() (or via CAN as fallback).
 *
 *          TFMini-S frame format: 59 59 [dist_lo] [dist_hi]
 *              [strength_lo] [strength_hi] [temp_lo] [temp_hi] [checksum]
 *          9 bytes per frame at 100 Hz = 900 bytes/s @ 115200 baud.
 *
 * @safety_req SWR-BSW-010: UART Driver
 * @traces_to  SYS-050, TSR-001, TSR-015
 *
 * @copyright Taktflow Systems 2026
 */

#include "Platform_Types.h"
#include "Std_Types.h"

/* ---- Ring buffer for injectable UART RX data ---- */

#define UART_POSIX_RX_BUF_SIZE  128u

static uint8  uart_rx_buf[UART_POSIX_RX_BUF_SIZE];
static uint8  uart_rx_head = 0u;  /**< Write index (producer)  */
static uint8  uart_rx_tail = 0u;  /**< Read index (consumer)   */
static uint8  uart_rx_count = 0u; /**< Bytes available         */

/* ---- Uart_Hw_* implementations ---- */

/**
 * @brief  Initialize UART hardware (POSIX: reset ring buffer)
 * @param  baudRate  Baud rate (ignored in simulation)
 * @return E_OK always
 */
Std_ReturnType Uart_Hw_Init(uint32 baudRate)
{
    (void)baudRate;
    uart_rx_head  = 0u;
    uart_rx_tail  = 0u;
    uart_rx_count = 0u;
    return E_OK;
}

/**
 * @brief  De-initialize UART hardware (POSIX: flush buffer)
 */
void Uart_Hw_DeInit(void)
{
    uart_rx_head  = 0u;
    uart_rx_tail  = 0u;
    uart_rx_count = 0u;
}

/**
 * @brief  Get number of bytes available in RX buffer
 * @return Number of bytes available to read
 */
uint8 Uart_Hw_GetRxCount(void)
{
    return uart_rx_count;
}

/**
 * @brief  Read bytes from UART RX buffer
 * @param  Buffer    Output buffer
 * @param  Length    Maximum bytes to read
 * @param  BytesRead Output: actual bytes read
 * @return E_OK always
 */
Std_ReturnType Uart_Hw_ReadRx(uint8* Buffer, uint8 Length, uint8* BytesRead)
{
    uint8 read = 0u;

    if ((Buffer == NULL_PTR) || (BytesRead == NULL_PTR)) {
        if (BytesRead != NULL_PTR) {
            *BytesRead = 0u;
        }
        return E_OK;
    }

    while ((read < Length) && (uart_rx_count > 0u)) {
        Buffer[read] = uart_rx_buf[uart_rx_tail];
        uart_rx_tail = (uart_rx_tail + 1u) % UART_POSIX_RX_BUF_SIZE;
        uart_rx_count--;
        read++;
    }

    *BytesRead = read;
    return E_OK;
}

/**
 * @brief  Get UART status (POSIX: idle when buffer empty, busy when data present)
 * @return UART_IDLE (1u) or UART_BUSY (2u)
 */
uint8 Uart_Hw_GetStatus(void)
{
    return (uart_rx_count > 0u) ? 2u : 1u; /* UART_BUSY / UART_IDLE */
}

/* ---- SIL injection API (plant simulator + test harness) ---- */

/**
 * @brief  Inject raw bytes into the UART RX buffer
 * @param  data   Pointer to data to inject
 * @param  length Number of bytes to inject
 * @return Number of bytes actually injected (may be less if buffer full)
 *
 * @details Called by the plant simulator to feed TFMini-S lidar frames
 *          into the FZC's UART RX path. The Swc_Lidar SWC reads from
 *          Uart_ReadRxData() → Uart_Hw_ReadRx() → this buffer.
 *
 *          Example: inject a TFMini-S distance frame (200cm, strength 1000):
 *            uint8 frame[] = {0x59, 0x59, 0xC8, 0x00, 0xE8, 0x03,
 *                             0x00, 0x00, checksum};
 *            Uart_Posix_InjectRx(frame, 9);
 */
uint8 Uart_Posix_InjectRx(const uint8* data, uint8 length)
{
    uint8 injected = 0u;

    if (data == NULL_PTR) {
        return 0u;
    }

    while ((injected < length) &&
           (uart_rx_count < UART_POSIX_RX_BUF_SIZE)) {
        uart_rx_buf[uart_rx_head] = data[injected];
        uart_rx_head = (uart_rx_head + 1u) % UART_POSIX_RX_BUF_SIZE;
        uart_rx_count++;
        injected++;
    }

    return injected;
}

/**
 * @brief  Flush the UART RX buffer (test reset)
 *
 * @details Convenience function for test setup — clears all buffered
 *          data to a known empty state before running test scenarios.
 */
void Uart_Posix_FlushRx(void)
{
    uart_rx_head  = 0u;
    uart_rx_tail  = 0u;
    uart_rx_count = 0u;
}
