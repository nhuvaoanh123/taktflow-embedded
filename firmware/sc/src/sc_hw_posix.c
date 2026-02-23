/**
 * @file    sc_hw_posix.c
 * @brief   POSIX hardware stubs for SC (Safety Controller) SIL simulation
 * @date    2026-02-23
 *
 * @details Implements all hardware externs used across SC source files:
 *          - HALCoGen system/GIO/RTI stubs (from sc_main.c)
 *          - DCAN1 register stubs with SocketCAN backend (from sc_can.c)
 *          - Self-test hardware stubs (from sc_selftest.c)
 *          - ESM error signaling stubs (from sc_esm.c)
 *
 *          The SC is a TMS570 bare-metal controller — no AUTOSAR BSW.
 *          It uses its own sc_types.h for type definitions.
 *
 *          The DCAN1 mailbox→CAN ID mapping enables the SC to receive
 *          CAN frames from vcan0 via SocketCAN, filtered to match the
 *          6 mailboxes defined in sc_cfg.h.
 *
 * @safety_req N/A — SIL simulation only, not for production
 * @copyright Taktflow Systems 2026
 */

#include "sc_types.h"
#include "sc_cfg.h"

#ifndef PLATFORM_POSIX_TEST
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#endif

/* ==================================================================
 * Module state
 * ================================================================== */

/** GIO pin state array (2 ports x 8 pins) */
static uint8 gio_pin_state[2u][8u];
static uint8 gio_pin_dir[2u][8u];

/** RTI tick tracking */
#ifndef PLATFORM_POSIX_TEST
static struct timespec rti_last_tick;
#endif
static boolean rti_running = FALSE;

/** SocketCAN file descriptor for DCAN1 simulation */
static int dcan_fd = -1;

/** DCAN init tracking */
static boolean dcan_initialized = FALSE;

/** Mailbox → CAN ID mapping (SC receives on these 6 IDs) */
static const uint32 mb_can_id[6u] = {
    SC_CAN_ID_ESTOP,           /* MB0 = 0x001 */
    SC_CAN_ID_CVC_HB,          /* MB1 = 0x010 */
    SC_CAN_ID_FZC_HB,          /* MB2 = 0x011 */
    SC_CAN_ID_RZC_HB,          /* MB3 = 0x012 */
    SC_CAN_ID_VEHICLE_STATE,   /* MB4 = 0x100 */
    SC_CAN_ID_MOTOR_CURRENT    /* MB5 = 0x301 */
};

/* ==================================================================
 * HALCoGen system stubs (from sc_main.c:35-56)
 * ================================================================== */

/**
 * @brief  Initialize system clocks (PLL to 300 MHz) — no-op on POSIX
 */
void systemInit(void)
{
    /* POSIX: no PLL configuration needed */
}

/**
 * @brief  Initialize GIO module — zero all pin states
 */
void gioInit(void)
{
    uint8 p;
    uint8 i;
    for (p = 0u; p < 2u; p++) {
        for (i = 0u; i < 8u; i++) {
            gio_pin_state[p][i] = 0u;
            gio_pin_dir[p][i]   = 0u;
        }
    }
}

/**
 * @brief  Initialize RTI timer for 10ms tick — record start time
 */
void rtiInit(void)
{
#ifndef PLATFORM_POSIX_TEST
    clock_gettime(CLOCK_MONOTONIC, &rti_last_tick);
#endif
}

/**
 * @brief  Start RTI counter
 */
void rtiStartCounter(void)
{
    rti_running = TRUE;
#ifndef PLATFORM_POSIX_TEST
    clock_gettime(CLOCK_MONOTONIC, &rti_last_tick);
#endif
}

/**
 * @brief  Check if RTI tick flag is set (10ms elapsed)
 * @return TRUE if 10ms has elapsed since last clear
 */
boolean rtiIsTickPending(void)
{
    if (rti_running == FALSE) {
        return FALSE;
    }

#ifndef PLATFORM_POSIX_TEST
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    uint32 elapsed_us = (uint32)(
        ((now.tv_sec - rti_last_tick.tv_sec) * 1000000u) +
        ((now.tv_nsec - rti_last_tick.tv_nsec) / 1000u)
    );

    if (elapsed_us >= 10000u) {  /* 10ms = 10000us */
        return TRUE;
    }
#endif

    return FALSE;
}

/**
 * @brief  Clear RTI tick flag — update last-tick timestamp
 */
void rtiClearTick(void)
{
#ifndef PLATFORM_POSIX_TEST
    clock_gettime(CLOCK_MONOTONIC, &rti_last_tick);
#endif
}

/**
 * @brief  Set GIO pin direction
 * @param  port       Port number (0=A, 1=B)
 * @param  pin        Pin number within port
 * @param  direction  0=input, 1=output
 */
void gioSetDirection(uint8 port, uint8 pin, uint8 direction)
{
    if ((port < 2u) && (pin < 8u)) {
        gio_pin_dir[port][pin] = direction;
    }
}

/**
 * @brief  Set GIO pin value
 * @param  port   Port number (0=A, 1=B)
 * @param  pin    Pin number within port
 * @param  value  0=low, 1=high
 */
void gioSetBit(uint8 port, uint8 pin, uint8 value)
{
    if ((port < 2u) && (pin < 8u)) {
        gio_pin_state[port][pin] = value;
    }
}

/**
 * @brief  Get GIO pin value (readback)
 * @param  port  Port number (0=A, 1=B)
 * @param  pin   Pin number within port
 * @return Pin value (0 or 1)
 */
uint8 gioGetBit(uint8 port, uint8 pin)
{
    if ((port < 2u) && (pin < 8u)) {
        return gio_pin_state[port][pin];
    }
    return 0u;
}

/* ==================================================================
 * DCAN1 register stubs with SocketCAN backend (from sc_can.c:20-22)
 * ================================================================== */

/**
 * @brief  Helper: initialize SocketCAN socket for SC
 */
static void sc_posix_can_init(void)
{
#ifndef PLATFORM_POSIX_TEST
    if (dcan_fd >= 0) {
        return; /* Already initialized */
    }

    const char* iface = getenv("CAN_INTERFACE");
    if (iface == NULL) {
        iface = "vcan0";
    }

    int fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd < 0) {
        return;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, sizeof(ifr.ifr_name) - 1u);
    ifr.ifr_name[sizeof(ifr.ifr_name) - 1u] = '\0';

    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
        close(fd);
        return;
    }

    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return;
    }

    /* Set non-blocking mode */
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    dcan_fd = fd;
#endif
}

/**
 * @brief  Read DCAN1 register (POSIX: return simulated values)
 * @param  offset  Register offset
 * @return Register value (0 = no errors for error status)
 */
uint32 dcan1_reg_read(uint32 offset)
{
    /* DCAN_ES_OFFSET: return 0 = no errors, no bus-off */
    (void)offset;
    return 0u;
}

/**
 * @brief  Write DCAN1 register (POSIX: track init state)
 * @param  offset  Register offset
 * @param  value   Value to write
 */
void dcan1_reg_write(uint32 offset, uint32 value)
{
    (void)offset;
    (void)value;

    /* On exit-init (CTL offset write with value 0x00), open SocketCAN */
    if ((offset == 0x00u) && (value == 0x00u)) {
        dcan_initialized = TRUE;
        sc_posix_can_init();
    }
}

/**
 * @brief  Read CAN data from mailbox (POSIX: SocketCAN non-blocking read)
 *
 * Reads frames from SocketCAN, filters by CAN ID using the SC mailbox
 * mapping. If the received CAN ID matches the requested mailbox, copies
 * data and returns TRUE.
 *
 * @param  mbIndex  Mailbox index (0-based, 0-5)
 * @param  data     Output buffer (minimum 8 bytes)
 * @param  dlc      Output: data length code
 * @return TRUE if valid data available for this mailbox, FALSE otherwise
 */
boolean dcan1_get_mailbox_data(uint8 mbIndex, uint8* data, uint8* dlc)
{
#ifndef PLATFORM_POSIX_TEST
    if (dcan_fd < 0) {
        return FALSE;
    }
    if (mbIndex >= 6u) {
        return FALSE;
    }
    if ((data == NULL) || (dlc == NULL)) {
        return FALSE;
    }

    /* Read available frames (non-blocking), looking for our mailbox's CAN ID */
    struct can_frame frame;
    uint32 target_id = mb_can_id[mbIndex];
    int max_reads = 32; /* Limit reads per call to prevent starvation */

    while (max_reads > 0) {
        max_reads--;
        ssize_t nbytes = recv(dcan_fd, &frame, sizeof(frame), MSG_DONTWAIT | MSG_PEEK);
        if (nbytes <= 0) {
            return FALSE; /* No more frames available */
        }

        /* Consume the frame */
        nbytes = recv(dcan_fd, &frame, sizeof(frame), MSG_DONTWAIT);
        if (nbytes <= 0) {
            return FALSE;
        }

        /* Check if this frame matches our mailbox's CAN ID */
        uint32 rx_id = frame.can_id & 0x7FFu; /* Mask to 11-bit standard */
        if (rx_id == target_id) {
            uint8 i;
            uint8 rx_dlc = frame.can_dlc;
            if (rx_dlc > 8u) {
                rx_dlc = 8u;
            }
            for (i = 0u; i < rx_dlc; i++) {
                data[i] = frame.data[i];
            }
            *dlc = rx_dlc;
            return TRUE;
        }
        /* Frame didn't match this mailbox — continue reading */
    }
#else
    (void)mbIndex;
    (void)data;
    (void)dlc;
#endif

    return FALSE;
}

/* ==================================================================
 * Self-test hardware stubs (from sc_selftest.c:20-30)
 * ================================================================== */

/**
 * @brief  Lockstep CPU BIST — always passes in SIL
 * @return TRUE
 */
boolean hw_lockstep_bist(void)
{
    return TRUE;
}

/**
 * @brief  RAM pattern BIST — always passes in SIL
 * @return TRUE
 */
boolean hw_ram_pbist(void)
{
    return TRUE;
}

/**
 * @brief  Flash CRC integrity check — always passes in SIL
 * @return TRUE
 */
boolean hw_flash_crc_check(void)
{
    return TRUE;
}

/**
 * @brief  DCAN loopback test — always passes in SIL
 * @return TRUE
 */
boolean hw_dcan_loopback_test(void)
{
    return TRUE;
}

/**
 * @brief  GPIO readback test — always passes in SIL
 * @return TRUE
 */
boolean hw_gpio_readback_test(void)
{
    return TRUE;
}

/**
 * @brief  LED lamp test — always passes in SIL
 * @return TRUE
 */
boolean hw_lamp_test(void)
{
    return TRUE;
}

/**
 * @brief  Watchdog test — always passes in SIL
 * @return TRUE
 */
boolean hw_watchdog_test(void)
{
    return TRUE;
}

/**
 * @brief  Flash CRC incremental (runtime) — always passes in SIL
 * @return TRUE
 */
boolean hw_flash_crc_incremental(void)
{
    return TRUE;
}

/**
 * @brief  DCAN error check (runtime) — always passes in SIL
 * @return TRUE
 */
boolean hw_dcan_error_check(void)
{
    return TRUE;
}

/* ==================================================================
 * ESM (Error Signaling Module) stubs (from sc_esm.c:25-27)
 * ================================================================== */

/**
 * @brief  Enable ESM group 1 channel — no-op on POSIX
 * @param  channel  ESM channel number
 */
void esm_enable_group1_channel(uint8 channel)
{
    (void)channel;
}

/**
 * @brief  Clear ESM flag — no-op on POSIX
 * @param  group    ESM group (1 or 2)
 * @param  channel  ESM channel
 */
void esm_clear_flag(uint8 group, uint8 channel)
{
    (void)group;
    (void)channel;
}

/**
 * @brief  Check if ESM flag is set — always FALSE in SIL (no errors)
 * @param  group    ESM group (1 or 2)
 * @param  channel  ESM channel
 * @return FALSE (no ESM errors in simulation)
 */
boolean esm_is_flag_set(uint8 group, uint8 channel)
{
    (void)group;
    (void)channel;
    return FALSE;
}
