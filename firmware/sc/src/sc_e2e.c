/**
 * @file    sc_e2e.c
 * @brief   E2E CRC-8 validation for Safety Controller CAN messages
 * @date    2026-02-23
 *
 * @safety_req SWR-SC-003
 * @traces_to  SSR-SC-003
 * @note    Safety level: ASIL D
 * @standard ISO 26262 Part 6, AUTOSAR E2E Profile P01
 * @copyright Taktflow Systems 2026
 */
#include "sc_e2e.h"
#include "sc_cfg.h"

/* ==================================================================
 * Module State
 * ================================================================== */

/** Per-message E2E state: last alive counter received */
static uint8 e2e_last_alive[SC_MB_COUNT];

/** Per-message E2E state: first message flag (no alive check on first) */
static boolean e2e_first_rx[SC_MB_COUNT];

/** Per-message consecutive failure counter */
static uint8 e2e_fail_count[SC_MB_COUNT];

/** Per-message persistent failure flag */
static boolean e2e_failed[SC_MB_COUNT];

/* ==================================================================
 * Internal: CRC-8 Computation (SAE-J1850, poly 0x1D, init 0xFF)
 * ================================================================== */

/**
 * @brief  Compute CRC-8 using bit-by-bit method
 *
 * No lookup table — keeps code size minimal for SC (~400 LOC target).
 * Constant-time per byte (no data-dependent early exits).
 *
 * @param  data  Pointer to data bytes (may be NULL if len == 0)
 * @param  len   Number of bytes
 * @return Computed CRC-8 value
 */
static uint8 sc_crc8(const uint8* data, uint8 len)
{
    uint8 crc = SC_CRC8_INIT;
    uint8 i;
    uint8 j;

    for (i = 0u; i < len; i++) {
        crc ^= data[i];
        for (j = 0u; j < 8u; j++) {
            if ((crc & 0x80u) != 0u) {
                crc = (uint8)((crc << 1u) ^ SC_CRC8_POLY);
            } else {
                crc = (uint8)(crc << 1u);
            }
        }
    }

    return crc;
}

/* ==================================================================
 * Public API
 * ================================================================== */

void SC_E2E_Init(void)
{
    uint8 i;
    for (i = 0u; i < SC_MB_COUNT; i++) {
        e2e_last_alive[i] = 0u;
        e2e_first_rx[i]   = TRUE;
        e2e_fail_count[i] = 0u;
        e2e_failed[i]     = FALSE;
    }
}

boolean SC_E2E_Check(const uint8* data, uint8 dlc, uint8 dataId,
                     uint8 msgIndex)
{
    uint8 crc_input[7];
    uint8 expected_crc;
    uint8 received_crc;
    uint8 alive;
    uint8 expected_alive;
    boolean valid = TRUE;
    uint8 i;
    uint8 payload_len;

    if ((data == NULL_PTR) || (msgIndex >= SC_MB_COUNT) || (dlc < 2u)) {
        return FALSE;
    }

    /* Extract alive counter from byte 0 upper nibble */
    alive = (uint8)((data[0] >> 4u) & 0x0Fu);

    /* Extract received CRC from byte 1 */
    received_crc = data[1];

    /* Build CRC input: Data ID byte + payload bytes 2..DLC-1 */
    crc_input[0] = dataId;
    payload_len = (dlc > 2u) ? (uint8)(dlc - 2u) : 0u;
    if (payload_len > 6u) {
        payload_len = 6u;  /* Cap at 6 payload bytes for 8-byte CAN */
    }
    for (i = 0u; i < payload_len; i++) {
        crc_input[1u + i] = data[2u + i];
    }

    /* Compute expected CRC */
    expected_crc = sc_crc8(crc_input, (uint8)(1u + payload_len));

    /* Verify CRC */
    if (expected_crc != received_crc) {
        valid = FALSE;
    }

    /* Verify alive counter (skip on first reception) */
    if ((valid == TRUE) && (e2e_first_rx[msgIndex] == FALSE)) {
        expected_alive = (uint8)((e2e_last_alive[msgIndex] + 1u) & 0x0Fu);
        if (alive != expected_alive) {
            valid = FALSE;
        }
    }

    /* Update state */
    if (valid == TRUE) {
        e2e_last_alive[msgIndex] = alive;
        e2e_first_rx[msgIndex]   = FALSE;
        e2e_fail_count[msgIndex] = 0u;
    } else {
        e2e_fail_count[msgIndex]++;
        if (e2e_fail_count[msgIndex] >= SC_E2E_MAX_CONSEC_FAIL) {
            e2e_failed[msgIndex] = TRUE;
        }
    }

    return valid;
}

boolean SC_E2E_IsMsgFailed(uint8 msgIndex)
{
    if (msgIndex >= SC_MB_COUNT) {
        return TRUE;  /* Invalid index = fail-closed */
    }
    return e2e_failed[msgIndex];
}
