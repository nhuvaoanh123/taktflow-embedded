/**
 * @file    cvc_hw_stm32.c
 * @brief   STM32 hardware backend for CVC (Central Vehicle Computer)
 * @date    2026-03-03
 *
 * @details Phase F1.5: SysTick via HAL + bare-metal USART2 debug TX.
 *          Self-test and peripheral stubs still return E_OK (real
 *          implementations come in F2-F4).
 *
 *          Timing: HAL_Init() configures SysTick at 1ms (HSI 16 MHz).
 *          UART:   Bare-metal USART2 PA2=TX AF7, 115200 baud.
 *
 * @safety_req N/A — debug bring-up, not for production
 * @copyright Taktflow Systems 2026
 */

#include "Platform_Types.h"
#include "Std_Types.h"
#include "stm32g4xx_hal.h"

/* ==================================================================
 * UART Debug Output — bare-metal USART2 (PA2=TX, Nucleo VCP)
 * ================================================================== */

/**
 * @brief  Initialize USART2 for debug TX at 115200 baud (HSI 16 MHz)
 * @note   Bare-metal register access — no HAL UART dependency.
 *         PA2 = USART2_TX (AF7), connected to ST-LINK VCP on Nucleo.
 */
static void Dbg_Uart_Init(void)
{
    /* Enable GPIOA and USART2 peripheral clocks */
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;

    /* Read-back for clock stabilization (silicon errata workaround) */
    (void)RCC->APB1ENR1;

    /* Configure PA2 as alternate function (MODER=0b10) */
    GPIOA->MODER  = (GPIOA->MODER  & ~(3u << (2u * 2u)))
                   | (2u << (2u * 2u));

    /* PA2 alternate function = AF7 (USART2_TX) in AFRL register */
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xFu << (2u * 4u)))
                   | (7u << (2u * 4u));

    /* USART2: 115200 baud at 16 MHz HSI, 8N1, TX only */
    USART2->BRR = 139u;                            /* 16000000 / 115200 = 138.9 */
    USART2->CR1 = USART_CR1_TE | USART_CR1_UE;     /* Enable transmitter + USART */

    /* Wait for TE acknowledge before first transmission */
    while ((USART2->ISR & USART_ISR_TEACK) == 0u)
    {
        /* spin */
    }
}

/**
 * @brief  Blocking single-character transmit on USART2
 * @param  c  Character to send
 */
static void Dbg_Uart_PutChar(char c)
{
    while ((USART2->ISR & USART_ISR_TXE_TXFNF) == 0u)
    {
        /* Wait for TX data register empty */
    }
    USART2->TDR = (uint8)c;
}

/**
 * @brief  Print null-terminated string to USART2 (blocking)
 * @param  str  Null-terminated string to transmit
 * @note   Non-static — called from main.c via extern declaration.
 */
void Dbg_Uart_Print(const char *str)
{
    while (*str != '\0')
    {
        Dbg_Uart_PutChar(*str);
        str++;
    }
}

/* ==================================================================
 * Timing — SysTick via HAL (ISR in CubeMX stm32g4xx_it.c)
 * ================================================================== */

/**
 * @brief  Initialize system clocks and debug UART
 * @note   HAL_Init() configures SysTick at 1ms using HSI 16 MHz.
 *         UART init + boot banner printed at earliest opportunity.
 */
void Main_Hw_SystemClockInit(void)
{
    /* HAL_Init: flash prefetch, SysTick 1ms, NVIC priority grouping */
    (void)HAL_Init();

    /* Initialize debug UART (earliest possible output) */
    Dbg_Uart_Init();
    Dbg_Uart_Print("\r\n=== CVC Boot (HSI 16 MHz) ===\r\n");
}

/**
 * @brief  Configure MPU regions — no-op stub
 * @note   TODO:HARDWARE — configure MPU via HAL_MPU_ConfigRegion()
 */
void Main_Hw_MpuConfig(void)
{
    /* TODO:HARDWARE — configure MPU via HAL_MPU_ConfigRegion() */
}

/**
 * @brief  Initialize SysTick timer
 * @param  periodUs  Tick period in microseconds (expected: 1000 = 1ms)
 * @note   SysTick already configured by HAL_Init() at 1ms. This call
 *         validates the expected period and serves as a synchronization
 *         point before entering the main loop.
 */
void Main_Hw_SysTickInit(uint32 periodUs)
{
    (void)periodUs;
    /* SysTick already running from HAL_Init() — 1ms at HSI 16 MHz.
     * HAL_IncTick() ISR in stm32g4xx_it.c increments the HAL tick. */
}

/**
 * @brief  Wait for interrupt — saves power between ticks
 */
void Main_Hw_Wfi(void)
{
    __WFI();
}

/**
 * @brief  Get elapsed time since boot in microseconds
 * @return Elapsed microseconds (HAL tick * 1000)
 * @note   Resolution is 1ms (1000us steps). Overflows after ~49 days.
 */
uint32 Main_Hw_GetTick(void)
{
    return HAL_GetTick() * 1000u;
}

/* ==================================================================
 * Startup self-test stubs (Main_Hw_* from main.c)
 * ================================================================== */

/**
 * @brief  SPI loopback self-test — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — perform real SPI loopback via HAL_SPI_TransmitReceive()
 */
Std_ReturnType Main_Hw_SpiLoopbackTest(void)
{
    /* TODO:HARDWARE — real SPI loopback test */
    return E_OK;
}

/**
 * @brief  CAN loopback self-test — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — perform real CAN loopback via HAL_CAN
 */
Std_ReturnType Main_Hw_CanLoopbackTest(void)
{
    /* TODO:HARDWARE — real CAN loopback test */
    return E_OK;
}

/**
 * @brief  OLED I2C ACK self-test — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — probe SSD1306 via HAL_I2C_IsDeviceReady()
 */
Std_ReturnType Main_Hw_OledAckTest(void)
{
    /* TODO:HARDWARE — real OLED ACK probe */
    return E_OK;
}

/**
 * @brief  RAM pattern self-test — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — write/read march-C pattern to SRAM
 */
Std_ReturnType Main_Hw_RamPatternTest(void)
{
    /* TODO:HARDWARE — real RAM pattern test */
    return E_OK;
}

/**
 * @brief  Plant stack canary — no-op stub
 * @note   TODO:HARDWARE — write canary word at end of stack region
 */
void Main_Hw_PlantStackCanary(void)
{
    /* TODO:HARDWARE — plant canary at stack boundary */
}

/* ==================================================================
 * Periodic self-test stubs (SelfTest_Hw_* from Swc_SelfTest.c)
 * ================================================================== */

/**
 * @brief  Periodic SPI loopback self-test — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — periodic SPI loopback via HAL
 */
Std_ReturnType SelfTest_Hw_SpiLoopback(void)
{
    /* TODO:HARDWARE — real periodic SPI loopback */
    return E_OK;
}

/**
 * @brief  Periodic CAN loopback self-test — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — periodic CAN loopback via HAL
 */
Std_ReturnType SelfTest_Hw_CanLoopback(void)
{
    /* TODO:HARDWARE — real periodic CAN loopback */
    return E_OK;
}

/**
 * @brief  NVM integrity check — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — read NVM CRC and verify
 */
Std_ReturnType SelfTest_Hw_NvmCheck(void)
{
    /* TODO:HARDWARE — real NVM integrity check */
    return E_OK;
}

/**
 * @brief  Periodic OLED I2C ACK self-test — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — periodic SSD1306 ACK probe
 */
Std_ReturnType SelfTest_Hw_OledAck(void)
{
    /* TODO:HARDWARE — real periodic OLED ACK */
    return E_OK;
}

/**
 * @brief  MPU region verify — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — verify MPU region configuration registers
 */
Std_ReturnType SelfTest_Hw_MpuVerify(void)
{
    /* TODO:HARDWARE — real MPU region verify */
    return E_OK;
}

/**
 * @brief  Stack canary check — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — compare canary word at stack boundary
 */
Std_ReturnType SelfTest_Hw_CanaryCheck(void)
{
    /* TODO:HARDWARE — real canary check */
    return E_OK;
}

/**
 * @brief  Periodic RAM pattern test — stub returns E_OK
 * @return E_OK
 * @note   TODO:HARDWARE — periodic march-C on reserved SRAM block
 */
Std_ReturnType SelfTest_Hw_RamPattern(void)
{
    /* TODO:HARDWARE — real periodic RAM pattern test */
    return E_OK;
}

/* ==================================================================
 * SSD1306 OLED I2C stub
 * ================================================================== */

/**
 * @brief  Write data to I2C bus — stub returns E_OK
 * @param  addr  7-bit I2C slave address
 * @param  data  Pointer to data buffer
 * @param  len   Number of bytes to write
 * @return E_OK always (stub — no hardware)
 * @note   TODO:HARDWARE — call HAL_I2C_Master_Transmit()
 */
Std_ReturnType Ssd1306_Hw_I2cWrite(uint8 addr, const uint8* data, uint8 len)
{
    (void)addr;
    (void)data;
    (void)len;
    /* TODO:HARDWARE — real I2C write via HAL_I2C_Master_Transmit() */
    return E_OK;
}
