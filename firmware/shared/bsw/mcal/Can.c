/**
 * @file    Can.c
 * @brief   CAN MCAL driver implementation
 * @date    2026-02-21
 *
 * @details Platform-independent CAN driver logic. Hardware access is
 *          abstracted through Can_Hw_* functions (implemented per platform).
 *
 * @safety_req SWR-BSW-001, SWR-BSW-002, SWR-BSW-003, SWR-BSW-004, SWR-BSW-005
 * @traces_to  TSR-022, TSR-023, TSR-024, TSR-038, TSR-039
 *
 * @standard AUTOSAR_SWS_CANDriver, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#include "Can.h"

/* ---- Internal State ---- */

static Can_StateType can_state = CAN_CS_UNINIT;
static uint8         can_controller_id = 0u;
static boolean       can_bus_off_active = FALSE;

/* ---- API Implementation ---- */

void Can_Init(const Can_ConfigType* ConfigPtr)
{
    if (ConfigPtr == NULL_PTR) {
        can_state = CAN_CS_UNINIT;
        return;
    }

    can_controller_id = ConfigPtr->controllerId;

    if (Can_Hw_Init(ConfigPtr->baudrate) != E_OK) {
        can_state = CAN_CS_UNINIT;
        return;
    }

    can_bus_off_active = FALSE;
    can_state = CAN_CS_STOPPED;
}

void Can_DeInit(void)
{
    Can_Hw_Stop();
    can_state = CAN_CS_UNINIT;
    can_bus_off_active = FALSE;
}

Std_ReturnType Can_SetControllerMode(uint8 Controller, Can_StateType Mode)
{
    (void)Controller;

    if (can_state == CAN_CS_UNINIT) {
        return E_NOT_OK;
    }

    switch (Mode) {
    case CAN_CS_STARTED:
        if (can_state == CAN_CS_STOPPED) {
            Can_Hw_Start();
            can_state = CAN_CS_STARTED;
            return E_OK;
        }
        break;

    case CAN_CS_STOPPED:
        if (can_state == CAN_CS_STARTED) {
            Can_Hw_Stop();
            can_state = CAN_CS_STOPPED;
            return E_OK;
        }
        break;

    default:
        break;
    }

    return E_NOT_OK;
}

Can_StateType Can_GetControllerMode(uint8 Controller)
{
    (void)Controller;
    return can_state;
}

Can_ReturnType Can_Write(uint8 Hth, const Can_PduType* PduInfo)
{
    (void)Hth;

    /* Must be in STARTED mode */
    if (can_state != CAN_CS_STARTED) {
        return CAN_NOT_OK;
    }

    /* Validate parameters */
    if (PduInfo == NULL_PTR) {
        return CAN_NOT_OK;
    }

    if (PduInfo->length > CAN_MAX_DLC) {
        return CAN_NOT_OK;
    }

    if ((PduInfo->sdu == NULL_PTR) && (PduInfo->length > 0u)) {
        return CAN_NOT_OK;
    }

    /* Attempt hardware transmit */
    Std_ReturnType hw_ret = Can_Hw_Transmit(PduInfo->id, PduInfo->sdu, PduInfo->length);

    if (hw_ret != E_OK) {
        return CAN_BUSY;
    }

    return CAN_OK;
}

void Can_MainFunction_Read(void)
{
    Can_IdType rx_id;
    uint8      rx_data[CAN_MAX_DLC];
    uint8      rx_dlc;
    uint8      msg_count = 0u;

    if (can_state != CAN_CS_STARTED) {
        return;
    }

    /* Process all pending messages, up to CAN_MAX_RX_PER_CALL */
    while ((msg_count < CAN_MAX_RX_PER_CALL) &&
           (Can_Hw_Receive(&rx_id, rx_data, &rx_dlc) == TRUE)) {
        CanIf_RxIndication(rx_id, rx_data, rx_dlc);
        msg_count++;
    }
}

void Can_MainFunction_BusOff(void)
{
    if (can_state != CAN_CS_STARTED) {
        return;
    }

    if (Can_Hw_IsBusOff() == TRUE) {
        if (can_bus_off_active == FALSE) {
            can_bus_off_active = TRUE;
            CanIf_ControllerBusOff(can_controller_id);
        }
    } else {
        if (can_bus_off_active == TRUE) {
            can_bus_off_active = FALSE;
            /* Recovery complete — controller remains in STARTED */
        }
    }
}

Std_ReturnType Can_GetErrorCounters(uint8 Controller, uint8* tec, uint8* rec)
{
    (void)Controller;

    if ((tec == NULL_PTR) || (rec == NULL_PTR)) {
        return E_NOT_OK;
    }

    Can_Hw_GetErrorCounters(tec, rec);
    return E_OK;
}
