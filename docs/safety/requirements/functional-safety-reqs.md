---
document_id: FSR
title: "Functional Safety Requirements"
version: "1.0"
status: draft
iso_26262_part: 3
aspice_process: SYS.1
date: 2026-02-21
---

# Functional Safety Requirements

## 1. Purpose

This document specifies the functional safety requirements (FSR) for the Taktflow Zonal Vehicle Platform, derived from the safety goals (document SG) via the functional safety concept (document FSC), per ISO 26262-3 Clause 8.

Functional safety requirements define WHAT the system shall do to achieve or maintain a safe state. They are allocated to system elements and refined into technical safety requirements (TSR) in the technical safety concept (ISO 26262-4).

## 2. Referenced Documents

| Document ID | Title | Version |
|-------------|-------|---------|
| ITEM-DEF | Item Definition | 1.0 |
| HARA | Hazard Analysis and Risk Assessment | 1.0 |
| SG | Safety Goals | 1.0 |
| FSC | Functional Safety Concept | 1.0 |

## 3. Requirement Conventions

### 3.1 Requirement Structure

Each requirement follows the format:

- **ID**: FSR-XXX (sequential)
- **Title**: Descriptive name
- **ASIL**: Inherited from the traced safety goal (or decomposed per ISO 26262-9)
- **Traces up**: Safety goal(s) this requirement derives from
- **Traces down**: Technical safety requirement(s) that refine this FSR (see TSR document)
- **Safety mechanism**: Reference to the mechanism in the FSC that implements this requirement
- **Status**: draft | reviewed | approved | implemented | verified

### 3.2 Requirement Language

Requirements use "shall" for mandatory behavior, "should" for recommended behavior, and "may" for optional behavior. All FSRs in this document use "shall" — they are mandatory.

### 3.3 ASIL Inheritance

Unless ASIL decomposition is applied (per ISO 26262-9 Clause 5, documented in ASIL-DECOMP), each FSR inherits the ASIL level of its parent safety goal. Where a single FSR traces to multiple safety goals, it inherits the highest ASIL among them.

---

## 4. Functional Safety Requirements

### 4.1 Drive-by-Wire Safety (SG-001, SG-002)

---

### FSR-001: Dual Pedal Sensor Plausibility Monitoring

- **ASIL**: D
- **Traces up**: SG-001
- **Traces down**: TSR-001, TSR-002- **Safety mechanism**: SM-001
- **Allocation**: CVC
- **Status**: draft

The system shall continuously compare the readings from both pedal position sensors (AS5048A sensor 1 and AS5048A sensor 2) and detect a plausibility fault when the absolute difference between the two sensor readings exceeds a calibratable threshold (default: 5% of full-scale range) for more than 2 consecutive control cycles.

**Rationale**: Dual redundant pedal sensors are the primary means of detecting pedal position sensor faults. A single sensor failure (drift, stuck, offset) must be detected before it can cause unintended acceleration. The debounce of 2 cycles (20 ms) prevents false detection from transient noise while remaining within the 50 ms FTTI of SG-001.

---

### FSR-002: Pedal Sensor Fault Detection Within 20 ms

- **ASIL**: D
- **Traces up**: SG-001
- **Traces down**: TSR-003- **Safety mechanism**: SM-001
- **Allocation**: CVC
- **Status**: draft

The system shall detect a pedal sensor plausibility fault within 20 ms of the fault occurrence. Detection shall include both the dual-sensor comparison (FSR-001) and individual sensor range checks (each sensor reading within 0.5V to 4.5V equivalent).

**Rationale**: The 20 ms detection time is derived from the SG-001 FTTI of 50 ms. With 20 ms detection + 10 ms reaction (CAN transmission + motor cutoff) + 20 ms margin, the system meets the FTTI with sufficient margin for timing jitter and worst-case scheduling.

---

### FSR-003: Motor Torque Cutoff on Pedal Plausibility Failure

- **ASIL**: D
- **Traces up**: SG-001
- **Traces down**: TSR-004, TSR-005- **Safety mechanism**: SM-001, SM-002
- **Allocation**: CVC (command), RZC (execution)
- **Status**: draft

The system shall reduce the motor torque request to zero within 10 ms of detecting a pedal sensor plausibility fault (FSR-001, FSR-002). The zero-torque command shall be latched (maintained) until all of the following conditions are met: (a) the pedal sensor fault has cleared, (b) both sensor readings are within the valid range, (c) the operator has performed a manual reset action.

**Rationale**: Zero-torque latching prevents unintended acceleration from an intermittent sensor fault that repeatedly toggles between faulty and apparently-normal readings. Manual reset ensures the operator is aware of the fault before resuming operation.

---

### FSR-004: Motor Current Monitoring and Overcurrent Cutoff

- **ASIL**: A
- **Traces up**: SG-006
- **Traces down**: TSR-006, TSR-007- **Safety mechanism**: SM-002, SM-016
- **Allocation**: RZC
- **Status**: draft

The system shall continuously monitor motor current via the ACS723 current sensor and disable the motor driver (both BTS7960 enable lines set to LOW) within 10 ms of detecting that motor current exceeds the maximum rated threshold (calibratable, default: 25A) for a continuous debounce period of 10 ms.

**Rationale**: Overcurrent can cause motor winding damage, overheating, fire, or BTS7960 driver failure. The ACS723 provides galvanically isolated current measurement. The BTS7960 built-in overcurrent protection provides a hardware-diverse backup. The 10 ms debounce prevents false triggers from inrush current during motor startup.

---

### FSR-005: Motor Temperature Monitoring and Derating

- **ASIL**: A
- **Traces up**: SG-006
- **Traces down**: TSR-008, TSR-009- **Safety mechanism**: SM-015
- **Allocation**: RZC
- **Status**: draft

The system shall continuously monitor motor temperature via NTC thermistors and apply a progressive current derating curve:

- Below 60 degrees C: 100% rated current permitted
- 60 to 79 degrees C: Maximum 75% rated current (enter DEGRADED mode)
- 80 to 99 degrees C: Maximum 50% rated current (enter LIMP mode)
- 100 degrees C and above: Motor disabled (enter SAFE_STOP)
- Sensor reading below -30 degrees C or above 150 degrees C: Motor disabled (sensor fault)

The derating shall include a 10 degree C hysteresis on recovery (motor re-enable requires temperature 10 degrees C below the activation threshold).

**Rationale**: Progressive derating extends the thermal operating envelope by reducing heat generation before reaching critical temperatures. The hysteresis prevents oscillation between derating levels at boundary temperatures. Sensor range checks detect open-circuit (reads very high) and short-circuit (reads very low) NTC faults.

---

### 4.2 Steering Safety (SG-003)

---

### FSR-006: Steering Angle Feedback Monitoring

- **ASIL**: D
- **Traces up**: SG-003
- **Traces down**: TSR-010, TSR-011- **Safety mechanism**: SM-008
- **Allocation**: FZC
- **Status**: draft

The system shall continuously monitor the steering angle feedback sensor (AS5048A) and detect a steering position fault when any of the following conditions persist for more than 50 ms:

1. The absolute difference between the commanded steering angle and the measured steering angle exceeds 5 degrees.
2. The measured steering angle is outside the valid mechanical range (-45 to +45 degrees).
3. The rate of change of the measured steering angle exceeds the maximum physical slew rate (360 degrees/second).
4. The SPI communication with the steering angle sensor fails (CRC error, timeout, or no response).

**Rationale**: Steering angle feedback is the primary means of detecting servo runaway, mechanical binding, or sensor failure. The 50 ms debounce prevents false detection from transient servo oscillation during normal position tracking. The 5-degree threshold accounts for servo settling time and mechanical compliance.

---

### FSR-007: Steering Return-to-Center on Fault

- **ASIL**: D
- **Traces up**: SG-003
- **Traces down**: TSR-012, TSR-013- **Safety mechanism**: SM-008
- **Allocation**: FZC
- **Status**: draft

The system shall command the steering servo to the center position (0 degrees) at a controlled rate (not exceeding the maximum rate limit) within 100 ms of detecting a steering fault (FSR-006). If the steering servo does not reach the center position within 200 ms after the return-to-center command is issued, the system shall disable the steering servo PWM output entirely.

**Rationale**: Return-to-center is the safe state for steering (SG-003). A controlled rate-limited return prevents the correction itself from being a hazard (sudden steering movement). The 200 ms fallback to PWM disable handles the case where the servo is mechanically stuck or the feedback sensor fault prevents position verification.

---

### FSR-008: Steering Rate Limiting

- **ASIL**: C
- **Traces up**: SG-003
- **Traces down**: TSR-014- **Safety mechanism**: SM-009, SM-010
- **Allocation**: FZC
- **Status**: draft

The system shall limit the rate of change of the steering angle command to a maximum of 30 degrees per second. Any steering command that would exceed this rate shall be clamped to the maximum rate. Additionally, the system shall enforce software steering angle limits of -45 to +45 degrees (with a 2-degree margin inside mechanical stops), clamping any command that exceeds these limits.

**Rationale**: Rate limiting prevents sudden steering inputs (from CAN corruption, software faults, or operator error) from causing loss of vehicle control. The rate limit of 30 degrees/second is sufficient for normal steering at the platform's maximum speed while preventing snap steering. The software angle limits prevent the servo from reaching mechanical end stops, which could cause stalling and overheating.

---

### 4.3 Braking Safety (SG-004, SG-005)

---

### FSR-009: Brake Command Monitoring

- **ASIL**: D
- **Traces up**: SG-004, SG-005
- **Traces down**: TSR-015, TSR-016- **Safety mechanism**: SM-011, SM-014
- **Allocation**: FZC
- **Status**: draft

The system shall monitor the brake command path and detect a brake system fault when any of the following conditions are detected:

1. The brake servo PWM output does not match the commanded value (verified via timer capture feedback).
2. The brake servo draws no current when a non-zero brake force is commanded (indicating servo disconnection or failure).
3. A brake command is received while the vehicle state machine is in INIT or OFF mode (unexpected brake command).
4. The brake command value is outside the valid range (0% to 100%).

On detection of a brake fault (conditions 1 or 2), the system shall immediately notify the CVC via CAN and request motor torque cutoff as a backup deceleration mechanism.

**Rationale**: Brake monitoring ensures that the brake actuator responds to commands as expected. PWM feedback and current draw provide independent verification of servo operation. State-based plausibility (condition 3) detects spurious CAN messages. Motor cutoff provides a secondary deceleration path when braking is lost.

---

### FSR-010: Auto-Brake on CAN Timeout

- **ASIL**: D
- **Traces up**: SG-004
- **Traces down**: TSR-017- **Safety mechanism**: SM-012
- **Allocation**: FZC
- **Status**: draft

The system shall autonomously apply maximum braking force if no valid brake command CAN message (passing E2E verification) is received from the CVC within a timeout period of 100 ms. The auto-brake shall remain applied until valid CAN communication is restored and the CVC explicitly commands brake release.

**Rationale**: CAN communication loss (bus fault, CVC failure, wiring fault) removes the operator's ability to command braking. The FZC must autonomously apply brakes to bring the vehicle to a stop. The 100 ms timeout balances false-trigger avoidance (normal CAN jitter is under 20 ms) against timely response. The auto-brake is latching — it does not release on a single valid message, requiring explicit CVC confirmation that the fault has been resolved.

---

### 4.4 Obstacle Detection Safety (SG-007)

---

### FSR-011: Lidar Distance Monitoring and Obstacle Detection

- **ASIL**: C
- **Traces up**: SG-007
- **Traces down**: TSR-018, TSR-019- **Safety mechanism**: SM-017
- **Allocation**: FZC
- **Status**: draft

The system shall continuously monitor the forward distance via the TFMini-S lidar sensor and implement a graduated response based on three distance thresholds:

1. **Warning zone** (calibratable, default: 100 cm): The system shall activate an audible warning (buzzer) and transmit a warning CAN message to the CVC.
2. **Braking zone** (calibratable, default: 50 cm): The system shall request speed reduction via CAN and apply partial braking.
3. **Emergency zone** (calibratable, default: 20 cm): The system shall request immediate motor cutoff via CAN and apply maximum braking force.

The distance thresholds shall be adjustable at compile time and shall maintain the relationship: emergency < braking < warning.

**Rationale**: Graduated response provides proportional reaction to obstacle proximity. Warning allows the operator to react. Partial braking provides automated assistance. Emergency braking is the last resort to prevent collision. Compile-time adjustability allows tuning for different demonstration environments.

---

### FSR-012: Lidar Sensor Plausibility Check

- **ASIL**: C
- **Traces up**: SG-007
- **Traces down**: TSR-020, TSR-021- **Safety mechanism**: SM-018
- **Allocation**: FZC
- **Status**: draft

The system shall perform the following plausibility checks on every lidar sensor reading:

1. **Range check**: The distance reading shall be within the valid sensor range (2 cm to 1200 cm). Readings outside this range shall be flagged as invalid.
2. **Stuck sensor detection**: If the distance reading does not change by more than 1 cm over 50 consecutive samples (500 ms at 100 Hz), the sensor shall be flagged as potentially stuck.
3. **Signal strength check**: Readings with signal strength below the minimum threshold (calibratable, default: 100) shall be discarded as unreliable.
4. **Timeout check**: If no valid UART frame is received within 100 ms, a sensor communication fault shall be declared.

On any plausibility failure, the system shall substitute a safe default distance of 0 cm (obstacle present) and log a diagnostic trouble code.

**Rationale**: The safe default of 0 cm (obstacle present) triggers protective braking (FSR-011 emergency zone), which is the fail-safe behavior. This is a fail-closed design: sensor failure is treated as "obstacle detected" rather than "no obstacle," ensuring the system errs on the side of caution.

---

### 4.5 Communication Safety (SG-001 through SG-008)

---

### FSR-013: CAN E2E Protection on Safety Messages

- **ASIL**: D
- **Traces up**: SG-001, SG-002, SG-003, SG-004, SG-005, SG-006, SG-007, SG-008
- **Traces down**: TSR-022, TSR-023, TSR-024- **Safety mechanism**: SM-004
- **Allocation**: CVC, FZC, RZC, SC
- **Status**: draft

The system shall protect all safety-critical CAN messages with an end-to-end (E2E) protection header consisting of:

1. **CRC-8** (polynomial 0x1D, SAE J1850): Computed over the data payload and data ID. Detects random bit errors with Hamming distance 4 for payloads up to 8 bytes.
2. **Alive counter** (4-bit, 0-15, incrementing by 1 per transmission): The receiver shall verify that the alive counter increments by exactly 1 (modulo 16) between consecutive receptions. A gap greater than 1 indicates message loss; a repeated value indicates message duplication.
3. **Data ID** (8-bit, unique per message type): Prevents message masquerading (one message type being misinterpreted as another).

On E2E verification failure, the receiver shall discard the corrupted message and use the last valid value for up to 1 additional cycle. If E2E verification fails for 3 consecutive cycles, the receiver shall substitute a safe default value (zero torque, full brake, center steering).

Safety-critical CAN messages include: pedal position, torque request, torque status, steering command, steering feedback, brake command, brake status, motor current, motor temperature, vehicle state, E-stop, heartbeat.

**Rationale**: CAN 2.0B does not provide application-layer data integrity beyond the CAN CRC (which covers the frame only, not application semantics). E2E protection per AUTOSAR E2E Profile 1 detects corruption, repetition, loss, delay, insertion, and masquerading — all fault models identified in ISO 26262-4 for communication channels.

---

### 4.6 Independent Monitoring (SG-008)

---

### FSR-014: Heartbeat Transmission by All Zone ECUs

- **ASIL**: C
- **Traces up**: SG-008
- **Traces down**: TSR-025, TSR-026- **Safety mechanism**: SM-019
- **Allocation**: CVC, FZC, RZC
- **Status**: draft

Each zone ECU (CVC, FZC, RZC) shall transmit a heartbeat CAN message at a fixed interval of 50 ms (tolerance: +/- 5 ms). The heartbeat message shall include the ECU identifier, the alive counter, the current operating mode, and a CRC-8 for E2E protection.

**Rationale**: Heartbeat messages enable the Safety Controller to detect ECU failures (processor hang, CAN transceiver fault, power loss). The 50 ms interval provides detection within 150 ms (3 missed heartbeats). The operating mode field enables the SC to verify that all ECUs are in the expected state.

---

### FSR-015: Heartbeat Timeout Detection by Safety Controller

- **ASIL**: C
- **Traces up**: SG-008
- **Traces down**: TSR-027, TSR-028- **Safety mechanism**: SM-019
- **Allocation**: SC
- **Status**: draft

The Safety Controller shall independently monitor the heartbeat CAN messages from each zone ECU (CVC, FZC, RZC) and detect a heartbeat timeout when no valid heartbeat message (passing E2E check) is received from any single ECU within 150 ms (3 times the heartbeat interval). On heartbeat timeout detection, the SC shall:

1. Illuminate the fault LED corresponding to the failed ECU.
2. Wait for a confirmation period of 50 ms (to avoid false triggers from CAN bus contention).
3. If the heartbeat is still absent after the confirmation period, de-energize the kill relay (open circuit), forcing the system into the safe state.

**Rationale**: The 150 ms timeout (3 missed heartbeats) provides sufficient margin for CAN bus scheduling jitter while enabling timely detection of ECU failure. The 50 ms confirmation period reduces false-positive relay trips caused by transient CAN bus overload. The SC operates in CAN listen-only mode and cannot corrupt the bus.

---

### FSR-016: Kill Relay Activation on Safety Goal Violation

- **ASIL**: D
- **Traces up**: SG-001, SG-003, SG-004, SG-008
- **Traces down**: TSR-029, TSR-030- **Safety mechanism**: SM-005
- **Allocation**: SC
- **Status**: draft

The Safety Controller shall de-energize the kill relay (open circuit, removing 12V power from motor driver and servo circuits) within 5 ms of any of the following conditions being confirmed:

1. Heartbeat timeout from any zone ECU (after confirmation period per FSR-015).
2. Cross-plausibility fault between torque request and actual motor current (SM-003).
3. SC self-test failure during startup or runtime.
4. SC lockstep CPU core comparison error (hardware-detected).
5. SC external watchdog timeout (TPS3823 reset — implies SC firmware hang that survived lockstep detection).

The kill relay shall use an energize-to-run configuration: the relay coil must be actively held energized to allow power flow. Any loss of SC control (power loss, MCU hang, GPIO fault) inherently results in relay de-energization (safe state). The relay shall not be re-energized without a complete system power cycle and successful startup self-test.

**Rationale**: The kill relay is the ultimate hardware-enforced safety boundary. Its energize-to-run design ensures that any SC failure — including complete power loss — results in the safe state. The 5 ms reaction time is achievable because relay de-energization requires only de-asserting a GPIO pin (the relay dropout time is the limiting factor). The re-energization lockout prevents automated recovery from a potentially dangerous state.

---

### FSR-017: External Watchdog Monitoring Per ECU

- **ASIL**: D
- **Traces up**: SG-008
- **Traces down**: TSR-031, TSR-032- **Safety mechanism**: SM-020, SM-021
- **Allocation**: CVC, FZC, RZC, SC
- **Status**: draft

Each physical ECU (CVC, FZC, RZC, SC) shall be monitored by an external watchdog IC (TPS3823). The ECU firmware shall toggle the watchdog input (WDI) pin within the watchdog timeout period (configured via external capacitor, default: 1.6 seconds). The watchdog toggle shall occur only when the following conditions are met:

1. The main control loop has completed at least one full cycle.
2. All critical runtime self-checks have passed (stack canary intact, RAM test passed, CAN controller operational).

If the firmware fails to toggle the WDI pin (processor hang, infinite loop, stack overflow, priority inversion), the TPS3823 shall assert its RESET output, forcing a hardware reset of the MCU.

Additionally, the Safety Controller (TMS570LC43x) shall rely on its hardware lockstep CPU cores as the primary computation error detection mechanism, with the external watchdog providing backup detection for faults not caught by lockstep (e.g., clock failure affecting both cores simultaneously).

**Rationale**: The external watchdog is a hardware-independent mechanism that detects firmware hangs not caught by software-level monitoring. By conditioning the watchdog toggle on successful self-check completion, the watchdog also detects certain classes of logical faults (data corruption, stack overflow) rather than only detecting complete processor hang. The TPS3823 is a separate IC with its own oscillator, providing true independence from the MCU's clock domain.

---

### 4.7 Emergency Stop (SG-001, SG-008)

---

### FSR-018: E-Stop Broadcast and Immediate Motor Cutoff

- **ASIL**: B
- **Traces up**: SG-001, SG-008
- **Traces down**: TSR-033, TSR-034- **Safety mechanism**: SM-023
- **Allocation**: CVC (detection), FZC and RZC (reaction)
- **Status**: draft

The system shall detect E-stop button activation within 1 ms (via hardware interrupt on GPIO falling edge with hardware debounce). On E-stop detection, the CVC shall:

1. Immediately set the local torque request to zero.
2. Broadcast a high-priority E-stop CAN message (CAN ID 0x001, highest priority on the bus) within 1 ms of detection.
3. Transition the vehicle state machine to SAFE_STOP.

All receiving ECUs shall react to the E-stop CAN message within 10 ms:
- RZC: Disable motor driver (both enable lines LOW), set PWM duty to 0%.
- FZC: Apply maximum braking force, command steering to center, activate continuous buzzer.

The system shall remain in SAFE_STOP until the E-stop button is released AND a manual restart sequence is performed (ignition cycle).

**Rationale**: The E-stop is the operator's last resort for immediate system shutdown. Hardware interrupt detection ensures minimum latency. The highest-priority CAN ID (0x001) ensures the E-stop message is transmitted even under high bus load. The latching behavior (requires restart) prevents accidental resumption.

---

### 4.8 System State Management (SG-001 through SG-008)

---

### FSR-019: Vehicle State Machine Management

- **ASIL**: D
- **Traces up**: SG-001, SG-002, SG-003, SG-004, SG-005, SG-006, SG-007, SG-008
- **Traces down**: TSR-035, TSR-036, TSR-037- **Safety mechanism**: SM-022
- **Allocation**: CVC (primary), all ECUs (secondary via BswM)
- **Status**: draft

The CVC shall maintain a deterministic vehicle state machine with the following states: INIT, RUN, DEGRADED, LIMP, SAFE_STOP, SHUTDOWN. The state machine shall enforce the following rules:

1. **Valid transitions only**: The state machine shall reject any transition not defined in the transition table. Only the following forward transitions are permitted: INIT to RUN (self-test passed), RUN to DEGRADED (minor fault), DEGRADED to LIMP (persistent fault), LIMP to SAFE_STOP (critical fault), SAFE_STOP to SHUTDOWN (controlled power-down). The only reverse transition is DEGRADED to RUN (all faults cleared and recovery timer expired).
2. **E-stop override**: The E-stop transitions from any state directly to SAFE_STOP.
3. **SC override**: The SC can force SHUTDOWN from any state (kill relay de-energize).
4. **State broadcast**: The current vehicle state shall be broadcast on CAN every 10 ms, with E2E protection.
5. **State persistence**: The state machine state shall survive a controlled reset (stored in non-volatile memory) to prevent repeated self-test cycles from masking persistent faults.

Each state shall define the maximum permitted operational limits (torque, speed, steering range, steering rate) as specified in the FSC degradation concept (Section 5.2).

**Rationale**: A deterministic state machine with restricted transitions prevents the system from entering an undefined or unsafe operating mode. State persistence prevents "reset-washing" of faults. The E-stop and SC overrides provide independent paths to the safe state regardless of the software state.

---

### FSR-020: CAN Bus Loss Detection

- **ASIL**: C
- **Traces up**: SG-008
- **Traces down**: TSR-038, TSR-039- **Safety mechanism**: SM-004, SM-007
- **Allocation**: CVC, FZC, RZC, SC
- **Status**: draft

Each ECU shall detect loss of CAN bus communication (bus-off condition, no messages received for more than 200 ms, or CAN error counter exceeding the warning threshold) and take the following actions:

1. **CVC**: Transition to SAFE_STOP, disable torque output, attempt bus recovery (automatic retransmission per CAN 2.0B protocol).
2. **FZC**: Apply auto-brake (per FSR-010), command steering to center (per FSR-007).
3. **RZC**: Disable motor driver, set torque to zero.
4. **SC**: After heartbeat timeout confirmation (per FSR-015), de-energize kill relay.

Each ECU shall independently handle CAN loss without relying on commands from other ECUs.

**Rationale**: CAN bus is a single point of failure in this architecture (no redundant bus). Each ECU must be capable of autonomous safe-state transition when CAN communication is lost. The 200 ms timeout is chosen to be longer than any individual message timeout (avoiding false triggers from single message loss) while still providing timely detection.

---

### 4.9 Motor Direction Safety (SG-001)

---

### FSR-021: Motor Direction Control Plausibility

- **ASIL**: C
- **Traces up**: SG-001
- **Traces down**: TSR-040- **Safety mechanism**: SM-002
- **Allocation**: RZC
- **Status**: draft

The system shall verify that the motor rotation direction (as determined by the BTS7960 RPWM/LPWM signals) matches the commanded direction (as received via CAN from CVC). If the actual direction (determined by encoder feedback or back-EMF polarity) does not match the commanded direction within 50 ms of the command, the system shall disable the motor driver and log a DTC.

Additionally, the system shall prevent simultaneous activation of both RPWM and LPWM (shoot-through protection), enforcing a minimum dead-time of 10 microseconds between direction changes.

**Rationale**: Incorrect motor direction could cause unintended vehicle motion. The shoot-through protection prevents damage to the BTS7960 H-bridge and potential short-circuit conditions. Direction verification using encoder feedback provides independent confirmation of motor behavior.

---

### FSR-022: Cross-Plausibility — Torque Request vs. Actual Current

- **ASIL**: C
- **Traces up**: SG-001
- **Traces down**: TSR-041, TSR-042- **Safety mechanism**: SM-003
- **Allocation**: SC
- **Status**: draft

The Safety Controller shall continuously compare the torque request (CAN message from CVC) against the actual motor current (CAN message from RZC) using a calibratable torque-to-current lookup table. The SC shall detect a cross-plausibility fault when the measured current deviates from the expected current (based on the torque request) by more than 20% (calibratable threshold) for a continuous period of 50 ms.

On cross-plausibility fault detection, the SC shall de-energize the kill relay (per FSR-016).

**Rationale**: Cross-plausibility provides an independent, diverse check on the motor control path. It detects faults that are not caught by the CVC's pedal sensor monitoring (SM-001) or the RZC's current monitoring (SM-002), such as: software faults in torque calculation, CAN message corruption that passes E2E checks (extremely unlikely but non-zero probability), or BTS7960 driver faults that cause current without a corresponding torque request. The SC's independence (different vendor, different architecture, listen-only CAN) ensures this check cannot be defeated by a common-cause fault affecting the STM32 ECUs.

---

### 4.10 Operator Warning (SG-001 through SG-008)

---

### FSR-023: Operator Warning on Degradation

- **ASIL**: B
- **Traces up**: SG-001, SG-002, SG-003, SG-004, SG-005, SG-006, SG-007, SG-008
- **Traces down**: TSR-043, TSR-044, TSR-045- **Safety mechanism**: SM-022
- **Allocation**: CVC (OLED), FZC (buzzer), SC (fault LEDs)
- **Status**: draft

The system shall warn the operator within 200 ms of any transition from NORMAL operating mode to a degraded mode (DEGRADED, LIMP, SAFE_STOP, or SHUTDOWN) using at least two independent warning channels:

1. **Visual — OLED display (CVC)**: Display the current operating state, the triggering fault code, and the applicable operational restrictions (speed limit, torque limit).
2. **Audible — buzzer (FZC)**: Emit a warning pattern corresponding to the severity level (single beep for DEGRADED, slow repeating beep for LIMP, fast repeating beep for SAFE_STOP, continuous tone for SHUTDOWN/emergency).
3. **Visual — fault LEDs (SC)**: Illuminate the LED corresponding to the ECU where the fault originated. This channel is independent of CAN (SC drives LEDs directly via GPIO).

At least one warning channel (SC fault LEDs) shall be independent of the CAN bus, ensuring the operator is warned even in the event of total CAN bus failure.

**Rationale**: ISO 26262-3 Clause 8.4.5 requires that the driver (operator) be warned of degraded operation so they can adapt their behavior. Two independent channels provide redundancy in the warning path. The SC fault LEDs provide a CAN-independent warning for the worst case (complete bus failure).

---

### 4.11 Safe State Timing (SG-001 through SG-008)

---

### FSR-024: Safe State Transition Within FTTI

- **ASIL**: D
- **Traces up**: SG-001, SG-002, SG-003, SG-004, SG-005, SG-006, SG-007, SG-008
- **Traces down**: TSR-046, TSR-047- **Safety mechanism**: SM-001 through SM-023 (all)
- **Allocation**: All ECUs
- **Status**: draft

The system shall achieve the safe state defined for each safety goal within the Fault Tolerant Time Interval (FTTI) specified for that safety goal:

| Safety Goal | Safe State | FTTI | Primary Mechanism | Mechanism Total Time |
|-------------|------------|------|--------------------|--------------------|
| SG-001 | Motor off, brakes applied | 50 ms | SM-001 (pedal plausibility) | 30 ms |
| SG-002 | Controlled stop | 200 ms | SM-006 (motor health) | 70 ms |
| SG-003 | Steer to center, speed reduced | 100 ms | SM-008 (steering feedback) | 100 ms |
| SG-004 | Motor off (fail-safe) | 50 ms | SM-011 (brake monitoring) | 30 ms |
| SG-005 | Release brake | 200 ms | SM-014 (brake plausibility) | 20 ms |
| SG-006 | Motor off | 500 ms | SM-015 (temp derating) | 110 ms |
| SG-007 | Controlled stop | 200 ms | SM-017 (lidar distance) | 20 ms |
| SG-008 | System shutdown | 100 ms | SM-021 (lockstep) | < 1 ms |

For each safety goal, the total time from fault occurrence to safe state achievement (detection time + reaction time + actuation time) shall not exceed the FTTI. The system design shall provide a timing margin of at least 10% where achievable.

**Rationale**: FTTI compliance is the fundamental timing requirement for functional safety. The table above demonstrates that each safety goal has at least one primary mechanism that achieves the safe state within the FTTI. Secondary and tertiary mechanisms (which may exceed FTTI) provide defense in depth for faults not caught by the primary mechanism.

---

### 4.12 Loss of Braking (SG-004)

---

### FSR-025: Loss of Braking Detection and Motor Cutoff

- **ASIL**: D
- **Traces up**: SG-004
- **Traces down**: TSR-048, TSR-049- **Safety mechanism**: SM-011, SM-013
- **Allocation**: FZC (detection), CVC (command), RZC (execution), SC (backup)
- **Status**: draft

The system shall detect loss of braking capability (brake servo fault per FSR-009 conditions 1 or 2) and initiate motor cutoff as a backup deceleration mechanism within 30 ms of fault detection:

1. **FZC** detects brake servo fault and broadcasts a "brake fault" CAN message.
2. **CVC** receives the brake fault message and commands RZC to set torque to zero.
3. **RZC** disables the motor driver (both enable lines LOW).
4. If CAN communication to RZC fails, the **SC** heartbeat monitoring (FSR-015) and kill relay (FSR-016) provide a backup path to motor cutoff.

The system shall transition to SAFE_STOP and shall not resume normal operation until the brake system is verified functional.

**Rationale**: On a DC motor-driven platform, motor cutoff provides a secondary deceleration path through back-EMF and mechanical friction. While not equivalent to active braking, it prevents continued acceleration when braking is lost. The four-layer response (FZC detection, CVC coordination, RZC execution, SC backup) ensures motor cutoff even if multiple communication paths fail.

---

## 5. Requirements Traceability Summary

### 5.1 Safety Goal to FSR Mapping

| Safety Goal | ASIL | FSRs |
|-------------|------|------|
| SG-001 (Unintended acceleration) | D | FSR-001, FSR-002, FSR-003, FSR-013, FSR-016, FSR-018, FSR-019, FSR-021, FSR-022, FSR-024 |
| SG-002 (Loss of drive torque) | B | FSR-013, FSR-019, FSR-023, FSR-024 |
| SG-003 (Unintended steering) | D | FSR-006, FSR-007, FSR-008, FSR-013, FSR-016, FSR-019, FSR-024 |
| SG-004 (Loss of braking) | D | FSR-009, FSR-010, FSR-013, FSR-016, FSR-019, FSR-024, FSR-025 |
| SG-005 (Unintended braking) | A | FSR-009, FSR-013, FSR-019, FSR-024 |
| SG-006 (Motor protection) | A | FSR-004, FSR-005, FSR-013, FSR-019, FSR-024 |
| SG-007 (Obstacle detection) | C | FSR-011, FSR-012, FSR-013, FSR-019, FSR-024 |
| SG-008 (Independent monitoring) | C | FSR-013, FSR-014, FSR-015, FSR-016, FSR-017, FSR-018, FSR-019, FSR-020, FSR-024 |

### 5.2 FSR to ECU Allocation Summary

| ECU | Allocated FSRs |
|-----|---------------|
| CVC (STM32G474RE) | FSR-001, FSR-002, FSR-003, FSR-013, FSR-014, FSR-017, FSR-018, FSR-019, FSR-020, FSR-023, FSR-024, FSR-025 |
| FZC (STM32G474RE) | FSR-006, FSR-007, FSR-008, FSR-009, FSR-010, FSR-011, FSR-012, FSR-013, FSR-014, FSR-017, FSR-020, FSR-023, FSR-024, FSR-025 |
| RZC (STM32G474RE) | FSR-003, FSR-004, FSR-005, FSR-013, FSR-014, FSR-017, FSR-020, FSR-021, FSR-024, FSR-025 |
| SC (TMS570LC43x) | FSR-013, FSR-015, FSR-016, FSR-017, FSR-020, FSR-022, FSR-023, FSR-024 |

### 5.3 ASIL Distribution

| ASIL | Count | FSRs |
|------|-------|------|
| D | 13 | FSR-001, FSR-002, FSR-003, FSR-006, FSR-007, FSR-009, FSR-010, FSR-013, FSR-016, FSR-017, FSR-019, FSR-024, FSR-025 |
| C | 7 | FSR-008, FSR-011, FSR-012, FSR-014, FSR-015, FSR-020, FSR-021, FSR-022 |
| B | 2 | FSR-018, FSR-023 |
| A | 2 | FSR-004, FSR-005 |
| **Total** | **25** | |

## 6. Open Items and Assumptions

### 6.1 Assumptions

| ID | Assumption | Impact |
|----|-----------|--------|
| FSR-A-001 | Calibratable thresholds (pedal, steering, current, temperature) will be determined during integration testing | Default values are engineering estimates; final values require hardware characterization |
| FSR-A-002 | CAN message cycle times (10 ms for control, 50 ms for heartbeat) are achievable with the specified bus load | CAN bus utilization analysis needed during TSC phase |
| FSR-A-003 | The TPS3823 watchdog timeout (1.6 s) is appropriate for the application | Timeout value determined by external capacitor; confirmed during hardware integration |
| FSR-A-004 | MPU-enforced memory isolation is correctly configured on all STM32G474 ECUs | Required for FFI between ASIL D and QM tasks |

### 6.2 Open Items

| ID | Item | Owner | Target |
|----|------|-------|--------|
| FSR-O-001 | Derive Technical Safety Requirements (TSR) from each FSR | System Engineer | TSC phase |
| FSR-O-002 | Validate FTTI compliance for all safety mechanisms on target hardware | Test Engineer | Integration testing |
| FSR-O-003 | Define CAN message schedule (IDs, priorities, cycle times, bus utilization) | System Architect | TSC phase |
| FSR-O-004 | Characterize calibratable thresholds on target hardware | Integration Engineer | Hardware integration |
| FSR-O-005 | Perform ASIL decomposition analysis for FSRs where applicable | FSE | Safety analysis phase |
| FSR-O-006 | Complete independent review of all ASIL D FSRs (ISO 26262-2 confirmation measures) | Independent Reviewer | Before TSR derivation |

## 7. Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 0.1 | 2026-02-21 | System | Initial stub |
| 1.0 | 2026-02-21 | System | Complete FSR specification: 25 requirements (FSR-001 to FSR-025), traceability matrices, ASIL allocation |
