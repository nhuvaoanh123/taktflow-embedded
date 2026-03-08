/**
 * @file    Dio_Hw_STM32.c
 * @brief   STM32G474 hardware backend for DIO MCAL driver
 * @date    2026-03-08
 *
 * @details Real GPIO implementation using atomic BSRR writes and IDR reads.
 *          Channel mapping: ChannelId encodes port (bits 7:4) and pin (bits 3:0).
 *            Port 0 = GPIOA, 1 = GPIOB, 2 = GPIOC, 3 = GPIOD
 *            Pin 0-15 within each port.
 *          Example: PA5 = 0x05, PB1 = 0x11, PC13 = 0x2D
 *
 *          GPIO clock enable and pin mode configuration (input/output/AF)
 *          is handled by CubeMX HAL MSP init, NOT by this driver.
 *          This driver only reads/writes pin levels.
 *
 * @safety_req SWR-BSW-009: DIO Driver for Digital I/O
 * @copyright Taktflow Systems 2026
 */

#include "Platform_Types.h"
#include "Std_Types.h"
#include "Dio.h"
#include "stm32g4xx_hal.h"

/* ---- Port lookup ---- */

/** Map port index (0-3) to GPIO peripheral base address */
static GPIO_TypeDef* const dio_port_map[4] = {
    GPIOA, GPIOB, GPIOC, GPIOD
};

#define DIO_PORT(ch)  ((uint8)((ch) >> 4u) & 0x03u)
#define DIO_PIN(ch)   ((uint8)((ch) & 0x0Fu))

/* ---- Dio_Hw_* implementations ---- */

/**
 * @brief  Read pin level from GPIO IDR register
 * @param  ChannelId  Encoded channel: bits[7:4]=port, bits[3:0]=pin
 * @return STD_HIGH or STD_LOW
 */
uint8 Dio_Hw_ReadPin(uint8 ChannelId)
{
    uint8 port_idx = DIO_PORT(ChannelId);
    uint8 pin_num  = DIO_PIN(ChannelId);

    if (port_idx > 3u) {
        return STD_LOW;
    }

    GPIO_TypeDef* port = dio_port_map[port_idx];

    if ((port->IDR & (1u << pin_num)) != 0u) {
        return STD_HIGH;
    }
    return STD_LOW;
}

/**
 * @brief  Write pin level via GPIO BSRR register (atomic set/reset)
 * @param  ChannelId  Encoded channel: bits[7:4]=port, bits[3:0]=pin
 * @param  Level      STD_HIGH or STD_LOW
 *
 * @note   BSRR lower 16 bits set pins, upper 16 bits reset pins.
 *         Single 32-bit write — atomic, no read-modify-write race.
 */
void Dio_Hw_WritePin(uint8 ChannelId, uint8 Level)
{
    uint8 port_idx = DIO_PORT(ChannelId);
    uint8 pin_num  = DIO_PIN(ChannelId);

    if (port_idx > 3u) {
        return;
    }

    GPIO_TypeDef* port = dio_port_map[port_idx];

    if (Level != STD_LOW) {
        port->BSRR = (1u << pin_num);           /* Set pin high   */
    } else {
        port->BSRR = (1u << (pin_num + 16u));   /* Reset pin low  */
    }
}
