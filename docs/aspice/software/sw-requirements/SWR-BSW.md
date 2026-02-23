---
document_id: SWR-BSW
title: "Software Requirements — Shared BSW"
version: "1.0"
status: draft
aspice_process: SWE.1
date: 2026-02-21
---

## Human-in-the-Loop (HITL) Comment Lock

`HITL` means human-reviewer-owned comment content.

**Marker standard (code-friendly):**
- Markdown: `<!-- HITL-LOCK START:<id> -->` ... `<!-- HITL-LOCK END:<id> -->`
- C/C++/Java/JS/TS: `// HITL-LOCK START:<id>` ... `// HITL-LOCK END:<id>`
- Python/Shell/YAML/TOML: `# HITL-LOCK START:<id>` ... `# HITL-LOCK END:<id>`

**Rules:**
- AI must never edit, reformat, move, or delete text inside any `HITL-LOCK` block.
- Append-only: AI may add new comments/changes only; prior HITL comments stay unchanged.
- If a locked comment needs revision, add a new note outside the lock or ask the human reviewer to unlock it.


# Software Requirements — Shared BSW Layer

## 1. Purpose

This document specifies the software requirements for the AUTOSAR-like Basic Software (BSW) modules shared across the physical ECUs (CVC, FZC, RZC) of the Taktflow Zonal Vehicle Platform, per Automotive SPICE 4.0 SWE.1. The BSW layer provides standardized hardware abstraction, communication, diagnostic, and mode management services. The Safety Controller (SC) does not use the BSW stack (bare-metal firmware). The simulated ECUs (BCM, ICU, TCU) use direct POSIX SocketCAN instead of the BSW CAN stack.

## 2. Referenced Documents

| Document ID | Title | Version |
|-------------|-------|---------|
| SYSREQ | System Requirements Specification | 1.0 |
| SSR | Software Safety Requirements | 1.0 |
| TSR | Technical Safety Requirements | 1.0 |
| FSR | Functional Safety Requirements | 1.0 |
| CAN-MATRIX | CAN Message Matrix | 0.1 |

## 3. Requirement Conventions

### 3.1 Requirement Structure

Each requirement follows the format:

- **ID**: SWR-BSW-NNN (sequential)
- **Title**: Descriptive name
- **ASIL**: Varies per module (QM to D depending on usage context)
- **Traces up**: SYS-xxx, SSR-xxx, TSR-xxx
- **Traces down**: firmware/shared/bsw/{module}/{file}
- **Verified by**: TC-BSW-xxx (test case ID)
- **Status**: draft | reviewed | approved | implemented | verified

### 3.2 BSW Module Organization

| Module | AUTOSAR Name | ASIL | Location |
|--------|-------------|------|----------|
| Can | CAN Driver (MCAL) | D | firmware/shared/bsw/mcal/Can.c |
| Spi | SPI Driver (MCAL) | D | firmware/shared/bsw/mcal/Spi.c |
| Adc | ADC Driver (MCAL) | A | firmware/shared/bsw/mcal/Adc.c |
| Pwm | PWM Driver (MCAL) | D | firmware/shared/bsw/mcal/Pwm.c |
| Dio | DIO Driver (MCAL) | D | firmware/shared/bsw/mcal/Dio.c |
| Gpt | GPT Driver (MCAL) | D | firmware/shared/bsw/mcal/Gpt.c |
| CanIf | CAN Interface (ECUAL) | D | firmware/shared/bsw/ecual/CanIf.c |
| PduR | PDU Router (ECUAL) | D | firmware/shared/bsw/ecual/PduR.c |
| IoHwAb | I/O HW Abstraction (ECUAL) | D | firmware/shared/bsw/ecual/IoHwAb.c |
| Com | Communication (Services) | D | firmware/shared/bsw/services/Com.c |
| Dcm | Diagnostic Comm Mgr (Services) | QM | firmware/shared/bsw/services/Dcm.c |
| Dem | Diagnostic Event Mgr (Services) | B | firmware/shared/bsw/services/Dem.c |
| WdgM | Watchdog Manager (Services) | D | firmware/shared/bsw/services/WdgM.c |
| BswM | BSW Mode Manager (Services) | D | firmware/shared/bsw/services/BswM.c |
| E2E | End-to-End Protection (Services) | D | firmware/shared/bsw/services/E2E.c |
| Rte | Runtime Environment | D | firmware/shared/bsw/rte/Rte.c |

---

## 4. MCAL — Microcontroller Abstraction Layer

### SWR-BSW-001: CAN Driver Initialization

- **ASIL**: D
- **Traces up**: SYS-031, TSR-022, SSR-CVC-008
- **Traces down**: firmware/shared/bsw/mcal/Can.c:Can_Init()
- **Verified by**: TC-BSW-001
- **Status**: draft

The CAN driver shall initialize the FDCAN1 peripheral on the STM32G474RE in CAN 2.0B classic mode at 500 kbps. Initialization shall configure: (a) bit timing registers for 500 kbps (prescaler, time segment 1, time segment 2, SJW based on 170 MHz APB clock), (b) receive FIFO 0 for message reception with watermark interrupt, (c) transmit mailbox for message transmission with Tx complete interrupt, (d) global CAN error interrupt (bus-off, error passive, error warning). The Can_Init function shall return E_OK on successful initialization or E_NOT_OK if the peripheral fails to enter normal mode within 10 ms. The CAN driver shall not transmit any message until Can_SetControllerMode(CAN_CS_STARTED) is called.

---

### SWR-BSW-002: CAN Driver Write

- **ASIL**: D
- **Traces up**: SYS-031, SYS-032, TSR-023
- **Traces down**: firmware/shared/bsw/mcal/Can.c:Can_Write()
- **Verified by**: TC-BSW-002, TC-BSW-003
- **Status**: draft

The CAN driver shall provide a Can_Write(Hth, PduInfo) function that queues a CAN frame for transmission. The function shall: (a) validate the Hth (hardware transmit handle) is within the configured range, (b) validate the PduInfo pointer is not NULL and the DLC is between 0 and 8, (c) copy the CAN ID and data to a free transmit mailbox, (d) return CAN_OK if the message was queued, CAN_BUSY if all transmit mailboxes are full (caller shall retry on next MainFunction cycle), or CAN_NOT_OK on error. The function shall be reentrant for different Hths and non-reentrant for the same Hth (protected by the caller).

---

### SWR-BSW-003: CAN Driver MainFunction_Read

- **ASIL**: D
- **Traces up**: SYS-031, SYS-032, TSR-024
- **Traces down**: firmware/shared/bsw/mcal/Can.c:Can_MainFunction_Read()
- **Verified by**: TC-BSW-004, TC-BSW-005
- **Status**: draft

The CAN driver shall provide a Can_MainFunction_Read() function called cyclically (every 5 ms) to process received CAN frames. The function shall: (a) check the FDCAN1 receive FIFO 0 fill level, (b) for each pending message, extract the CAN ID, DLC, and data payload, (c) call CanIf_RxIndication(Hrh, CanId, CanSduPtr) for each received message, (d) acknowledge the message in the FIFO to release the buffer. The function shall process all pending messages in a single call (loop until FIFO empty). A maximum of 16 messages shall be processed per call to prevent main loop starvation.

---

### SWR-BSW-004: CAN Driver Bus-Off Recovery

- **ASIL**: D
- **Traces up**: SYS-034, TSR-038, TSR-039
- **Traces down**: firmware/shared/bsw/mcal/Can.c:Can_MainFunction_BusOff()
- **Verified by**: TC-BSW-006
- **Status**: draft

The CAN driver shall detect bus-off condition by monitoring the FDCAN1 error status register (Protocol Status Register, Bus_Off bit). On bus-off detection, the driver shall: (a) call CanIf_ControllerBusOff(ControllerId) to notify upper layers, (b) set an internal bus-off flag, (c) wait for the CAN 2.0B automatic recovery sequence (128 occurrences of 11 consecutive recessive bits). The driver shall report recovery to CanIf via CanIf_ControllerModeIndication(ControllerId, CAN_CS_STARTED) when the controller exits bus-off. The driver shall not initiate automatic retransmission during bus-off; retry logic is handled by upper layers (BswM).

---

### SWR-BSW-005: CAN Driver Error Callback

- **ASIL**: D
- **Traces up**: SYS-034, TSR-038
- **Traces down**: firmware/shared/bsw/mcal/Can.c:Can_ErrorISR()
- **Verified by**: TC-BSW-007
- **Status**: draft

The CAN driver shall handle FDCAN1 error interrupts by: (a) reading the Error Status Register to identify the error type (stuff error, form error, ack error, bit error, CRC error), (b) reading the transmit and receive error counters (TEC, REC), (c) calling a configurable error notification callback (Can_ErrorNotification) with the error type and counter values. The error counters shall be readable via Can_GetErrorCounters(ControllerId, *TEC, *REC) for upper-layer diagnostic use (Dem DTC reporting, BswM mode decisions).

---

### SWR-BSW-006: SPI Driver for AS5048A Sensors

- **ASIL**: D
- **Traces up**: SYS-047, TSR-001, TSR-010, SSR-CVC-001, SSR-FZC-001
- **Traces down**: firmware/shared/bsw/mcal/Spi.c:Spi_Init(), Spi_WriteIB(), Spi_ReadIB(), Spi_SyncTransmit()
- **Verified by**: TC-BSW-008, TC-BSW-009
- **Status**: draft

The SPI driver shall initialize SPI1 in master mode with: CPOL=0, CPHA=1 (AS5048A requirement), 1 MHz clock (configurable), 16-bit data frame, MSB first, software-managed chip-select. The driver shall provide: (a) Spi_WriteIB(Channel, DataBufferPtr) to load the transmit buffer, (b) Spi_ReadIB(Channel, DataBufferPointer) to read the receive buffer, (c) Spi_SyncTransmit(Sequence) to execute a synchronous SPI transaction with a configurable timeout (default 500 us). The chip-select control shall be performed by the IoHwAb module via Dio_WriteChannel(). The driver shall return E_OK on successful transaction or E_NOT_OK on timeout or SPI error flag.

---

### SWR-BSW-007: ADC Driver for Analog Sensing

- **ASIL**: A
- **Traces up**: SYS-049, TSR-006, TSR-008, SSR-RZC-003, SSR-RZC-006
- **Traces down**: firmware/shared/bsw/mcal/Adc.c:Adc_Init(), Adc_StartGroupConversion(), Adc_ReadGroup()
- **Verified by**: TC-BSW-010, TC-BSW-011
- **Status**: draft

The ADC driver shall initialize ADC1 on the STM32G474RE with: 12-bit resolution, right-aligned data, single conversion mode (software triggered) or continuous mode (timer triggered). The driver shall provide: (a) Adc_Init(ConfigPtr) to configure channel groups (current sensing group at 1 kHz, temperature/voltage group at 10 Hz), (b) Adc_StartGroupConversion(Group) to trigger conversion of a channel group, (c) Adc_ReadGroup(Group, DataBufferPtr) to read the conversion results. Each channel group shall be configured via the Adc_ConfigType structure containing: channel list, number of channels, trigger source, conversion mode, and result buffer pointer. The ADC conversion time shall not exceed 5 us per channel.

---

### SWR-BSW-008: PWM Driver for Motor and Servo Control

- **ASIL**: D
- **Traces up**: SYS-050, TSR-005, TSR-012, SSR-RZC-001, SSR-FZC-004
- **Traces down**: firmware/shared/bsw/mcal/Pwm.c:Pwm_Init(), Pwm_SetDutyCycle(), Pwm_SetOutputToIdle()
- **Verified by**: TC-BSW-012, TC-BSW-013
- **Status**: draft

The PWM driver shall initialize timer peripherals for PWM generation. The driver shall provide: (a) Pwm_Init(ConfigPtr) to configure PWM channels with: timer instance, channel, frequency, initial duty cycle, polarity, and idle state, (b) Pwm_SetDutyCycle(ChannelNumber, DutyCycle) to set the duty cycle (0x0000 = 0%, 0x8000 = 100%) by writing the timer compare register, (c) Pwm_SetOutputToIdle(ChannelNumber) to force the PWM output to its idle state by disabling the timer output and optionally reconfiguring the pin as GPIO output LOW. The duty cycle parameter shall use a 16-bit fixed-point representation (0x0000 to 0x8000) for resolution independence from timer ARR value. The driver shall support both 20 kHz motor PWM (TIM1 on RZC) and 50 Hz servo PWM (TIM2 on FZC).

---

### SWR-BSW-009: DIO Driver for Digital I/O

- **ASIL**: D
- **Traces up**: SYS-050, TSR-005, TSR-033, SSR-RZC-001, SSR-CVC-014
- **Traces down**: firmware/shared/bsw/mcal/Dio.c:Dio_ReadChannel(), Dio_WriteChannel(), Dio_FlipChannel()
- **Verified by**: TC-BSW-014, TC-BSW-015
- **Status**: draft

The DIO driver shall provide atomic access to GPIO pins: (a) Dio_ReadChannel(ChannelId) shall return the current pin level (STD_HIGH or STD_LOW) by reading the GPIO IDR register, (b) Dio_WriteChannel(ChannelId, Level) shall set the pin to the specified level by writing the GPIO BSRR register (atomic set/reset, no read-modify-write), (c) Dio_FlipChannel(ChannelId) shall toggle the pin using BSRR. The DIO channel IDs shall be defined in Dio_Cfg.h as an enum mapping symbolic names (e.g., DIO_CH_MOTOR_R_EN, DIO_CH_MOTOR_L_EN, DIO_CH_ESTOP_IN, DIO_CH_WDG_WDI, DIO_CH_SPI_CS1, DIO_CH_SPI_CS2) to hardware port/pin combinations. The driver shall not modify GPIO configuration (direction, pull-up/down); that is handled during initialization by the port driver or SystemInit.

---

### SWR-BSW-010: GPT Driver for Timing

- **ASIL**: D
- **Traces up**: SYS-053, TSR-046, TSR-047
- **Traces down**: firmware/shared/bsw/mcal/Gpt.c:Gpt_Init(), Gpt_StartTimer(), Gpt_GetTimeElapsed()
- **Verified by**: TC-BSW-016
- **Status**: draft

The GPT driver shall provide general-purpose timer services for the BSW and SWC layers: (a) Gpt_Init(ConfigPtr) to configure timer channels with: timer instance, channel, prescaler, and period, (b) Gpt_StartTimer(Channel, Value) to start a one-shot or continuous timer, (c) Gpt_GetTimeElapsed(Channel) to read the current timer counter in microseconds, (d) Gpt_EnableNotification(Channel) to enable timer expiry callback. The GPT shall provide at least one microsecond-resolution free-running counter for timeout measurement (SPI timeout, CAN message timeout, WCET measurement). The driver shall use TIM6 or TIM7 (basic timers) to avoid conflicts with PWM timers.

---

## 5. ECUAL — ECU Abstraction Layer

### SWR-BSW-011: CanIf PDU Routing

- **ASIL**: D
- **Traces up**: SYS-031, SYS-032, TSR-022, TSR-023, TSR-024
- **Traces down**: firmware/shared/bsw/ecual/CanIf.c:CanIf_Transmit(), CanIf_RxIndication()
- **Verified by**: TC-BSW-017, TC-BSW-018
- **Status**: draft

The CAN Interface module shall route CAN PDUs between upper layers (PduR) and the CAN driver. The module shall provide: (a) CanIf_Transmit(TxPduId, PduInfoPtr) which maps the logical TxPduId to a CAN hardware transmit handle (Hth) and CAN ID via a compile-time routing table, then calls Can_Write(), (b) CanIf_RxIndication(Hrh, CanId, PduInfoPtr) which maps the received CAN ID to a logical RxPduId via a compile-time lookup table and calls PduR_CanIfRxIndication(RxPduId, PduInfoPtr). The routing tables shall be const arrays in flash, generated from the CAN message matrix configuration. Unrecognized CAN IDs in CanIf_RxIndication shall be silently discarded.

---

### SWR-BSW-012: CanIf Controller Mode Management

- **ASIL**: D
- **Traces up**: SYS-034, TSR-038
- **Traces down**: firmware/shared/bsw/ecual/CanIf.c:CanIf_SetControllerMode(), CanIf_ControllerBusOff()
- **Verified by**: TC-BSW-019
- **Status**: draft

The CanIf module shall manage CAN controller modes: (a) CanIf_SetControllerMode(ControllerId, Mode) shall translate CanIf modes (CANIF_CS_STARTED, CANIF_CS_STOPPED, CANIF_CS_SLEEP) to CAN driver modes and call Can_SetControllerMode(), (b) CanIf_ControllerBusOff(ControllerId) shall be called by the CAN driver on bus-off detection; the CanIf shall notify BswM via BswM_CanSM_CurrentState(CAN_BUS_OFF) to trigger mode-dependent actions. The CanIf shall block all CanIf_Transmit calls when the controller is in STOPPED or SLEEP mode, returning E_NOT_OK.

---

### SWR-BSW-013: PduR Routing between CanIf, Com, and Dcm

- **ASIL**: D
- **Traces up**: SYS-031, SYS-032, SYS-037
- **Traces down**: firmware/shared/bsw/ecual/PduR.c:PduR_CanIfRxIndication(), PduR_ComTransmit()
- **Verified by**: TC-BSW-020, TC-BSW-021
- **Status**: draft

The PDU Router shall route PDUs between the CAN Interface and upper-layer modules (Com and Dcm). The routing shall be based on a compile-time routing table mapping each PduId to a destination module:

| PDU Type | Source | Destination | Example |
|----------|--------|-------------|---------|
| Control messages | CanIf (Rx) | Com | Torque request, steering command, brake command |
| Status messages | Com (Tx) | CanIf | Heartbeat, motor current, body status |
| Diagnostic requests | CanIf (Rx) | Dcm | UDS requests (0x7DF, 0x600-0x603) |
| Diagnostic responses | Dcm (Tx) | CanIf | UDS responses |

The PduR shall provide: (a) PduR_CanIfRxIndication(RxPduId, PduInfoPtr) which routes to Com_RxIndication or Dcm_RxIndication based on the routing table, (b) PduR_ComTransmit(TxPduId, PduInfoPtr) which calls CanIf_Transmit for Com-originated messages, (c) PduR_DcmTransmit(TxPduId, PduInfoPtr) which calls CanIf_Transmit for Dcm responses.

---

### SWR-BSW-014: IoHwAb Sensor and Actuator Abstraction

- **ASIL**: D
- **Traces up**: SYS-047, SYS-049, SYS-050, TSR-001, TSR-006, TSR-008, TSR-010
- **Traces down**: firmware/shared/bsw/ecual/IoHwAb.c:IoHwAb_ReadPedalAngle(), IoHwAb_ReadMotorCurrent(), IoHwAb_SetMotorPWM()
- **Verified by**: TC-BSW-022, TC-BSW-023
- **Status**: draft

The I/O Hardware Abstraction module shall provide application-level access to sensors and actuators, hiding MCAL driver details. The module shall provide the following functions:

| Function | Description | MCAL Used |
|----------|-------------|-----------|
| IoHwAb_ReadPedalAngle(SensorId, *Angle) | Read AS5048A pedal sensor, return angle in 0.01 degree units | Spi, Dio (CS) |
| IoHwAb_ReadSteeringAngle(*Angle) | Read AS5048A steering sensor, return angle in 0.01 degree units | Spi, Dio (CS) |
| IoHwAb_ReadMotorCurrent(*Current_mA) | Read ACS723 current, return milliamps | Adc |
| IoHwAb_ReadMotorTemp(*Temp_dC) | Read NTC temperature, return 0.1 degree C units | Adc |
| IoHwAb_ReadBatteryVoltage(*Voltage_mV) | Read battery voltage divider, return millivolts | Adc |
| IoHwAb_SetMotorPWM(Direction, DutyCycle) | Set BTS7960 RPWM/LPWM duty cycle | Pwm, Dio (EN) |
| IoHwAb_SetSteeringServoPWM(DutyCycle) | Set steering servo PWM pulse width | Pwm |
| IoHwAb_SetBrakeServoPWM(DutyCycle) | Set brake servo PWM pulse width | Pwm |
| IoHwAb_SetBuzzer(OnOff) | Drive piezo buzzer GPIO | Dio |
| IoHwAb_ReadEStop(*State) | Read E-stop GPIO state | Dio |

Each function shall perform the necessary MCAL calls and apply calibration/conversion factors (e.g., ADC counts to milliamps, raw angle to calibrated degrees). Error handling: if the underlying MCAL call fails, the IoHwAb function shall return E_NOT_OK and the output parameter shall not be modified (caller retains last valid value).

---

## 6. Services Layer

### SWR-BSW-015: Com Signal Packing and Unpacking

- **ASIL**: D
- **Traces up**: SYS-032, TSR-022, TSR-023, TSR-024, SSR-CVC-007, SSR-CVC-018
- **Traces down**: firmware/shared/bsw/services/Com.c:Com_SendSignal(), Com_ReceiveSignal(), Com_MainFunctionTx(), Com_MainFunctionRx()
- **Verified by**: TC-BSW-024, TC-BSW-025, TC-BSW-026
- **Status**: draft

The Com module shall manage signal-level communication over CAN PDUs. The module shall provide:

(a) **Com_SendSignal(SignalId, SignalDataPtr)**: Copy the signal value into the corresponding PDU transmit buffer at the configured bit position and length. Signal types: uint8, uint16, sint16, boolean. Byte order: little-endian (Intel format).

(b) **Com_ReceiveSignal(SignalId, SignalDataPtr)**: Extract the signal value from the corresponding PDU receive buffer at the configured bit position and length. Return the most recently received value. If no valid message has been received, return the configured initial value.

(c) **Com_MainFunctionTx()**: Called cyclically (every 10 ms). For each PDU whose transmit deadline has expired, call PduR_ComTransmit to queue the PDU for CAN transmission. Tx deadlines are configured per PDU (10 ms for control, 50 ms for heartbeat, 100 ms for status).

(d) **Com_MainFunctionRx()**: Called cyclically (every 10 ms). For each PDU whose receive deadline has expired (no new message received within the configured timeout), set the PDU's timeout flag and notify the application via Com_CbkRxTOut(PduId).

Signal configuration (signal ID, PDU assignment, bit position, length, byte order, initial value) shall be defined in Com_Cfg.h as const structures generated from the CAN message matrix.

---

### SWR-BSW-016: Com E2E Integration

- **ASIL**: D
- **Traces up**: SYS-032, TSR-022, TSR-023, TSR-024
- **Traces down**: firmware/shared/bsw/services/Com.c:Com_MainFunctionTx(), Com_MainFunctionRx()
- **Verified by**: TC-BSW-027, TC-BSW-028
- **Status**: draft

The Com module shall integrate E2E protection into the transmit and receive paths for safety-critical PDUs (configured per PDU in Com_Cfg.h). On the transmit path, Com_MainFunctionTx shall call E2E_Protect(PduDataPtr, DataId) to compute and insert the E2E header (alive counter, CRC-8) before passing the PDU to PduR. On the receive path, Com_MainFunctionRx shall call E2E_Check(PduDataPtr, DataId, *Status) on each received safety-critical PDU. The E2E check status (OK, WRONG_CRC, WRONG_SEQUENCE, REPEATED, LOST) shall be passed to the application via Com_CbkE2EStatus(PduId, E2EStatus). Non-safety PDUs (QM) shall bypass E2E processing.

---

### SWR-BSW-017: Dcm UDS Service Dispatch

- **ASIL**: QM
- **Traces up**: SYS-037, SYS-038, SYS-039, SYS-040
- **Traces down**: firmware/shared/bsw/services/Dcm.c:Dcm_MainFunction(), Dcm_ProcessRequest()
- **Verified by**: TC-BSW-029, TC-BSW-030
- **Status**: draft

The Dcm module shall receive UDS diagnostic requests from the PduR and dispatch them to the appropriate service handler. The Dcm shall support the following services (minimal set for physical ECUs):

| SID | Service | Handler |
|-----|---------|---------|
| 0x10 | DiagnosticSessionControl | Dcm_SessionControl() |
| 0x22 | ReadDataByIdentifier | Dcm_ReadDID() |
| 0x3E | TesterPresent | Dcm_TesterPresent() |

The Dcm shall maintain the current diagnostic session (default or extended) and a session timeout timer (5 seconds). The Dcm_MainFunction() shall be called every 10 ms to manage session timeouts and pending responses. Unsupported SIDs shall return NRC 0x11 (serviceNotSupported). The full UDS service set (0x14, 0x19, 0x27, 0x2E) is handled by the TCU; the physical ECU Dcm provides a minimal subset for local DID reads.

---

### SWR-BSW-018: Dem DTC Event Management

- **ASIL**: B
- **Traces up**: SYS-041, TSR-004, TSR-011, TSR-020
- **Traces down**: firmware/shared/bsw/services/Dem.c:Dem_SetEventStatus(), Dem_GetEventStatus(), Dem_MainFunction()
- **Verified by**: TC-BSW-031, TC-BSW-032, TC-BSW-033
- **Status**: draft

The Dem module shall manage diagnostic trouble codes (DTCs) for the local ECU. The module shall provide:

(a) **Dem_SetEventStatus(EventId, EventStatus)**: Called by application SWCs to report fault status. EventStatus: DEM_EVENT_STATUS_PASSED (fault not present) or DEM_EVENT_STATUS_FAILED (fault present). The Dem shall apply debouncing (SWR-BSW-019) before updating the DTC status.

(b) **Dem_GetEventStatus(EventId, *EventStatusByte)**: Return the current DTC status byte for the given event.

(c) **Dem_MainFunction()**: Called every 100 ms. Shall process: debounce counter updates, freeze-frame capture on status transitions, aging counter updates, and DTC notification broadcast (transmit DTC event CAN message on ID 0x400 when a DTC status changes).

The Dem shall support a minimum of 32 DTC events per ECU. Each DTC shall have: DTC number (3 bytes, SAE J2012), status byte (8 bits per ISO 14229), occurrence counter (8-bit, saturating), aging counter (8-bit), and freeze-frame data (16 bytes: vehicle state, speed, temperature, voltage, current, torque, timestamp).

---

### SWR-BSW-019: Dem Counter-Based Debouncing

- **ASIL**: B
- **Traces up**: SYS-041, TSR-002, TSR-006
- **Traces down**: firmware/shared/bsw/services/Dem.c:Dem_DebounceEvent()
- **Verified by**: TC-BSW-034, TC-BSW-035
- **Status**: draft

The Dem module shall implement counter-based debouncing for DTC events. Each event shall have a configurable debounce counter with: (a) increment step (default: +1 per FAILED report), (b) decrement step (default: -1 per PASSED report), (c) pass threshold (default: -3, counter must reach this value for confirmed pass), (d) fail threshold (default: +3, counter must reach this value for confirmed fail). The counter shall be clamped to the range [pass_threshold, fail_threshold]. When the counter reaches the fail threshold, the DTC testFailed bit shall be set. When the counter reaches the pass threshold, the testFailed bit shall be cleared. The debounce parameters shall be configurable per event in Dem_Cfg.h.

---

### SWR-BSW-020: Dem NVM Persistence

- **ASIL**: B
- **Traces up**: SYS-041, TSR-037
- **Traces down**: firmware/shared/bsw/services/Dem.c:Dem_NvmWrite(), Dem_NvmRead()
- **Verified by**: TC-BSW-036
- **Status**: draft

The Dem module shall persist the DTC database to non-volatile memory (dedicated flash sector) on the following events: (a) a DTC status byte changes (testFailed, confirmedDTC transitions), (b) the system transitions to SHUTDOWN (controlled shutdown), (c) a periodic save every 60 seconds during operation. On startup, the Dem shall read the persisted DTC database and restore all DTC entries with their status bytes, occurrence counters, aging counters, and freeze-frame data. The NVM storage shall use a dual-copy CRC-32 protected scheme (same as the vehicle state persistence in SSR-CVC-019) to protect against flash corruption.

---

### SWR-BSW-021: WdgM Alive Supervision

- **ASIL**: D
- **Traces up**: SYS-027, TSR-031, TSR-032, SSR-CVC-013, SSR-FZC-020, SSR-RZC-013
- **Traces down**: firmware/shared/bsw/services/WdgM.c:WdgM_Init(), WdgM_CheckpointReached(), WdgM_MainFunction()
- **Verified by**: TC-BSW-037, TC-BSW-038, TC-BSW-039
- **Status**: draft

The WdgM module shall supervise the alive status of each ECU's safety-critical task. The module shall provide:

(a) **WdgM_Init(ConfigPtr)**: Configure supervision entities. Each ECU shall have one supervised entity per safety task (e.g., SE_PEDAL_TASK, SE_MOTOR_TASK). Each entity has: expected alive count per supervision cycle, tolerance (allowed deviation from expected count), and supervision period.

(b) **WdgM_CheckpointReached(SupervisedEntityId, CheckpointId)**: Called by the application at each checkpoint in the task execution sequence. The WdgM shall increment the alive counter for the supervised entity.

(c) **WdgM_MainFunction()**: Called every supervision period (configurable, default 100 ms). For each supervised entity, the WdgM shall compare the alive counter against the expected range (expected_count +/- tolerance). If the counter is within range, the entity status is ALIVE. If outside range, the entity status is EXPIRED. The WdgM shall then reset the alive counter. If all entities are ALIVE, the WdgM shall toggle the TPS3823 WDI pin via Dio_FlipChannel(DIO_CH_WDG_WDI). If any entity is EXPIRED, the WDI toggle shall be suppressed (watchdog will timeout and reset the MCU).

The WdgM shall also verify the self-check conditions before toggling the watchdog: stack canary intact, RAM test pattern valid, CAN controller not in bus-off state (per SSR-CVC-013, SSR-FZC-020, SSR-RZC-013).

---

### SWR-BSW-022: BswM Mode Management Rules

- **ASIL**: D
- **Traces up**: SYS-029, SYS-030, TSR-035, TSR-036, SSR-CVC-016, SSR-CVC-017
- **Traces down**: firmware/shared/bsw/services/BswM.c:BswM_Init(), BswM_MainFunction(), BswM_RequestMode()
- **Verified by**: TC-BSW-040, TC-BSW-041, TC-BSW-042
- **Status**: draft

The BswM module shall manage mode-dependent behavior of the BSW and application layers. The module shall implement a rule-based engine with the following rules:

| Rule | Condition | Action |
|------|-----------|--------|
| R1 | Vehicle state received as RUN | Enable all Com Tx PDUs, enable all PWM channels |
| R2 | Vehicle state received as DEGRADED | Apply torque limit 75%, apply speed limit 50%, activate BCM hazard lights |
| R3 | Vehicle state received as LIMP | Apply torque limit 30%, apply speed limit 20%, activate BCM hazard lights |
| R4 | Vehicle state received as SAFE_STOP | Disable motor PWM, enable brake servo maximum, command steering center |
| R5 | Vehicle state received as SHUTDOWN | Disable all outputs, stop Com Tx except heartbeat, stop feeding watchdog |
| R6 | CAN bus-off detected | Execute R4 actions locally, suppress CAN Tx |
| R7 | E-stop received | Execute R4 actions, set local state to SAFE_STOP |
| R8 | Startup complete | Transition from INIT to RUN (if self-test passed) or SAFE_STOP (if self-test failed) |

The BswM_MainFunction() shall be called every 10 ms to evaluate all rules and execute corresponding actions. The BswM shall accept mode indications from multiple sources: Com (vehicle state CAN message), CanIf (bus-off notification), application SWCs (local fault detection).

---

### SWR-BSW-023: E2E CRC-8 Calculation

- **ASIL**: D
- **Traces up**: SYS-032, TSR-022, SSR-CVC-008, SSR-FZC-015, SSR-RZC-008
- **Traces down**: firmware/shared/bsw/services/E2E.c:E2E_CalculateCRC8()
- **Verified by**: TC-BSW-043, TC-BSW-044
- **Status**: draft

The E2E module shall implement CRC-8 calculation using the SAE J1850 polynomial (0x1D) with initial value 0xFF. The function E2E_CalculateCRC8(DataPtr, Length, StartValue) shall compute the CRC byte-by-byte over the input data. The CRC shall be computed over the application data payload (bytes 2-7 of the CAN frame) concatenated with the Data ID byte (byte 0 bits 3:0). The result shall be stored in byte 1 of the CAN frame. The CRC calculation shall be constant-time (no data-dependent branches) to ensure deterministic WCET. A precomputed 256-entry CRC lookup table in flash shall be used for performance.

---

### SWR-BSW-024: E2E Alive Counter and Data ID Management

- **ASIL**: D
- **Traces up**: SYS-032, TSR-022, TSR-023, TSR-024
- **Traces down**: firmware/shared/bsw/services/E2E.c:E2E_Protect(), E2E_Check()
- **Verified by**: TC-BSW-045, TC-BSW-046, TC-BSW-047
- **Status**: draft

The E2E module shall manage alive counters and data IDs for E2E-protected CAN messages:

(a) **E2E_Protect(PduDataPtr, DataId)**: Increment the alive counter for the given DataId (4-bit, wrapping 0-15), write it to byte 0 bits 7:4, write the DataId to byte 0 bits 3:0, compute CRC-8 over bytes 2-7 plus DataId and write to byte 1. Return E2E_P_OK.

(b) **E2E_Check(PduDataPtr, DataId, *Status)**: Extract the alive counter from byte 0 bits 7:4, extract the DataId from byte 0 bits 3:0, recompute CRC-8 and compare with byte 1. Verify: DataId matches expected, alive counter has incremented by 1 from previous reception. Return status: E2E_P_OK (all checks pass), E2E_P_WRONGCRC (CRC mismatch), E2E_P_WRONGSEQUENCE (counter gap > 1), E2E_P_REPEATED (same counter as previous), E2E_P_LOST (counter gap 2-14, some messages lost).

The module shall maintain an array of 16 alive counter states (one per DataId) for both transmit and receive sides.

---

### SWR-BSW-025: E2E Per-PDU Configuration

- **ASIL**: D
- **Traces up**: SYS-032, TSR-022
- **Traces down**: firmware/shared/bsw/services/E2E.c, firmware/shared/bsw/services/E2E_Cfg.h
- **Verified by**: TC-BSW-048
- **Status**: draft

The E2E module shall support per-PDU configuration via a compile-time configuration table (E2E_Cfg.h). Each E2E-protected PDU shall have a configuration entry specifying:

| Field | Description |
|-------|-------------|
| DataId | 4-bit unique identifier for this message type |
| DataLength | Payload length in bytes (always 8 for CAN 2.0B) |
| MaxDeltaCounter | Maximum allowed alive counter gap before declaring message loss (default: 1) |
| MaxNoNewOrRepeatedData | Maximum consecutive E2E failures before safe default substitution (default: 3) |
| CounterOffset | Bit position of alive counter in the PDU (fixed: byte 0, bits 7:4) |
| CRCOffset | Byte position of CRC in the PDU (fixed: byte 1) |

The configuration table shall be a const array in flash. The E2E_Protect and E2E_Check functions shall look up the configuration by DataId.

---

## 7. Runtime Environment

### SWR-BSW-026: Rte Port-Based Communication

- **ASIL**: D
- **Traces up**: SYS-029, SYS-030, TSR-035
- **Traces down**: firmware/shared/bsw/rte/Rte.c:Rte_Read(), Rte_Write(), Rte_Call()
- **Verified by**: TC-BSW-049, TC-BSW-050
- **Status**: draft

The RTE module shall provide port-based communication between SWC runnables and BSW modules. The module shall provide:

(a) **Rte_Read_{Port}_{Signal}(DataPtr)**: Copy the latest value of the signal from the RTE buffer to the caller's variable. Uses copy semantics (data is copied, not referenced) to ensure data consistency without locks.

(b) **Rte_Write_{Port}_{Signal}(Data)**: Copy the provided value into the RTE buffer for consumption by other runnables. Uses copy semantics.

(c) **Rte_Call_{Port}_{Operation}(Args)**: Invoke a client-server operation on a BSW module (e.g., Rte_Call_Dem_SetEventStatus to report a DTC event).

The RTE buffers shall be statically allocated arrays in RAM (no dynamic allocation). For signals wider than 32 bits, the RTE shall use a double-buffer scheme with a sequence counter to provide consistent reads without disabling interrupts.

---

### SWR-BSW-027: Rte Runnable Scheduling

- **ASIL**: D
- **Traces up**: SYS-053, TSR-046, TSR-047, SSR-CVC-023, SSR-FZC-023, SSR-RZC-017
- **Traces down**: firmware/shared/bsw/rte/Rte.c:Rte_MainFunction()
- **Verified by**: TC-BSW-051, TC-BSW-052
- **Status**: draft

The RTE module shall schedule application SWC runnables at their configured periods using the FreeRTOS RTOS scheduler. Each runnable shall be assigned: (a) a period (1 ms, 10 ms, 50 ms, 100 ms, 200 ms), (b) an RTOS priority (higher for safety-critical runnables, lower for QM runnables), (c) a WCET budget. The RTE shall create one FreeRTOS task per priority level, with each task executing all runnables of that priority in round-robin order within the task period. Safety-critical runnables (ASIL D: pedal plausibility, motor control, steering monitor, brake monitor) shall have higher RTOS priority than QM runnables (display update, telemetry). The RTE shall measure the actual execution time of each runnable using GPT timestamps and flag a WCET overrun DTC via Dem if any runnable exceeds its budget.

---

## 8. Requirements Traceability Summary

### 8.1 Upstream Traceability

| SWR-BSW | Traces Up To |
|---------|-------------|
| SWR-BSW-001 | SYS-031, TSR-022, SSR-CVC-008 |
| SWR-BSW-002 | SYS-031, SYS-032, TSR-023 |
| SWR-BSW-003 | SYS-031, SYS-032, TSR-024 |
| SWR-BSW-004 | SYS-034, TSR-038, TSR-039 |
| SWR-BSW-005 | SYS-034, TSR-038 |
| SWR-BSW-006 | SYS-047, TSR-001, TSR-010, SSR-CVC-001, SSR-FZC-001 |
| SWR-BSW-007 | SYS-049, TSR-006, TSR-008, SSR-RZC-003, SSR-RZC-006 |
| SWR-BSW-008 | SYS-050, TSR-005, TSR-012, SSR-RZC-001, SSR-FZC-004 |
| SWR-BSW-009 | SYS-050, TSR-005, TSR-033, SSR-RZC-001, SSR-CVC-014 |
| SWR-BSW-010 | SYS-053, TSR-046, TSR-047 |
| SWR-BSW-011 | SYS-031, SYS-032, TSR-022, TSR-023, TSR-024 |
| SWR-BSW-012 | SYS-034, TSR-038 |
| SWR-BSW-013 | SYS-031, SYS-032, SYS-037 |
| SWR-BSW-014 | SYS-047, SYS-049, SYS-050, TSR-001, TSR-006, TSR-008, TSR-010 |
| SWR-BSW-015 | SYS-032, TSR-022, TSR-023, TSR-024, SSR-CVC-007, SSR-CVC-018 |
| SWR-BSW-016 | SYS-032, TSR-022, TSR-023, TSR-024 |
| SWR-BSW-017 | SYS-037, SYS-038, SYS-039, SYS-040 |
| SWR-BSW-018 | SYS-041, TSR-004, TSR-011, TSR-020 |
| SWR-BSW-019 | SYS-041, TSR-002, TSR-006 |
| SWR-BSW-020 | SYS-041, TSR-037 |
| SWR-BSW-021 | SYS-027, TSR-031, TSR-032, SSR-CVC-013, SSR-FZC-020, SSR-RZC-013 |
| SWR-BSW-022 | SYS-029, SYS-030, TSR-035, TSR-036, SSR-CVC-016, SSR-CVC-017 |
| SWR-BSW-023 | SYS-032, TSR-022, SSR-CVC-008, SSR-FZC-015, SSR-RZC-008 |
| SWR-BSW-024 | SYS-032, TSR-022, TSR-023, TSR-024 |
| SWR-BSW-025 | SYS-032, TSR-022 |
| SWR-BSW-026 | SYS-029, SYS-030, TSR-035 |
| SWR-BSW-027 | SYS-053, TSR-046, TSR-047, SSR-CVC-023, SSR-FZC-023, SSR-RZC-017 |

### 8.2 Requirement Summary

| Metric | Value |
|--------|-------|
| Total SWR-BSW requirements | 27 |
| ASIL D | 21 |
| ASIL B | 3 |
| ASIL A | 1 |
| QM | 2 |
| Test cases (placeholder) | TC-BSW-001 to TC-BSW-052 |

### 8.3 ASIL Distribution by Module

| Module | Count | ASIL |
|--------|-------|------|
| Can (MCAL) | 5 | D |
| Spi (MCAL) | 1 | D |
| Adc (MCAL) | 1 | A |
| Pwm (MCAL) | 1 | D |
| Dio (MCAL) | 1 | D |
| Gpt (MCAL) | 1 | D |
| CanIf (ECUAL) | 2 | D |
| PduR (ECUAL) | 1 | D |
| IoHwAb (ECUAL) | 1 | D |
| Com (Services) | 2 | D |
| Dcm (Services) | 1 | QM |
| Dem (Services) | 3 | B |
| WdgM (Services) | 1 | D |
| BswM (Services) | 1 | D |
| E2E (Services) | 3 | D |
| Rte | 2 | D |

## 9. Open Items and Assumptions

### 9.1 Assumptions

| ID | Assumption | Impact |
|----|-----------|--------|
| BSW-A-001 | FreeRTOS is the RTOS on all STM32G474RE ECUs | RTE scheduling and WdgM depend on FreeRTOS task management |
| BSW-A-002 | BSW modules share a common configuration header generated from the CAN matrix | Config consistency across ECUs |
| BSW-A-003 | The BSW layer is compiled identically for CVC, FZC, and RZC with per-ECU configuration headers | Single source, multiple configurations |
| BSW-A-004 | NVM operations (Dem persistence) use a dedicated flash sector not used by application code | Flash sector allocation defined in linker script |

### 9.2 Open Items

| ID | Item | Owner | Target |
|----|------|-------|--------|
| BSW-O-001 | Generate Com and CanIf configuration from finalized CAN message matrix | SW Architect | SWE.2 phase |
| BSW-O-002 | Define Dem DTC event-to-DTC-number mapping table | System Architect | SWE.1 phase |
| BSW-O-003 | Define BswM rule action detail (which PWM channels, which Com PDUs) per ECU | SW Architect | SWE.2 phase |
| BSW-O-004 | Perform WCET analysis for BSW MainFunction calls | SW Engineer | SWE.3 phase |

## 10. Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 0.1 | 2026-02-21 | System | Initial stub (planned status) |
| 1.0 | 2026-02-21 | System | Complete SWR specification: 27 requirements (SWR-BSW-001 to SWR-BSW-027), full traceability, ASIL allocation |

