/**
 * @file    Dcm.h
 * @brief   Diagnostic Communication Manager — UDS service dispatch
 * @date    2026-02-21
 *
 * @safety_req SWR-BSW-017
 * @traces_to  TSR-038, TSR-039, TSR-040
 *
 * @standard AUTOSAR_SWS_DiagnosticCommunicationManager, ISO 26262 Part 6
 * @copyright Taktflow Systems 2026
 */
#ifndef DCM_H
#define DCM_H

#include "Std_Types.h"
#include "ComStack_Types.h"

/* ---- Constants ---- */

#define DCM_MAX_DIDS            16u   /**< Maximum DID table entries        */
#define DCM_TX_BUF_SIZE          8u   /**< Max UDS response on CAN 2.0B    */
#define DCM_MAIN_CYCLE_MS       10u   /**< MainFunction call period in ms   */

/* UDS Service IDs */
#define DCM_SID_SESSION_CTRL    0x10u
#define DCM_SID_READ_DID        0x22u
#define DCM_SID_TESTER_PRESENT  0x3Eu

/* UDS Negative Response Code (NRC) values */
#define DCM_NRC_SERVICE_NOT_SUPPORTED          0x11u
#define DCM_NRC_INCORRECT_MSG_LENGTH           0x13u
#define DCM_NRC_REQUEST_OUT_OF_RANGE           0x31u

/* UDS Response SID offset */
#define DCM_POSITIVE_RESPONSE_OFFSET           0x40u
#define DCM_NEGATIVE_RESPONSE_SID              0x7Fu

/* Suppress positive response bit */
#define DCM_SUPPRESS_POS_RSP_BIT               0x80u

/* ---- Types ---- */

/** Diagnostic session type */
typedef enum {
    DCM_DEFAULT_SESSION  = 0x01u,
    DCM_EXTENDED_SESSION = 0x03u
} Dcm_SessionType;

/** DID read callback function pointer type */
typedef Std_ReturnType (*Dcm_DidReadFuncType)(uint8* Data, uint8 Length);

/** DID table entry (compile-time) */
typedef struct {
    uint16              Did;           /**< 16-bit DID identifier           */
    Dcm_DidReadFuncType ReadFunc;      /**< Callback to read DID data       */
    uint8               DataLength;    /**< DID data length in bytes        */
} Dcm_DidTableType;

/** Dcm module configuration */
typedef struct {
    const Dcm_DidTableType*  DidTable;       /**< DID table array           */
    uint8                    DidCount;       /**< Number of DIDs            */
    PduIdType                TxPduId;        /**< TX PDU ID for responses   */
    uint16                   S3TimeoutMs;    /**< S3 session timeout in ms  */
} Dcm_ConfigType;

/* ---- External Dependencies ---- */

extern Std_ReturnType PduR_DcmTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);

/* ---- API Functions ---- */

/**
 * @brief  Initialize DCM with configuration
 * @param  ConfigPtr  DID table and timing config (must not be NULL)
 */
void Dcm_Init(const Dcm_ConfigType* ConfigPtr);

/**
 * @brief  Cyclic main function — processes pending requests, manages S3 timer
 * @note   Call every DCM_MAIN_CYCLE_MS (10 ms)
 */
void Dcm_MainFunction(void);

/**
 * @brief  Receive indication from PduR — stores request for processing
 * @param  RxPduId     Received PDU ID
 * @param  PduInfoPtr  Received UDS request data
 */
void Dcm_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/**
 * @brief  Get current diagnostic session
 * @return Current Dcm_SessionType (DEFAULT or EXTENDED)
 */
Dcm_SessionType Dcm_GetCurrentSession(void);

#endif /* DCM_H */
