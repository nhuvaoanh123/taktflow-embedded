/**
 * @file    Can_Hw_STM32.c
 * @brief   STM32 hardware backend for CAN MCAL driver (stub — TODO:HARDWARE)
 * @date    2026-03-03
 *
 * @details Stub implementation for initial STM32 build. All functions return
 *          E_OK / minimal defaults. Replace with real STM32 HAL calls in
 *          Phase F2/F3 when hardware is available.
 *
 * @safety_req SWR-BSW-001: CAN initialization
 * @safety_req SWR-BSW-002: CAN transmit
 * @safety_req SWR-BSW-003: CAN receive processing
 * @safety_req SWR-BSW-004: Bus-off recovery
 * @safety_req SWR-BSW-005: Error reporting
 * @copyright Taktflow Systems 2026
 */

#include "Platform_Types.h"
#include "Std_Types.h"
#include "ComStack_Types.h"
#include "Can.h"

/* ---- Can_Hw_* stub implementations ---- */

/**
 * @brief  Initialize CAN hardware (STM32 stub: no-op)
 * @param  baudrate  Baudrate in bps (e.g. 500000)
 * @return E_OK always
 */
Std_ReturnType Can_Hw_Init(uint32 baudrate)
{
    (void)baudrate; /* TODO:HARDWARE — configure bxCAN peripheral with baudrate */
    return E_OK;
}

/**
 * @brief  Start CAN controller (STM32 stub: no-op)
 */
void Can_Hw_Start(void)
{
    /* TODO:HARDWARE — leave INIT mode, enter NORMAL mode on bxCAN */
}

/**
 * @brief  Stop CAN controller (STM32 stub: no-op)
 */
void Can_Hw_Stop(void)
{
    /* TODO:HARDWARE — enter INIT mode on bxCAN, disable interrupts */
}

/**
 * @brief  Transmit a CAN frame (STM32 stub: no-op, returns E_OK)
 * @param  id    CAN identifier (11-bit standard)
 * @param  data  Pointer to payload data
 * @param  dlc   Data length (0..8)
 * @return E_OK always
 */
Std_ReturnType Can_Hw_Transmit(Can_IdType id, const uint8* data, uint8 dlc)
{
    (void)id;   /* TODO:HARDWARE — load TX mailbox with id, data, dlc */
    (void)data;
    (void)dlc;
    return E_OK;
}

/**
 * @brief  Non-blocking receive of a CAN frame (STM32 stub: no data)
 * @param  id    Output: received CAN identifier
 * @param  data  Output: received payload (min 8 bytes)
 * @param  dlc   Output: data length
 * @return FALSE always (no hardware data available)
 */
boolean Can_Hw_Receive(Can_IdType* id, uint8* data, uint8* dlc)
{
    (void)id;   /* TODO:HARDWARE — read RX FIFO 0/1 from bxCAN */
    (void)data;
    (void)dlc;
    return FALSE;
}

/**
 * @brief  Check if CAN bus is in bus-off state (STM32 stub: never bus-off)
 * @return FALSE always
 */
boolean Can_Hw_IsBusOff(void)
{
    /* TODO:HARDWARE — read ESR register CAN_ESR_BOFF bit */
    return FALSE;
}

/**
 * @brief  Get CAN error counters (STM32 stub: always 0)
 * @param  tec  Output: transmit error counter
 * @param  rec  Output: receive error counter
 */
void Can_Hw_GetErrorCounters(uint8* tec, uint8* rec)
{
    /* TODO:HARDWARE — read TEC/REC from ESR register */
    if (tec != NULL_PTR) {
        *tec = 0u;
    }
    if (rec != NULL_PTR) {
        *rec = 0u;
    }
}
