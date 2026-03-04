/**
 * @file    sc_hw_tms570.c
 * @brief   TMS570LC43x hardware implementation for SC (Safety Controller)
 * @date    2026-03-04
 *
 * @details Implements all hardware externs used across SC source files
 *          for the TMS570LC4357 LaunchPad target:
 *          - HALCoGen system/GIO/RTI wrappers (from sc_main.c)
 *          - DCAN1 register access for listen-only CAN (from sc_can.c)
 *          - Self-test hardware stubs (from sc_selftest.c)
 *          - ESM error signaling (from sc_esm.c)
 *          - SCI debug UART output (from sc_main.c)
 *
 *          The SC is bare-metal — no AUTOSAR BSW. Uses sc_types.h
 *          for type definitions. All HALCoGen-generated code is in
 *          halcogencfg/source/ and linked at build time.
 *
 * @safety_req SWR-SC-001 to SWR-SC-026
 * @traces_to  SSR-SC-001 to SSR-SC-017
 * @note    Safety level: ASIL D (self-test stubs to be replaced with
 *          real implementations after CAN bring-up)
 * @standard ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */

#ifdef PLATFORM_TMS570

#include "sc_types.h"
#include "sc_cfg.h"

/* ==================================================================
 * TMS570LC43x Register Base Addresses
 * ================================================================== */

/** DCAN1 base address (TMS570LC43x TRM Table 2-2) */
#define DCAN1_BASE              0xFFF7DC00u

/** DCAN message object RAM base (TMS570LC43x, DCAN1) */
#define DCAN1_MSG_RAM_BASE      0xFFF7E000u

/** GIO base address */
#define GIO_BASE                0xFFF7BC00u

/** RTI base address */
#define RTI_BASE                0xFFFFFC00u

/** ESM base address */
#define ESM_BASE                0xFFFFF500u

/** SCI/LIN base address (SCI1 for debug UART via XDS110) */
#define SCI_BASE                0xFFF7E400u

/* ==================================================================
 * GIO Register Offsets (from GIO_BASE)
 * ================================================================== */

#define GIO_GCR0                0x00u   /* Global control */
#define GIO_DIRA                0x34u   /* Port A direction */
#define GIO_DINA                0x38u   /* Port A data input */
#define GIO_DOUTA               0x3Cu   /* Port A data output */
#define GIO_DSETA               0x40u   /* Port A data set */
#define GIO_DCLRA               0x44u   /* Port A data clear */
#define GIO_DIRB                0x54u   /* Port B direction */
#define GIO_DINB                0x58u   /* Port B data input */
#define GIO_DOUTB               0x5Cu   /* Port B data output */
#define GIO_DSETB               0x60u   /* Port B data set */
#define GIO_DCLRB               0x64u   /* Port B data clear */

/* ==================================================================
 * RTI Register Offsets (from RTI_BASE)
 * ================================================================== */

#define RTI_GCTRL               0x00u   /* Global control */
#define RTI_COMPCTRL            0x0Cu   /* Compare control */
#define RTI_CNT0                0x10u   /* Free running counter 0 */
#define RTI_CMP0                0x50u   /* Compare 0 value */
#define RTI_UDCP0               0x54u   /* Update compare 0 */
#define RTI_INTFLAG             0x88u   /* Interrupt flag */
#define RTI_CLEARINT            0x8Cu   /* Interrupt clear (write-1-to-clear) */

/** RTI compare 0 interrupt flag bit */
#define RTI_CMP0_FLAG           0x01u

/* ==================================================================
 * ESM Register Offsets (from ESM_BASE)
 * ================================================================== */

#define ESM_EEPAPR1             0x00u   /* Group 1 enable set */
#define ESM_DEPAPR1             0x04u   /* Group 1 enable clear */
#define ESM_SR1                 0x18u   /* Group 1 status */
#define ESM_SR4                 0x24u   /* Group 1 status clear (write-1-to-clear) */
#define ESM_SR2                 0x1Cu   /* Group 2 status (read-only) */
/* Group 2 clear: write to ESM_SR2 (some TMS570 variants) or use EKR */

/* ==================================================================
 * SCI Register Offsets (from SCI_BASE)
 * ================================================================== */

#define SCI_GCR0                0x00u   /* Global control 0 */
#define SCI_GCR1                0x04u   /* Global control 1 */
#define SCI_BRS                 0x2Cu   /* Baud rate selection */
#define SCI_FLR                 0x1Cu   /* Flags register */
#define SCI_TD                  0x38u   /* Transmit data */

/** SCI TX ready flag */
#define SCI_FLR_TXRDY           0x00000100u

/* ==================================================================
 * DCAN Message Object RAM Layout
 *
 * Each message object is 32 bytes (8 words) starting at DCAN1_MSG_RAM_BASE.
 * Message object N (1-indexed) starts at:
 *   DCAN1_MSG_RAM_BASE + ((N-1) * 0x20)
 *
 * Word offsets within a message object:
 *   0x00: IF1CMD / (unused in RAM view)
 *   0x04: IF1MSK / (unused in RAM view)
 *   0x08: IF1ARB — Arbitration (CAN ID in bits 28:18 for std)
 *   0x0C: IF1MCTL — Message control (DLC bits 3:0, NewDat bit 15)
 *   0x10: IF1DATA — Data bytes 0-3 (byte 0 in bits 7:0)
 *   0x14: IF1DATB — Data bytes 4-7 (byte 4 in bits 7:0)
 *
 * For DCAN hardware, we use Interface Registers (IF1/IF2) to access
 * message object RAM. Direct RAM access is also possible on TMS570.
 * ================================================================== */

/** DCAN IF1 command register offsets (from DCAN1_BASE) */
#define DCAN_IF1CMD             0x100u
#define DCAN_IF1MSK             0x104u
#define DCAN_IF1ARB             0x108u
#define DCAN_IF1MCTL            0x10Cu
#define DCAN_IF1DATA            0x110u
#define DCAN_IF1DATB            0x114u

/** DCAN IF2 command register offsets (from DCAN1_BASE) — used for RX */
#define DCAN_IF2CMD             0x120u
#define DCAN_IF2MSK             0x124u
#define DCAN_IF2ARB             0x128u
#define DCAN_IF2MCTL            0x12Cu
#define DCAN_IF2DATA            0x130u
#define DCAN_IF2DATB            0x134u

/** IF command register bits (MISRA 12.2: use uint32 literal for shift) */
#define DCAN_IFCMD_BUSY         ((uint32)1u << 15u)  /* IF busy flag */
#define DCAN_IFCMD_DATAB        ((uint32)1u << 0u)   /* Access data B */
#define DCAN_IFCMD_DATAA        ((uint32)1u << 1u)   /* Access data A */
#define DCAN_IFCMD_NEWDAT       ((uint32)1u << 2u)   /* Access NewDat/IntPnd */
#define DCAN_IFCMD_CLRINTPND    ((uint32)1u << 3u)   /* Clear IntPnd */
#define DCAN_IFCMD_CONTROL      ((uint32)1u << 4u)   /* Access control bits */
#define DCAN_IFCMD_ARB          ((uint32)1u << 5u)   /* Access arbitration */
#define DCAN_IFCMD_MASK         ((uint32)1u << 6u)   /* Access mask */
#define DCAN_IFCMD_WR           ((uint32)1u << 7u)   /* Write (1) / Read (0) */

/** MCTL NewDat bit */
#define DCAN_MCTL_NEWDAT        ((uint32)1u << 15u)

/* ==================================================================
 * Helper: volatile register access
 * ================================================================== */

/* MISRA Rule 11.4 deviation: integer-to-pointer cast is required for
 * memory-mapped peripheral register access on bare-metal TMS570.
 * The addresses are fixed hardware register addresses from the TRM. */
static uint32 reg_read(uint32 base, uint32 offset)
{
    /* cppcheck-suppress misra-c2012-11.4 */
    volatile const uint32 *addr = (volatile const uint32 *)(base + offset);
    return *addr;
}

static void reg_write(uint32 base, uint32 offset, uint32 value)
{
    /* cppcheck-suppress misra-c2012-11.4 */
    volatile uint32 *addr = (volatile uint32 *)(base + offset);
    *addr = value;
}

/* ==================================================================
 * HALCoGen system stubs (from sc_main.c)
 *
 * systemInit() is called by HALCoGen startup code before main().
 * We provide a version that delegates to HALCoGen's _system_init()
 * or is a no-op if HALCoGen startup handles it.
 * ================================================================== */

/**
 * @brief  Initialize system clocks (PLL, flash, peripherals)
 *
 * On TMS570, HALCoGen's sys_startup.c calls systemInit() during
 * C runtime init — before main(). This function is already linked
 * from HALCoGen-generated system.c. We do NOT redefine it here;
 * the HALCoGen version is used directly.
 *
 * This is a placeholder comment — the symbol comes from
 * halcogencfg/source/system.c at link time.
 */
/* systemInit() — provided by HALCoGen system.c, not defined here */

/**
 * @brief  Initialize GIO module
 *
 * HALCoGen gioInit() configures pin directions and pull-ups per the
 * .hcg project settings. Called from main() before sc_configure_gpio().
 *
 * This is provided by HALCoGen gio.c at link time.
 */
/* gioInit() — provided by HALCoGen gio.c, not defined here */

/**
 * @brief  Initialize RTI timer for 10ms tick
 *
 * HALCoGen rtiInit() configures compare 0 for 10ms period.
 *
 * This is provided by HALCoGen rti.c at link time.
 */
/* rtiInit() — provided by HALCoGen rti.c, not defined here */

/**
 * @brief  Start RTI counter block 0
 *
 * Provided by HALCoGen rti.c. Writes RTI_GCTRL to enable counter.
 */
/* rtiStartCounter() — provided by HALCoGen rti.c, not defined here */

/* ==================================================================
 * GIO pin access (from sc_gio.h / sc_main.c)
 * ================================================================== */

/**
 * @brief  Set GIO pin direction
 * @param  port       Port number (0=A, 1=B)
 * @param  pin        Pin number within port (0-7)
 * @param  direction  0=input, 1=output
 */
void gioSetDirection(uint8 port, uint8 pin, uint8 direction)
{
    uint32 dir_offset;
    uint32 dir_val;

    if (pin > 7u) {
        return;
    }

    if (port == 0u) {
        dir_offset = GIO_DIRA;
    } else if (port == 1u) {
        dir_offset = GIO_DIRB;
    } else {
        return;
    }

    dir_val = reg_read(GIO_BASE, dir_offset);

    if (direction != 0u) {
        dir_val |= ((uint32)1u << (uint32)pin);
    } else {
        dir_val &= ~((uint32)1u << (uint32)pin);
    }

    reg_write(GIO_BASE, dir_offset, dir_val);
}

/**
 * @brief  Set GIO pin value
 * @param  port   Port number (0=A, 1=B)
 * @param  pin    Pin number within port (0-7)
 * @param  value  0=low, 1=high
 */
void gioSetBit(uint8 port, uint8 pin, uint8 value)
{
    uint32 set_offset;
    uint32 clr_offset;

    if (pin > 7u) {
        return;
    }

    if (port == 0u) {
        set_offset = GIO_DSETA;
        clr_offset = GIO_DCLRA;
    } else if (port == 1u) {
        set_offset = GIO_DSETB;
        clr_offset = GIO_DCLRB;
    } else {
        return;
    }

    if (value != 0u) {
        reg_write(GIO_BASE, set_offset, ((uint32)1u << (uint32)pin));
    } else {
        reg_write(GIO_BASE, clr_offset, ((uint32)1u << (uint32)pin));
    }
}

/**
 * @brief  Get GIO pin value (readback from DIN register)
 * @param  port  Port number (0=A, 1=B)
 * @param  pin   Pin number within port (0-7)
 * @return Pin value (0 or 1)
 */
uint8 gioGetBit(uint8 port, uint8 pin)
{
    uint32 din_offset;
    uint32 din_val;

    if (pin > 7u) {
        return 0u;
    }

    if (port == 0u) {
        din_offset = GIO_DINA;
    } else if (port == 1u) {
        din_offset = GIO_DINB;
    } else {
        return 0u;
    }

    din_val = reg_read(GIO_BASE, din_offset);

    return ((din_val >> (uint32)pin) & 1u) != 0u ? 1u : 0u;
}

/* ==================================================================
 * RTI tick checking (from sc_main.c)
 * ================================================================== */

/**
 * @brief  Check if RTI compare 0 flag is set (10ms elapsed)
 * @return TRUE if 10ms has elapsed since last clear
 */
boolean rtiIsTickPending(void)
{
    uint32 flags = reg_read(RTI_BASE, RTI_INTFLAG);
    return ((flags & RTI_CMP0_FLAG) != 0u) ? TRUE : FALSE;
}

/**
 * @brief  Clear RTI compare 0 interrupt flag
 */
void rtiClearTick(void)
{
    reg_write(RTI_BASE, RTI_CLEARINT, RTI_CMP0_FLAG);
}

/* ==================================================================
 * DCAN1 register access (from sc_can.c)
 * ================================================================== */

/**
 * @brief  Read DCAN1 register
 * @param  offset  Register offset from DCAN1 base
 * @return Register value
 */
uint32 dcan1_reg_read(uint32 offset)
{
    return reg_read(DCAN1_BASE, offset);
}

/**
 * @brief  Write DCAN1 register
 * @param  offset  Register offset from DCAN1 base
 * @param  value   Value to write
 */
void dcan1_reg_write(uint32 offset, uint32 value)
{
    reg_write(DCAN1_BASE, offset, value);
}

/**
 * @brief  Wait for DCAN IF2 to be ready (not busy)
 *
 * DCAN IF registers have a busy flag while transferring data
 * between the CPU interface and message object RAM.
 */
static void dcan1_wait_if2_ready(void)
{
    volatile uint32 timeout = 10000u;
    while (((reg_read(DCAN1_BASE, DCAN_IF2CMD) & DCAN_IFCMD_BUSY) != 0u) &&
           (timeout > 0u)) {
        timeout--;
    }
}

/**
 * @brief  Read CAN data from DCAN1 message object
 *
 * Uses DCAN IF2 interface registers to read from the message object
 * RAM. Checks NewDat bit to determine if new data is available.
 * Clears NewDat after successful read.
 *
 * @param  mbIndex  Mailbox index (0-based, 0-5)
 * @param  data     Output buffer (minimum 8 bytes)
 * @param  dlc      Output: data length code
 * @return TRUE if valid new data available, FALSE otherwise
 */
boolean dcan1_get_mailbox_data(uint8 mbIndex, uint8* data, uint8* dlc)
{
    uint32 msg_num;
    uint32 mctl;
    uint32 data_a;
    uint32 data_b;
    uint8 msg_dlc;

    if (mbIndex >= SC_MB_COUNT) {
        return FALSE;
    }
    if ((data == NULL_PTR) || (dlc == NULL_PTR)) {
        return FALSE;
    }

    /* Message objects are 1-indexed in DCAN hardware */
    msg_num = (uint32)mbIndex + 1u;

    /* Wait for IF2 to be available */
    dcan1_wait_if2_ready();

    /* Request transfer from message object to IF2 registers:
     * Read data A + data B + control (includes NewDat, DLC) */
    reg_write(DCAN1_BASE, DCAN_IF2CMD,
              DCAN_IFCMD_DATAA | DCAN_IFCMD_DATAB |
              DCAN_IFCMD_CONTROL | DCAN_IFCMD_NEWDAT |
              (msg_num & 0xFFu));  /* Message number, WR=0 (read) */

    /* Wait for transfer to complete */
    dcan1_wait_if2_ready();

    /* Check NewDat bit in MCTL */
    mctl = reg_read(DCAN1_BASE, DCAN_IF2MCTL);
    if ((mctl & DCAN_MCTL_NEWDAT) == 0u) {
        return FALSE;  /* No new data */
    }

    /* Extract DLC (bits 3:0) */
    msg_dlc = (uint8)(mctl & 0x0Fu);
    if (msg_dlc > 8u) {
        msg_dlc = 8u;
    }

    /* Read data registers — DCAN stores data in big-endian word order:
     * DATA register: byte0 in bits 7:0, byte1 in 15:8, byte2 in 23:16, byte3 in 31:24
     * DATB register: byte4 in bits 7:0, byte5 in 15:8, byte6 in 23:16, byte7 in 31:24 */
    data_a = reg_read(DCAN1_BASE, DCAN_IF2DATA);
    data_b = reg_read(DCAN1_BASE, DCAN_IF2DATB);

    data[0] = (uint8)(data_a & 0xFFu);
    data[1] = (uint8)((data_a >> 8u) & 0xFFu);
    data[2] = (uint8)((data_a >> 16u) & 0xFFu);
    data[3] = (uint8)((data_a >> 24u) & 0xFFu);
    data[4] = (uint8)(data_b & 0xFFu);
    data[5] = (uint8)((data_b >> 8u) & 0xFFu);
    data[6] = (uint8)((data_b >> 16u) & 0xFFu);
    data[7] = (uint8)((data_b >> 24u) & 0xFFu);

    *dlc = msg_dlc;

    /* Clear NewDat by writing back MCTL with NewDat=0 via IF2
     * (write CONTROL + NEWDAT flags to clear it in message object) */
    dcan1_wait_if2_ready();
    reg_write(DCAN1_BASE, DCAN_IF2MCTL, mctl & ~DCAN_MCTL_NEWDAT);
    reg_write(DCAN1_BASE, DCAN_IF2CMD,
              DCAN_IFCMD_WR | DCAN_IFCMD_CONTROL | DCAN_IFCMD_NEWDAT |
              (msg_num & 0xFFu));
    dcan1_wait_if2_ready();

    return TRUE;
}

/* ==================================================================
 * ESM (Error Signaling Module) — from sc_esm.c
 * ================================================================== */

/**
 * @brief  Enable ESM group 1 channel
 * @param  channel  ESM channel number (0-31)
 */
void esm_enable_group1_channel(uint8 channel)
{
    if (channel > 31u) {
        return;
    }
    reg_write(ESM_BASE, ESM_EEPAPR1, ((uint32)1u << (uint32)channel));
}

/**
 * @brief  Clear ESM flag
 * @param  group    ESM group (1 or 2)
 * @param  channel  ESM channel (0-31)
 */
void esm_clear_flag(uint8 group, uint8 channel)
{
    if (channel > 31u) {
        return;
    }

    if (group == 1u) {
        /* Write 1 to clear the flag in group 1 status register */
        reg_write(ESM_BASE, ESM_SR4, ((uint32)1u << (uint32)channel));
    } else {
        /* Group 2: no direct clear on some TMS570 variants —
         * cleared by ESM key register or hardware reset */
        (void)group;
    }
}

/**
 * @brief  Check if ESM flag is set
 * @param  group    ESM group (1 or 2)
 * @param  channel  ESM channel (0-31)
 * @return TRUE if the ESM flag is set, FALSE otherwise
 */
boolean esm_is_flag_set(uint8 group, uint8 channel)
{
    uint32 status;

    if (channel > 31u) {
        return FALSE;
    }

    if (group == 1u) {
        status = reg_read(ESM_BASE, ESM_SR1);
    } else if (group == 2u) {
        status = reg_read(ESM_BASE, ESM_SR2);
    } else {
        return FALSE;
    }

    return ((status & ((uint32)1u << (uint32)channel)) != 0u) ? TRUE : FALSE;
}

/* ==================================================================
 * Self-test hardware functions (from sc_selftest.c)
 *
 * Phase 1: All return TRUE (stubs) for bring-up.
 * Phase 2 (after CAN works): Implement real DCAN loopback,
 *          GPIO readback, lamp test.
 * Phase 3 (after validation): Implement STC, PBIST, flash CRC.
 * ================================================================== */

/**
 * @brief  Lockstep CPU BIST via STC module
 * @return TRUE (stub — real STC implementation deferred)
 * @note   TODO:HARDWARE — implement STC self-test
 */
boolean hw_lockstep_bist(void)
{
    /* TODO:HARDWARE — STC (Self-Test Controller) runs lockstep
     * compare test. Requires careful sequencing and takes ~100ms.
     * Deferred until after CAN bring-up. */
    return TRUE;
}

/**
 * @brief  RAM PBIST (Programmable Built-In Self-Test)
 * @return TRUE (stub — real PBIST implementation deferred)
 * @note   TODO:HARDWARE — implement PBIST
 */
boolean hw_ram_pbist(void)
{
    /* TODO:HARDWARE — PBIST runs hardware pattern generator on RAM.
     * Must save/restore tested region. Deferred. */
    return TRUE;
}

/**
 * @brief  Flash CRC integrity check via CRC module
 * @return TRUE (stub — real flash CRC implementation deferred)
 * @note   TODO:HARDWARE — implement flash CRC check
 */
boolean hw_flash_crc_check(void)
{
    /* TODO:HARDWARE — use TMS570 CRC module (MCRC) to verify
     * flash integrity against golden CRC stored at link time.
     * Deferred. */
    return TRUE;
}

/**
 * @brief  DCAN1 internal loopback test
 * @return TRUE (stub — implement after CAN mailbox config verified)
 * @note   TODO:HARDWARE — implement DCAN loopback test (priority 1)
 */
boolean hw_dcan_loopback_test(void)
{
    /* TODO:HARDWARE — Put DCAN1 in internal loopback mode (TEST.Lback),
     * transmit a frame, verify reception on another mailbox.
     * This is the highest-priority real self-test. */
    return TRUE;
}

/**
 * @brief  GPIO readback test — write output, read back DIN
 * @return TRUE (stub)
 * @note   TODO:HARDWARE — implement GPIO readback (priority 2)
 */
boolean hw_gpio_readback_test(void)
{
    /* TODO:HARDWARE — Write relay pin HIGH, read back DIN register,
     * verify matches. Then restore original value. */
    return TRUE;
}

/**
 * @brief  LED lamp test — all LEDs ON, brief delay, all OFF
 * @return TRUE (stub)
 * @note   TODO:HARDWARE — implement lamp test (priority 3)
 */
boolean hw_lamp_test(void)
{
    /* TODO:HARDWARE — Set all LED pins HIGH, ~200ms delay (busy-wait),
     * set all LOW. Visual check by operator. Always returns TRUE
     * (lamp test is visual, not electrical). */
    return TRUE;
}

/**
 * @brief  Watchdog (TPS3823) toggle test
 * @return TRUE (stub)
 * @note   TODO:HARDWARE — implement watchdog test
 */
boolean hw_watchdog_test(void)
{
    /* TODO:HARDWARE — Toggle WDI pin a few times, verify no reset
     * occurs within the 1.6s TPS3823 timeout. */
    return TRUE;
}

/**
 * @brief  Flash CRC incremental (runtime) — one sector per call
 * @return TRUE (stub)
 * @note   TODO:HARDWARE — implement incremental flash CRC
 */
boolean hw_flash_crc_incremental(void)
{
    /* TODO:HARDWARE — CRC one flash sector using MCRC module.
     * Called once per 60s runtime self-test cycle. */
    return TRUE;
}

/**
 * @brief  DCAN error check (runtime) — read ES register
 * @return TRUE if no critical errors, FALSE if bus-off or error passive
 */
boolean hw_dcan_error_check(void)
{
    uint32 es = reg_read(DCAN1_BASE, 0x04u);  /* DCAN_ES */

    /* Check bus-off (bit 7) and error passive (bit 5) */
    if ((es & 0xA0u) != 0u) {
        return FALSE;
    }
    return TRUE;
}

/* ==================================================================
 * SCI Debug UART
 *
 * SCI1 is connected to XDS110 virtual COM port on the LaunchPad.
 * Used for boot diagnostics only — not safety-relevant.
 * ================================================================== */

/**
 * @brief  Initialize SCI1 for 115200 baud debug output
 *
 * HALCoGen should configure SCI1 in the project. This function
 * provides a fallback bare-metal init if needed.
 *
 * Baud rate calculation: VCLK1 / (16 * (BRS + 1))
 * At 75 MHz VCLK1: 75000000 / (16 * 115200) = 40.69 → BRS = 40
 * Actual baud = 75000000 / (16 * 41) = 114329 (0.76% error — OK)
 */
void sc_sci_init(void)
{
    /* Reset SCI */
    reg_write(SCI_BASE, SCI_GCR0, 0u);
    reg_write(SCI_BASE, SCI_GCR0, 1u);  /* Release from reset */

    /* Configure: 1 stop bit, no parity, 8-bit char, async mode */
    reg_write(SCI_BASE, SCI_GCR1,
              ((uint32)1u << 25u) |   /* CLOCK (internal) */
              ((uint32)1u << 24u) |   /* SWnRST (out of reset) */
              ((uint32)1u << 5u)  |   /* TXENA */
              ((uint32)1u << 1u));    /* TIMING (async) */

    /* Baud rate: 75 MHz / (16 * 41) = ~114329 baud */
    reg_write(SCI_BASE, SCI_BRS, 40u);

    /* Re-enable TX after config */
    {
        uint32 gcr1 = reg_read(SCI_BASE, SCI_GCR1);
        gcr1 |= ((uint32)1u << 25u) | ((uint32)1u << 24u) | ((uint32)1u << 5u) | ((uint32)1u << 1u);
        reg_write(SCI_BASE, SCI_GCR1, gcr1);
    }
}

/**
 * @brief  Send a single byte over SCI1
 * @param  ch  Character to transmit
 */
void sc_sci_putchar(uint8 ch)
{
    /* Wait for TX ready */
    while ((reg_read(SCI_BASE, SCI_FLR) & SCI_FLR_TXRDY) == 0u) {
        /* Busy wait */
    }
    reg_write(SCI_BASE, SCI_TD, (uint32)ch);
}

/**
 * @brief  Send a null-terminated string over SCI1
 * @param  str  String to transmit
 */
void sc_sci_puts(const char* str)
{
    if (str == NULL_PTR) {
        return;
    }
    while (*str != '\0') {
        sc_sci_putchar((uint8)*str);
        str++;
    }
}

/**
 * @brief  Send a uint32 as decimal string over SCI1
 * @param  val  Value to print
 */
void sc_sci_put_uint(uint32 val)
{
    char buf[11];  /* max 10 digits + null */
    uint8 i = 0u;
    uint8 j;

    if (val == 0u) {
        sc_sci_putchar((uint8)'0');
        return;
    }

    while (val > 0u) {
        buf[i] = (char)((uint8)'0' + (uint8)(val % 10u));
        val /= 10u;
        i++;
    }

    /* Print in reverse order */
    for (j = i; j > 0u; j--) {
        sc_sci_putchar((uint8)buf[j - 1u]);
    }
}

#endif /* PLATFORM_TMS570 */
