/**
 * @file    main.c
 * @brief   Bare-metal blinky smoke test for Nucleo-G474RE (LD2 on PA5)
 * @date    2026-03-03
 *
 * @details  Minimal blinky that toggles LD2 (PA5) at approximately 1 Hz
 *           using SysTick for timing.  No HAL dependency -- uses direct
 *           CMSIS-style register access against STM32G4 memory map.
 *
 *           Clock: HSI 16 MHz (default after reset, no PLL).
 *           SysTick: 1 ms tick derived from HSI.
 *
 *           This is a hardware smoke test -- if the LED blinks, the
 *           toolchain, flash, and basic clocking are confirmed working.
 *
 * @standard MISRA C:2012 (advisory deviations noted inline)
 * @copyright Taktflow Systems 2026
 */

/* ==================================================================
 * STM32G4 Register Definitions (CMSIS-style, minimal subset)
 * ================================================================== */

#include <stdint.h>

/* ---- Base addresses -------------------------------------------------- */
#define PERIPH_BASE         ((uint32_t)0x40000000u)
#define AHB2_PERIPH_BASE    (PERIPH_BASE + 0x08000000u)
#define GPIOA_BASE          (AHB2_PERIPH_BASE + 0x00000000u)

#define RCC_BASE            (PERIPH_BASE + 0x01021000u) /* AHB1: 0x40021000 */
#define RCC_AHB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x4Cu))

#define SYSTICK_BASE        ((uint32_t)0xE000E010u)
#define SYSTICK_CSR         (*(volatile uint32_t *)(SYSTICK_BASE + 0x00u))
#define SYSTICK_RVR         (*(volatile uint32_t *)(SYSTICK_BASE + 0x04u))
#define SYSTICK_CVR         (*(volatile uint32_t *)(SYSTICK_BASE + 0x08u))

/* ---- GPIO register offsets (applied to GPIOA_BASE) ------------------- */
#define GPIOA_MODER         (*(volatile uint32_t *)(GPIOA_BASE + 0x00u))
#define GPIOA_OTYPER        (*(volatile uint32_t *)(GPIOA_BASE + 0x04u))
#define GPIOA_OSPEEDR       (*(volatile uint32_t *)(GPIOA_BASE + 0x08u))
#define GPIOA_PUPDR         (*(volatile uint32_t *)(GPIOA_BASE + 0x0Cu))
#define GPIOA_ODR           (*(volatile uint32_t *)(GPIOA_BASE + 0x14u))

/* ---- Bit definitions ------------------------------------------------- */
#define RCC_AHB2ENR_GPIOAEN ((uint32_t)(1u << 0u))

#define LED_PIN             5u  /* LD2 = PA5 on Nucleo-G474RE */

#define SYSTICK_CSR_ENABLE  (1u << 0u)
#define SYSTICK_CSR_TICKINT (1u << 1u)
#define SYSTICK_CSR_CLKSRC  (1u << 2u)  /* 1 = processor clock */

/* ---- HSI clock frequency (default after reset) ----------------------- */
#define HSI_FREQ_HZ         16000000u
#define SYSTICK_1MS_RELOAD   ((HSI_FREQ_HZ / 1000u) - 1u)  /* 15999 */

/* ==================================================================
 * SysTick Millisecond Counter
 * ================================================================== */

static volatile uint32_t g_tick_ms;

/**
 * @brief  SysTick interrupt handler -- increments millisecond counter.
 * @note   Called every 1 ms by hardware.  Name must match startup vector table.
 */
void SysTick_Handler(void)
{
    g_tick_ms++;
}

/* ==================================================================
 * Delay (blocking, ms)
 * ================================================================== */

/**
 * @brief  Blocking delay using SysTick counter.
 * @param  ms  Number of milliseconds to wait.
 */
static void delay_ms(uint32_t ms)
{
    uint32_t start = g_tick_ms;

    while ((g_tick_ms - start) < ms) {
        /* Wait -- SysTick_Handler advances g_tick_ms */
    }
}

/* ==================================================================
 * Fallback: volatile software delay (if SysTick is not available)
 * ================================================================== */

/**
 * @brief  Rough software delay loop (~1 s at 16 MHz HSI).
 * @note   Only used as a fallback if SysTick init fails.
 *         Timing is approximate and depends on compiler optimization.
 */
static void delay_sw_1s(void)
{
    volatile uint32_t i;

    for (i = 0u; i < 1600000u; i++) {
        /* empty -- burns CPU cycles */
    }
}

/* ==================================================================
 * Peripheral Initialization
 * ================================================================== */

/**
 * @brief  Configure SysTick for 1 ms tick using processor clock (HSI 16 MHz).
 * @return 0 on success, 1 on failure (reload value out of range).
 */
static uint32_t systick_init(void)
{
    uint32_t result = 0u;

    if (SYSTICK_1MS_RELOAD > 0x00FFFFFFu) {
        /* Reload value exceeds 24-bit SysTick counter -- should not happen at 16 MHz */
        result = 1u;
    } else {
        SYSTICK_RVR = SYSTICK_1MS_RELOAD;
        SYSTICK_CVR = 0u;  /* Clear current value */
        SYSTICK_CSR = SYSTICK_CSR_CLKSRC | SYSTICK_CSR_TICKINT | SYSTICK_CSR_ENABLE;
    }

    return result;
}

/**
 * @brief  Enable GPIOA clock and configure PA5 as push-pull output (low speed).
 */
static void led_init(void)
{
    /* Enable GPIOA peripheral clock */
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    /* Brief delay for clock to stabilize (2 AHB cycles minimum) */
    {
        volatile uint32_t dummy = RCC_AHB2ENR;
        (void)dummy;
    }

    /* MODER: set PA5 to General Purpose Output (01) */
    /* Each pin uses 2 bits: clear bits [11:10], then set to 01 */
    GPIOA_MODER &= ~(3u << (LED_PIN * 2u));
    GPIOA_MODER |=  (1u << (LED_PIN * 2u));

    /* OTYPER: push-pull (0) -- default after reset, explicit for clarity */
    GPIOA_OTYPER &= ~(1u << LED_PIN);

    /* OSPEEDR: low speed (00) -- sufficient for LED */
    GPIOA_OSPEEDR &= ~(3u << (LED_PIN * 2u));

    /* PUPDR: no pull-up/pull-down (00) */
    GPIOA_PUPDR &= ~(3u << (LED_PIN * 2u));
}

/* ==================================================================
 * LED Toggle
 * ================================================================== */

/**
 * @brief  Toggle LD2 (PA5) by XOR-ing the output data register.
 */
static void led_toggle(void)
{
    GPIOA_ODR ^= (1u << LED_PIN);
}

/* ==================================================================
 * Main Entry Point
 * ================================================================== */

/**
 * @brief  Blinky main -- init peripherals, then toggle LED at ~1 Hz.
 * @return Does not return (infinite loop).
 */
int main(void)
{
    uint32_t systick_ok;

    /* ---- Step 1: Configure LED output ---- */
    led_init();

    /* ---- Step 2: Configure SysTick for 1 ms tick ---- */
    systick_ok = systick_init();

    /* ---- Step 3: Blink forever at ~1 Hz (500 ms on, 500 ms off) ---- */
    if (systick_ok == 0u) {
        /* SysTick available -- use precise ms delay */
        for (;;) {
            led_toggle();
            delay_ms(500u);
        }
    } else {
        /* Fallback -- use rough software delay */
        for (;;) {
            led_toggle();
            delay_sw_1s();
        }
    }

    /* Never reached */
    return 0;
}
