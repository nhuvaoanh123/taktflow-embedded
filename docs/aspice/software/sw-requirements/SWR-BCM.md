---
document_id: SWR-BCM
title: "Software Requirements — BCM"
version: "1.0"
status: draft
aspice_process: SWE.1
ecu: BCM
asil: QM
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


# Software Requirements — Body Control Module (BCM)

## 1. Purpose

This document specifies the software requirements for the Body Control Module (BCM) of the Taktflow Zonal Vehicle Platform, per Automotive SPICE 4.0 SWE.1. The BCM is a simulated ECU running as a Docker container on a POSIX platform with SocketCAN (vcan0). It handles body functions: headlight control, turn indicators, hazard lights, and door lock simulation. All BCM functions are QM-rated (no safety-critical functions).

## 2. Referenced Documents

| Document ID | Title | Version |
|-------------|-------|---------|
| SYSREQ | System Requirements Specification | 1.0 |
| SG | Safety Goals | 1.0 |
| FSC | Functional Safety Concept | 1.0 |
| CAN-MATRIX | CAN Message Matrix | 0.1 |

## 3. Requirement Conventions

### 3.1 Requirement Structure

Each requirement follows the format:

- **ID**: SWR-BCM-NNN (sequential)
- **Title**: Descriptive name
- **ASIL**: QM (all BCM requirements)
- **Traces up**: SYS-xxx
- **Traces down**: firmware/bcm/src/{file}
- **Verified by**: TC-BCM-xxx (test case ID)
- **Status**: draft | reviewed | approved | implemented | verified

### 3.2 Runtime Environment

The BCM runs as a Docker container (Linux/POSIX) and accesses the CAN bus via SocketCAN (vcan0). It does not use the shared AUTOSAR-like BSW stack. CAN access is via direct POSIX socket API (AF_CAN, SOCK_RAW, CAN_RAW).

---

## 4. Requirements

### SWR-BCM-001: SocketCAN Interface Initialization

- **ASIL**: QM
- **Traces up**: SYS-031, SYS-035
- **Traces down**: firmware/bcm/src/can_interface.c:BCM_CAN_Init()
- **Verified by**: TC-BCM-001
- **Status**: draft

The BCM software shall initialize a SocketCAN interface on startup by: (a) creating a raw CAN socket (AF_CAN, SOCK_RAW, CAN_RAW), (b) binding the socket to the vcan0 interface, (c) configuring a CAN filter to accept messages with CAN IDs 0x100 (vehicle state), 0x350 (body control command), and 0x001 (E-stop), and (d) verifying the socket is operational by reading the interface flags. If socket initialization fails, the BCM shall log an error and retry every 1 second up to 10 attempts before entering an error state.

---

### SWR-BCM-002: Vehicle State CAN Reception

- **ASIL**: QM
- **Traces up**: SYS-029, SYS-030, SYS-035
- **Traces down**: firmware/bcm/src/can_interface.c:BCM_CAN_ReceiveState()
- **Verified by**: TC-BCM-002, TC-BCM-003
- **Status**: draft

The BCM software shall receive the vehicle state CAN message (CAN ID 0x100) from the CVC and extract the vehicle state field (4-bit, byte 0 bits 3:0 after E2E header). The BCM shall maintain a local copy of the current vehicle state (INIT, RUN, DEGRADED, LIMP, SAFE_STOP, SHUTDOWN). If no vehicle state message is received for 500 ms, the BCM shall assume SHUTDOWN state and deactivate all outputs. The BCM shall perform E2E validation (CRC-8 and alive counter check) on the received message and discard messages that fail validation.

---

### SWR-BCM-003: Headlight Auto-On Control

- **ASIL**: QM
- **Traces up**: SYS-035
- **Traces down**: firmware/bcm/src/body_control.c:BCM_Headlight_Update()
- **Verified by**: TC-BCM-004, TC-BCM-005
- **Status**: draft

The BCM software shall automatically activate the headlight output when the received vehicle state is RUN, DEGRADED, or LIMP. The headlight shall be represented as a boolean state variable (headlight_on). The headlight shall be activated within 100 ms of receiving a vehicle state transition to RUN. The headlight state shall be included in the body status CAN message (SWR-BCM-010).

---

### SWR-BCM-004: Headlight Auto-Off Control

- **ASIL**: QM
- **Traces up**: SYS-035
- **Traces down**: firmware/bcm/src/body_control.c:BCM_Headlight_Update()
- **Verified by**: TC-BCM-006
- **Status**: draft

The BCM software shall automatically deactivate the headlight output when the received vehicle state is INIT, SAFE_STOP, or SHUTDOWN, or when no vehicle state message has been received for 500 ms. The headlight shall be deactivated within 100 ms of the triggering condition.

---

### SWR-BCM-005: Headlight Manual Override

- **ASIL**: QM
- **Traces up**: SYS-035
- **Traces down**: firmware/bcm/src/body_control.c:BCM_Headlight_Override()
- **Verified by**: TC-BCM-007
- **Status**: draft

The BCM software shall support a manual headlight override via a CAN command (CAN ID 0x350, body control command, byte 2 bit 0: 1 = force ON, 0 = auto mode). When manual override is active, the headlight shall remain on regardless of the vehicle state. The manual override shall be cancelled by sending a command with the override bit cleared, returning to automatic mode (SWR-BCM-003, SWR-BCM-004).

---

### SWR-BCM-006: Turn Indicator Control

- **ASIL**: QM
- **Traces up**: SYS-036
- **Traces down**: firmware/bcm/src/body_control.c:BCM_TurnIndicator_Update()
- **Verified by**: TC-BCM-008, TC-BCM-009, TC-BCM-010
- **Status**: draft

The BCM software shall control left and right turn indicators based on a CAN command (CAN ID 0x350, byte 3: 0x00 = off, 0x01 = left, 0x02 = right). When a turn indicator is active, the corresponding indicator state shall flash at 1.5 Hz (350 ms on, 350 ms off, total period approximately 700 ms). The flash timing shall be maintained by a software timer with 10 ms resolution. When the turn indicator command changes (e.g., left to right, or left to off), the new state shall take effect within one flash cycle (700 ms maximum). Only one turn direction shall be active at a time (left or right, never both simultaneously in turn mode).

---

### SWR-BCM-007: Hazard Light Activation

- **ASIL**: QM
- **Traces up**: SYS-036
- **Traces down**: firmware/bcm/src/body_control.c:BCM_HazardLight_Update()
- **Verified by**: TC-BCM-011, TC-BCM-012, TC-BCM-013
- **Status**: draft

The BCM software shall activate hazard lights (both left and right indicators flashing simultaneously) when any of the following conditions is detected: (a) the vehicle state transitions to SAFE_STOP or SHUTDOWN, (b) a hazard light CAN command is received (CAN ID 0x350, byte 3: 0x03 = hazard), or (c) the vehicle state is DEGRADED or LIMP (automatic hazard activation for degraded operation). Hazard lights shall flash at 1.5 Hz (350 ms on, 350 ms off), synchronized between left and right. Hazard lights shall have priority over turn indicators: if hazard lights are active, any turn indicator command shall be ignored until hazard lights are deactivated.

---

### SWR-BCM-008: Hazard Light Deactivation

- **ASIL**: QM
- **Traces up**: SYS-036
- **Traces down**: firmware/bcm/src/body_control.c:BCM_HazardLight_Update()
- **Verified by**: TC-BCM-014
- **Status**: draft

The BCM software shall deactivate hazard lights when all of the following conditions are met: (a) the vehicle state is RUN (not DEGRADED, LIMP, SAFE_STOP, or SHUTDOWN), (b) no hazard light CAN command is active, and (c) the vehicle state has been RUN for at least 2 consecutive seconds (debounce to prevent hazard light flicker during transient state changes). Upon deactivation, turn indicator commands shall be honored again.

---

### SWR-BCM-009: Door Lock Simulation

- **ASIL**: QM
- **Traces up**: SYS-035
- **Traces down**: firmware/bcm/src/body_control.c:BCM_DoorLock_Update()
- **Verified by**: TC-BCM-015, TC-BCM-016
- **Status**: draft

The BCM software shall simulate door lock/unlock functionality based on a CAN command (CAN ID 0x350, byte 4: 0x00 = unlock, 0x01 = lock). The BCM shall maintain a door lock state variable (locked/unlocked). Upon receiving a lock or unlock command, the state shall transition within 100 ms. The door lock state shall be included in the body status CAN message (SWR-BCM-010). The BCM shall automatically lock the doors when the vehicle state transitions to RUN (speed > 0 implied by RUN state) and automatically unlock when the vehicle state transitions to SHUTDOWN.

---

### SWR-BCM-010: Body Status CAN Transmission

- **ASIL**: QM
- **Traces up**: SYS-035, SYS-036
- **Traces down**: firmware/bcm/src/can_interface.c:BCM_CAN_TransmitStatus()
- **Verified by**: TC-BCM-017, TC-BCM-018
- **Status**: draft

The BCM software shall transmit a body status CAN message (CAN ID 0x360) every 100 ms. The message payload shall contain:

| Byte | Bits | Field | Description |
|------|------|-------|-------------|
| 0 | 7:4 | Alive counter | 4-bit, wrapping 0-15 |
| 0 | 3:0 | Reserved | Set to 0 |
| 1 | 7:0 | CRC-8 | CRC-8 over bytes 2-7 |
| 2 | 0 | Headlight state | 0 = off, 1 = on |
| 2 | 1 | Left indicator | 0 = off, 1 = on (current flash state) |
| 2 | 2 | Right indicator | 0 = off, 1 = on (current flash state) |
| 2 | 3 | Hazard active | 0 = off, 1 = active |
| 2 | 4 | Door locked | 0 = unlocked, 1 = locked |
| 2 | 5 | Headlight override | 0 = auto, 1 = manual override |
| 2 | 7:6 | Reserved | Set to 0 |

The alive counter shall increment by 1 per transmission. The CRC-8 shall use polynomial 0x1D (SAE J1850) for consistency with the E2E protection used on safety-critical messages.

---

### SWR-BCM-011: Body Control Command Reception

- **ASIL**: QM
- **Traces up**: SYS-035, SYS-036
- **Traces down**: firmware/bcm/src/can_interface.c:BCM_CAN_ReceiveCommand()
- **Verified by**: TC-BCM-019, TC-BCM-020
- **Status**: draft

The BCM software shall receive body control commands on CAN ID 0x350 from the CVC. This is a QM (non-safety) message and does not use E2E protection. The command message format shall be:

| Byte | Field | Description |
|------|-------|-------------|
| 0 | Headlight control | Bit 0: manual override (0=auto, 1=force on) |
| 1 | Indicator control | 0x00=off, 0x01=left, 0x02=right, 0x03=hazard |
| 2 | Door lock control | 0x00=unlock, 0x01=lock |

If no valid command is received for 2 seconds, the BCM shall revert all outputs to their automatic/default states.

---

### SWR-BCM-012: BCM Main Loop Execution

- **ASIL**: QM
- **Traces up**: SYS-035
- **Traces down**: firmware/bcm/src/main.c:BCM_Main()
- **Verified by**: TC-BCM-021
- **Status**: draft

The BCM software shall execute a main loop at a 10 ms cycle time (100 Hz) using a POSIX timer (timer_create with CLOCK_MONOTONIC) or equivalent scheduling mechanism. Each loop iteration shall: (a) read all pending CAN messages from the SocketCAN socket (non-blocking read), (b) update the vehicle state from received messages (SWR-BCM-002), (c) process body control commands (SWR-BCM-011), (d) update headlight state (SWR-BCM-003, SWR-BCM-004, SWR-BCM-005), (e) update turn indicator and hazard light flash timers (SWR-BCM-006, SWR-BCM-007, SWR-BCM-008), (f) update door lock state (SWR-BCM-009), (g) transmit body status message every 10th cycle (100 ms) (SWR-BCM-010). The main loop shall log a warning if any iteration exceeds 5 ms execution time.

---

## 5. Requirements Traceability Summary

### 5.1 SYS to SWR-BCM Mapping

| SYS | SWR-BCM |
|-----|---------|
| SYS-031 | SWR-BCM-001 |
| SYS-029 | SWR-BCM-002 |
| SYS-030 | SWR-BCM-002 |
| SYS-035 | SWR-BCM-001, SWR-BCM-002, SWR-BCM-003, SWR-BCM-004, SWR-BCM-005, SWR-BCM-009, SWR-BCM-010, SWR-BCM-011, SWR-BCM-012 |
| SYS-036 | SWR-BCM-006, SWR-BCM-007, SWR-BCM-008, SWR-BCM-010, SWR-BCM-011 |

### 5.2 Requirement Summary

| Metric | Value |
|--------|-------|
| Total SWR-BCM requirements | 12 |
| ASIL D | 0 |
| ASIL C | 0 |
| ASIL B | 0 |
| ASIL A | 0 |
| QM | 12 |
| Test cases (placeholder) | TC-BCM-001 to TC-BCM-021 |

## 6. Open Items and Assumptions

### 6.1 Assumptions

| ID | Assumption | Impact |
|----|-----------|--------|
| BCM-A-001 | Docker container has access to vcan0 SocketCAN interface | BCM CAN communication depends on Docker network configuration |
| BCM-A-002 | CAN message IDs 0x350 (body command) and 0x360 (body status) are not used by other ECUs | CAN arbitration conflict if IDs overlap |
| BCM-A-003 | POSIX timer provides sufficient timing accuracy for 1.5 Hz flash rate | Flash rate deviation within 10% is acceptable for turn indicators |

### 6.2 Open Items

| ID | Item | Owner | Target |
|----|------|-------|--------|
| BCM-O-001 | Finalize CAN message IDs for body control domain in CAN matrix | System Architect | SYS.3 phase |
| BCM-O-002 | Define Docker container networking for SocketCAN access | DevOps Engineer | Phase 3 |

## 7. Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 0.1 | 2026-02-21 | System | Initial stub (planned status) |
| 1.0 | 2026-02-21 | System | Complete SWR specification: 12 requirements (SWR-BCM-001 to SWR-BCM-012), traceability, test case mapping |

