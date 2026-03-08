/**
 * @file    Uart_Hw_STM32.c
 * @brief   STM32G474 hardware backend for UART MCAL driver
 * @date    2026-03-08
 *
 * @details Real UART implementation using HAL UART with DMA circular RX.
 *          FZC uses USART2 (PA2=TX, PA3=RX) for TFMini-S lidar @ 115200.
 *
 *          DMA circular mode: UART RX runs continuously, writing received
 *          bytes into a ring buffer via DMA. Application reads from the
 *          software ring buffer, tracking DMA NDTR to compute available bytes.
 *
 *          IMPORTANT: On Nucleo boards, PA2/PA3 are routed to ST-LINK VCP
 *          via solder bridges SB63/SB65. These must be removed for
 *          production FZC to connect to TFMini-S. During F2 bring-up,
 *          USART2 doubles as debug TX (see fzc_hw_stm32.c).
 *
 * @safety_req SWR-BSW-010: UART Driver for TFMini-S Lidar
 * @copyright Taktflow Systems 2026
 */

#include "Platform_Types.h"
#include "Std_Types.h"
#include "Uart.h"
#include "stm32g4xx_hal.h"

/* ==================================================================
 * Static state
 * ================================================================== */

/** DMA circular RX buffer */
static uint8 uart_dma_buf[UART_RX_BUF_SIZE];

/** Software read position in DMA buffer */
static uint8 uart_rx_read_pos = 0u;

/** HAL UART handle */
static UART_HandleTypeDef huart;

/** HAL DMA handle for UART RX */
static DMA_HandleTypeDef hdma_uart_rx;

/* ==================================================================
 * Uart_Hw_* API implementations
 * ================================================================== */

/**
 * @brief  Initialize UART hardware with DMA circular RX
 * @param  baudRate  Baud rate (e.g. 115200 for TFMini-S)
 * @return E_OK on success, E_NOT_OK on failure
 *
 * @note   CubeMX MSP init handles GPIO (PA2/PA3 AF7) and DMA clock.
 *         This function configures USART2 parameters and starts DMA RX.
 */
Std_ReturnType Uart_Hw_Init(uint32 baudRate)
{
    uint8 i;

    for (i = 0u; i < UART_RX_BUF_SIZE; i++) {
        uart_dma_buf[i] = 0u;
    }
    uart_rx_read_pos = 0u;

    /* USART2 configuration: 8N1, RX+TX */
    huart.Instance          = USART2;
    huart.Init.BaudRate     = baudRate;
    huart.Init.WordLength   = UART_WORDLENGTH_8B;
    huart.Init.StopBits     = UART_STOPBITS_1;
    huart.Init.Parity       = UART_PARITY_NONE;
    huart.Init.Mode         = UART_MODE_TX_RX;
    huart.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart) != HAL_OK) {
        return E_NOT_OK;
    }

    /* Start DMA circular RX — bytes flow into uart_dma_buf continuously */
    if (HAL_UART_Receive_DMA(&huart, uart_dma_buf,
                              (uint16)UART_RX_BUF_SIZE) != HAL_OK) {
        return E_NOT_OK;
    }

    return E_OK;
}

/**
 * @brief  De-initialize UART hardware
 */
void Uart_Hw_DeInit(void)
{
    (void)HAL_UART_DMAStop(&huart);
    (void)HAL_UART_DeInit(&huart);
    uart_rx_read_pos = 0u;
}

/**
 * @brief  Get number of bytes available in RX DMA buffer
 * @return Number of unread bytes
 *
 * @note   Uses DMA NDTR (Number of Data To Register) to compute
 *         how many bytes DMA has written since our last read position.
 */
uint8 Uart_Hw_GetRxCount(void)
{
    uint8 dma_write_pos;
    uint8 available;

    /* DMA writes circularly. NDTR counts DOWN from buffer size.
     * Current DMA write position = UART_RX_BUF_SIZE - NDTR */
    dma_write_pos = (uint8)(UART_RX_BUF_SIZE
                   - __HAL_DMA_GET_COUNTER(huart.hdmarx));

    if (dma_write_pos >= uart_rx_read_pos) {
        available = dma_write_pos - uart_rx_read_pos;
    } else {
        available = (uint8)(UART_RX_BUF_SIZE - uart_rx_read_pos + dma_write_pos);
    }

    return available;
}

/**
 * @brief  Read bytes from UART DMA circular buffer
 * @param  Buffer    Output buffer
 * @param  Length    Maximum bytes to read
 * @param  BytesRead Output: actual bytes read
 * @return E_OK always
 *
 * @note   Reads from software tracking position up to DMA write position.
 *         Handles ring buffer wraparound.
 */
Std_ReturnType Uart_Hw_ReadRx(uint8* Buffer, uint8 Length, uint8* BytesRead)
{
    uint8 available;
    uint8 to_read;
    uint8 i;

    if ((Buffer == NULL_PTR) || (BytesRead == NULL_PTR)) {
        if (BytesRead != NULL_PTR) {
            *BytesRead = 0u;
        }
        return E_OK;
    }

    available = Uart_Hw_GetRxCount();
    to_read = (Length < available) ? Length : available;

    for (i = 0u; i < to_read; i++) {
        Buffer[i] = uart_dma_buf[uart_rx_read_pos];
        uart_rx_read_pos = (uart_rx_read_pos + 1u) % UART_RX_BUF_SIZE;
    }

    *BytesRead = to_read;
    return E_OK;
}

/**
 * @brief  Get UART status
 * @return UART_IDLE (1u) or UART_BUSY (2u)
 */
uint8 Uart_Hw_GetStatus(void)
{
    HAL_UART_StateTypeDef state = HAL_UART_GetState(&huart);

    if ((state == HAL_UART_STATE_BUSY_RX) ||
        (state == HAL_UART_STATE_BUSY_TX_RX)) {
        return 2u; /* UART_BUSY — DMA RX active (normal for circular mode) */
    }
    return 1u; /* UART_IDLE */
}
