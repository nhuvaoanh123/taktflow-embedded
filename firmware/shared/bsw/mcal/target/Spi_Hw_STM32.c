/**
 * @file    Spi_Hw_STM32.c
 * @brief   STM32G474 hardware backend for SPI MCAL driver
 * @date    2026-03-08
 *
 * @details Real SPI implementation using HAL SPI for AS5048A angle sensors.
 *          CVC uses SPI1 (PA5=SCK, PA6=MISO, PA7=MOSI) with two CS lines
 *          (PA4=CS1, PA15=CS2) for dual pedal sensors.
 *          FZC uses SPI2 (PB13=SCK, PB14=MISO, PB15=MOSI, PB12=CS)
 *          for steering angle sensor.
 *
 *          GPIO and SPI clock configuration handled by CubeMX MSP init.
 *          This driver manages CS assertion and full-duplex transfers.
 *
 *          AS5048A protocol: CPOL=0, CPHA=1, 16-bit frames, MSB first.
 *          Read angle: TX = 0xFFFF (NOP), RX[0] = 14-bit angle.
 *
 * @safety_req SWR-BSW-006: SPI Driver for AS5048A Sensors
 * @copyright Taktflow Systems 2026
 */

#include "Platform_Types.h"
#include "Std_Types.h"
#include "Spi.h"
#include "stm32g4xx_hal.h"

/* ==================================================================
 * Static state
 * ================================================================== */

/** HAL SPI handle — initialized by CubeMX HAL_SPI_MspInit() */
static SPI_HandleTypeDef hspi;

/** CS pin mapping: [channel] = {GPIO port, pin number}
 *  CVC: ch0=PA4 (CS1), ch1=PA15 (CS2)
 *  FZC: ch0=PB12 (CS)
 *  Populated at init time based on which SPI peripheral is found. */
typedef struct {
    GPIO_TypeDef* port;
    uint16        pin;
} Spi_CsPin;

#define SPI_HW_MAX_CS  4u

static Spi_CsPin spi_cs[SPI_HW_MAX_CS];
static uint8     spi_cs_count = 0u;

/** Transfer timeout in ms */
#define SPI_HW_TIMEOUT_MS  10u

/* ==================================================================
 * Spi_Hw_* API implementations
 * ================================================================== */

/**
 * @brief  Initialize SPI hardware
 * @return E_OK on success, E_NOT_OK on failure
 *
 * @note   Tries SPI1 first (CVC), falls back to SPI2 (FZC).
 *         CubeMX MSP init handles GPIO AF and clock enable.
 *         CS pins are software-managed GPIO outputs.
 */
Std_ReturnType Spi_Hw_Init(void)
{
    uint8 i;

    for (i = 0u; i < SPI_HW_MAX_CS; i++) {
        spi_cs[i].port = NULL;
        spi_cs[i].pin  = 0u;
    }

    /* Configure SPI peripheral: AS5048A requires CPOL=0, CPHA=1, 16-bit */
    hspi.Instance               = SPI1;
    hspi.Init.Mode              = SPI_MODE_MASTER;
    hspi.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi.Init.DataSize          = SPI_DATASIZE_16BIT;
    hspi.Init.CLKPolarity       = SPI_POLARITY_LOW;   /* CPOL=0 */
    hspi.Init.CLKPhase          = SPI_PHASE_2EDGE;     /* CPHA=1 */
    hspi.Init.NSS               = SPI_NSS_SOFT;
    hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64; /* ~2.6 MHz @ 170 MHz */
    hspi.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hspi.Init.NSSPMode          = SPI_NSS_PULSE_DISABLE;

    if (HAL_SPI_Init(&hspi) != HAL_OK) {
        /* SPI1 failed — try SPI2 (FZC) */
        hspi.Instance = SPI2;
        if (HAL_SPI_Init(&hspi) != HAL_OK) {
            return E_NOT_OK;
        }

        /* FZC: SPI2, CS on PB12 */
        spi_cs[0].port = GPIOB;
        spi_cs[0].pin  = GPIO_PIN_12;
        spi_cs_count   = 1u;
    } else {
        /* CVC: SPI1, CS1=PA4, CS2=PA15 */
        spi_cs[0].port = GPIOA;
        spi_cs[0].pin  = GPIO_PIN_4;
        spi_cs[1].port = GPIOA;
        spi_cs[1].pin  = GPIO_PIN_15;
        spi_cs_count   = 2u;
    }

    /* De-assert all CS lines (active low → set high) */
    for (i = 0u; i < spi_cs_count; i++) {
        HAL_GPIO_WritePin(spi_cs[i].port, spi_cs[i].pin, GPIO_PIN_SET);
    }

    return E_OK;
}

/**
 * @brief  SPI full-duplex transmit/receive
 * @param  Channel  CS channel index (0=CS1, 1=CS2)
 * @param  TxBuf    Transmit buffer (16-bit words)
 * @param  RxBuf    Receive buffer (16-bit words)
 * @param  Length   Transfer length in 16-bit words
 * @return E_OK on success, E_NOT_OK on failure
 *
 * @note   Manages CS assertion: assert (low) before transfer,
 *         de-assert (high) after. HAL_SPI_TransmitReceive handles
 *         the full-duplex clock generation.
 */
Std_ReturnType Spi_Hw_Transmit(uint8 Channel, const uint16* TxBuf,
                                uint16* RxBuf, uint8 Length)
{
    HAL_StatusTypeDef status;

    if (Channel >= spi_cs_count) {
        return E_NOT_OK;
    }
    if ((TxBuf == NULL_PTR) && (RxBuf == NULL_PTR)) {
        return E_NOT_OK;
    }
    if (Length == 0u) {
        return E_OK;
    }

    /* Assert CS (active low) */
    HAL_GPIO_WritePin(spi_cs[Channel].port, spi_cs[Channel].pin,
                      GPIO_PIN_RESET);

    /* Full-duplex transfer */
    if ((TxBuf != NULL_PTR) && (RxBuf != NULL_PTR)) {
        status = HAL_SPI_TransmitReceive(
            &hspi, (uint8*)TxBuf, (uint8*)RxBuf,
            (uint16)Length, SPI_HW_TIMEOUT_MS);
    } else if (RxBuf != NULL_PTR) {
        /* Read-only: send 0xFFFF (NOP) to clock data in */
        uint16 nop = 0xFFFFu;
        uint8 i;
        status = HAL_OK;
        for (i = 0u; (i < Length) && (status == HAL_OK); i++) {
            status = HAL_SPI_TransmitReceive(
                &hspi, (uint8*)&nop, (uint8*)&RxBuf[i],
                1u, SPI_HW_TIMEOUT_MS);
        }
    } else {
        /* Write-only */
        status = HAL_SPI_Transmit(
            &hspi, (uint8*)TxBuf, (uint16)Length, SPI_HW_TIMEOUT_MS);
    }

    /* De-assert CS */
    HAL_GPIO_WritePin(spi_cs[Channel].port, spi_cs[Channel].pin,
                      GPIO_PIN_SET);

    return (status == HAL_OK) ? E_OK : E_NOT_OK;
}

/**
 * @brief  Get SPI peripheral status
 * @return SPI_IDLE (1) or SPI_BUSY (2)
 */
uint8 Spi_Hw_GetStatus(void)
{
    HAL_SPI_StateTypeDef state = HAL_SPI_GetState(&hspi);

    if (state == HAL_SPI_STATE_BUSY) {
        return 2u; /* SPI_BUSY */
    }
    return 1u; /* SPI_IDLE */
}
