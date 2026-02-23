/**
 * @file    sc_e2e.h
 * @brief   E2E CRC-8 validation for Safety Controller CAN messages
 * @date    2026-02-23
 *
 * @details Recomputes CRC-8 (poly 0x1D, init 0xFF) over Data ID byte +
 *          payload bytes 2..DLC-1. Verifies alive counter increment.
 *          Tracks per-message consecutive E2E failure count.
 *
 * @safety_req SWR-SC-003
 * @traces_to  SSR-SC-003
 * @note    Safety level: ASIL D
 * @standard ISO 26262 Part 6, AUTOSAR E2E Profile P01
 * @copyright Taktflow Systems 2026
 */
#ifndef SC_E2E_H
#define SC_E2E_H

#include "sc_types.h"

/**
 * @brief  Initialize E2E module — reset all per-message states
 */
void SC_E2E_Init(void);

/**
 * @brief  Validate E2E protection on a received CAN message
 *
 * Recomputes CRC-8 over Data ID + payload bytes 2..DLC-1, compares with
 * byte 1. Extracts alive counter from byte 0 upper nibble, verifies
 * increment by exactly 1 (with wrap 15->0).
 *
 * @param  data      Pointer to CAN message data (8 bytes)
 * @param  dlc       Data length code
 * @param  dataId    E2E Data ID for this message type
 * @param  msgIndex  Mailbox index (0-based) for per-message state
 * @return TRUE if CRC and alive counter are valid, FALSE otherwise
 */
boolean SC_E2E_Check(const uint8* data, uint8 dlc, uint8 dataId,
                     uint8 msgIndex);

/**
 * @brief  Check if a message type has reached persistent E2E failure
 *
 * @param  msgIndex  Mailbox index (0-based)
 * @return TRUE if 3+ consecutive E2E failures on this message
 */
boolean SC_E2E_IsMsgFailed(uint8 msgIndex);

#endif /* SC_E2E_H */
