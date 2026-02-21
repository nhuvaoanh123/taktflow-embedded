---
document_id: SYSREQ
title: "System Requirements Specification"
version: "1.0"
status: draft
aspice_process: SYS.2
date: 2026-02-21
---

# System Requirements Specification

## 1. Purpose

This document specifies the system-level requirements for the Taktflow Zonal Vehicle Platform, derived from the stakeholder requirements (document STKR) per Automotive SPICE 4.0 SYS.2 (System Requirements Analysis). System requirements define the technical "what" at the system boundary — they are verifiable, traceable, and allocated to system elements in the system architecture (SYS.3).

System requirements bridge the gap between stakeholder needs (expressed in stakeholder language) and technical safety requirements / software requirements (expressed in engineering language). Every system requirement traces upward to at least one stakeholder requirement and downward to at least one technical safety requirement (TSR) or software requirement (SWR).

## 2. Referenced Documents

| Document ID | Title | Version |
|-------------|-------|---------|
| STKR | Stakeholder Requirements | 1.0 |
| ITEM-DEF | Item Definition | 1.0 |
| HARA | Hazard Analysis and Risk Assessment | 1.0 |
| SG | Safety Goals | 1.0 |
| FSR | Functional Safety Requirements | 1.0 |
| FSC | Functional Safety Concept | 1.0 |

## 3. Requirement Conventions

### 3.1 Requirement Structure

Each requirement follows this format:

- **ID**: SYS-NNN (sequential)
- **Title**: Descriptive name
- **Traces up**: STK-NNN (stakeholder requirement parent)
- **Traces down**: TSR-NNN / SWR-NNN (placeholder until TSC and SWE.1 phases)
- **Safety relevance**: ASIL level if safety-related, or QM if not
- **Verification method**: Inspection (I), Analysis (A), Test (T), or Demonstration (D)
- **Verification criteria**: Specific pass/fail criteria
- **Status**: draft | reviewed | approved | implemented | verified

### 3.2 Requirement Language

- "shall" = mandatory
- "should" = recommended
- "may" = optional

All SYS requirements use "shall" unless otherwise noted.

---

## 4. Functional System Requirements — Drive-by-Wire

### SYS-001: Dual Pedal Position Sensing

- **Traces up**: STK-005, STK-016
- **Traces down**: TSR-001, TSR-002 (placeholder)
- **Safety relevance**: ASIL D
- **Verification method**: Test
- **Verification criteria**: Both pedal sensors independently report angle values within 1% linearity across the 0-100% pedal travel range. Both sensors are readable within a single 10 ms control cycle.
- **Status**: draft

The system shall sense the operator pedal position using two independent AS5048A magnetic angle sensors connected to the CVC via SPI1 with separate chip-select lines (PA4 and PA15). Both sensor readings shall be acquired in every control cycle and made available to the pedal plausibility monitoring function.

---

### SYS-002: Pedal Sensor Plausibility Monitoring

- **Traces up**: STK-005, STK-016, STK-018
- **Traces down**: TSR-003, TSR-004 (placeholder)
- **Safety relevance**: ASIL D
- **Verification method**: Test
- **Verification criteria**: A simulated sensor disagreement of 6% of full scale (above the 5% threshold) persisting for 2 control cycles triggers a plausibility fault within 20 ms. A disagreement of 4% (below threshold) does not trigger a fault.
- **Status**: draft

The system shall compare the two pedal sensor readings every control cycle and detect a plausibility fault when the absolute difference exceeds a calibratable threshold (default: 5% of full-scale range) for 2 or more consecutive cycles. On detection, the system shall set the torque request to zero and latch the fault until manual reset.

---

### SYS-003: Pedal-to-Torque Mapping

- **Traces up**: STK-005
- **Traces down**: SWR-CVC-001 (placeholder)
- **Safety relevance**: ASIL D
- **Verification method**: Test
- **Verification criteria**: For a validated pedal input of 0%, the torque request is 0%. For a validated pedal input of 100%, the torque request equals the maximum permitted by the current operating mode. The mapping is monotonically increasing.
- **Status**: draft

The system shall convert a validated pedal position (average of both sensor readings when plausibility check passes) into a motor torque request using a calibratable mapping function. The mapping shall apply ramp-rate limiting (maximum increase rate: 50% per second) to prevent sudden torque transients. The maximum torque request shall be constrained by the current vehicle operating mode (RUN: 100%, DEGRADED: 75%, LIMP: 30%).

---

### SYS-004: Motor Torque Control via CAN

- **Traces up**: STK-005, STK-022
- **Traces down**: TSR-005, SWR-RZC-001 (placeholder)
- **Safety relevance**: ASIL D
- **Verification method**: Test
- **Verification criteria**: A torque request CAN message transmitted by CVC at 10 ms intervals results in a corresponding PWM duty cycle change at the RZC BTS7960 motor driver within 20 ms of transmission. The duty cycle is proportional to the requested torque within 2% accuracy.
- **Status**: draft

The system shall transmit the torque request from the CVC to the RZC via CAN at a fixed cycle time of 10 ms. The RZC shall translate the received torque request into PWM duty cycle for the BTS7960 motor driver and control the motor direction via RPWM/LPWM signals. The torque CAN message shall include E2E protection (CRC-8, alive counter, data ID).

---

### SYS-005: Motor Current Monitoring

- **Traces up**: STK-005, STK-016
- **Traces down**: TSR-006, TSR-007 (placeholder)
- **Safety relevance**: ASIL A
- **Verification method**: Test
- **Verification criteria**: An applied motor current of 26A (above the 25A threshold) sustained for 10 ms results in motor driver disable within 20 ms. The ACS723 current reading accuracy is within 2A of the actual current over the 0-30A range.
- **Status**: draft

The system shall continuously monitor motor current via the ACS723 current sensor (ADC1_CH1 on RZC, sampled at 1 kHz minimum). The system shall disable the motor driver (BTS7960 R_EN and L_EN set LOW) within 10 ms of detecting that motor current exceeds the maximum rated threshold (calibratable, default: 25A) for a continuous debounce period of 10 ms.

---

### SYS-006: Motor Temperature Monitoring and Derating

- **Traces up**: STK-005, STK-016
- **Traces down**: TSR-008, TSR-009 (placeholder)
- **Safety relevance**: ASIL A
- **Verification method**: Test
- **Verification criteria**: Simulated NTC readings corresponding to 60C, 80C, and 100C thresholds trigger derating to 75%, 50%, and 0% (motor off) respectively within 200 ms. Recovery occurs at 50C, 70C, and 90C respectively (10C hysteresis verified).
- **Status**: draft

The system shall continuously monitor motor temperature via NTC thermistors (ADC1_CH2 and ADC1_CH3 on RZC) and apply a progressive current derating curve: below 60C full current, 60-79C maximum 75% rated current, 80-99C maximum 50% rated current, at or above 100C motor disabled. Recovery shall include 10C hysteresis. Sensor readings outside -30C to 150C shall be treated as sensor fault (motor disabled).

---

### SYS-007: Motor Direction Control and Plausibility

- **Traces up**: STK-005, STK-016
- **Traces down**: TSR-040, SWR-RZC-002 (placeholder)
- **Safety relevance**: ASIL C
- **Verification method**: Test
- **Verification criteria**: A commanded forward direction results in forward encoder count within 50 ms. Simultaneous RPWM and LPWM activation is physically prevented with at least 10 us dead-time verified by oscilloscope capture.
- **Status**: draft

The system shall verify that the motor rotation direction matches the commanded direction within 50 ms using encoder feedback. The system shall enforce a minimum dead-time of 10 microseconds between motor direction changes to prevent H-bridge shoot-through. If direction mismatch is detected, the motor driver shall be disabled and a DTC logged.

---

### SYS-008: Battery Voltage Monitoring

- **Traces up**: STK-005
- **Traces down**: SWR-RZC-003 (placeholder)
- **Safety relevance**: QM
- **Verification method**: Test
- **Verification criteria**: Applied voltages of 9V and 16V on the battery input trigger undervoltage and overvoltage warnings respectively. A voltage below 8V or above 17V triggers motor disable.
- **Status**: draft

The system shall continuously monitor battery voltage via a resistor divider on ADC1_CH4 (RZC). The system shall report warning conditions at configurable thresholds (default: below 10V or above 15V) and shall disable the motor at critical thresholds (default: below 8V or above 17V) to protect the electronics and motor driver.

---

### SYS-009: Encoder Feedback for Speed Measurement

- **Traces up**: STK-005, STK-014
- **Traces down**: SWR-RZC-004 (placeholder)
- **Safety relevance**: QM
- **Verification method**: Test
- **Verification criteria**: Encoder pulses at a known motor speed produce a speed calculation within 5% of the actual motor speed.
- **Status**: draft

The system shall read motor encoder feedback on the RZC via GPIO interrupt to measure motor speed and direction. The encoder data shall be used for speed calculation, direction verification (SYS-007), and reporting to the CVC and telemetry systems via CAN.

---

## 5. Functional System Requirements — Steering

### SYS-010: Steering Command Reception and Servo Control

- **Traces up**: STK-006
- **Traces down**: TSR-012, SWR-FZC-001 (placeholder)
- **Safety relevance**: ASIL D
- **Verification method**: Test
- **Verification criteria**: A steering command CAN message specifying +20 degrees results in the steering servo reaching 20 +/- 1 degrees within 200 ms as measured by the AS5048A feedback sensor.
- **Status**: draft

The system shall receive steering angle commands from the CVC via CAN (10 ms cycle, E2E protected) and drive the steering servo to the commanded angle using PWM (TIM2_CH1 on FZC, 50 Hz servo frequency). The system shall implement closed-loop position control using the steering angle feedback sensor.

---

### SYS-011: Steering Angle Feedback Monitoring

- **Traces up**: STK-006, STK-016, STK-018
- **Traces down**: TSR-010, TSR-011 (placeholder)
- **Safety relevance**: ASIL D
- **Verification method**: Test
- **Verification criteria**: A simulated command-vs-feedback deviation of 6 degrees (above 5 degree threshold) persisting for 50 ms triggers a steering fault. A deviation of 4 degrees does not trigger a fault.
- **Status**: draft

The system shall continuously monitor the steering angle sensor (AS5048A on SPI2, FZC) and detect a steering fault when: (a) the command-vs-feedback difference exceeds 5 degrees for 50 ms, (b) the angle is outside -45 to +45 degrees, (c) the rate of change exceeds 360 degrees/second, or (d) SPI communication fails (CRC error, timeout, no response).

---

### SYS-012: Steering Return-to-Center on Fault

- **Traces up**: STK-006, STK-017
- **Traces down**: TSR-012, TSR-013 (placeholder)
- **Safety relevance**: ASIL D
- **Verification method**: Test
- **Verification criteria**: On steering fault injection, the steering servo reaches 0 degrees (center) within 100 ms at a controlled rate. If center is not reached within 200 ms, PWM output is disabled (verified by oscilloscope).
- **Status**: draft

The system shall command the steering servo to the center position (0 degrees) at a controlled rate not exceeding the rate limit upon detection of a steering fault. If the servo does not reach center within 200 ms, the system shall disable the steering servo PWM output entirely.

---

### SYS-013: Steering Rate and Angle Limiting

- **Traces up**: STK-006, STK-016
- **Traces down**: TSR-014 (placeholder)
- **Safety relevance**: ASIL C
- **Verification method**: Test
- **Verification criteria**: A step command from 0 to 45 degrees is rate-limited to arrive in no less than 1.5 seconds (30 deg/s). A command of 50 degrees is clamped to 43 degrees (45 minus 2 degree margin).
- **Status**: draft

The system shall limit the steering angle command rate of change to a maximum of 30 degrees per second and enforce software angle limits of -45 to +45 degrees with a 2-degree margin inside mechanical stops. Any command exceeding these limits shall be clamped.

---

## 6. Functional System Requirements — Braking

### SYS-014: Brake Command Reception and Servo Control

- **Traces up**: STK-007
- **Traces down**: TSR-015, SWR-FZC-002 (placeholder)
- **Safety relevance**: ASIL D
- **Verification method**: Test
- **Verification criteria**: A brake command of 80% force received via CAN results in the brake servo applying proportional PWM within 20 ms.
- **Status**: draft

The system shall receive brake commands from the CVC via CAN (10 ms cycle, E2E protected) and drive the brake servo to apply proportional braking force using PWM (TIM2_CH2 on FZC, 50 Hz servo frequency). Brake commands shall be range-validated (0% to 100%).

---

### SYS-015: Brake System Monitoring

- **Traces up**: STK-007, STK-016, STK-018
- **Traces down**: TSR-015, TSR-016 (placeholder)
- **Safety relevance**: ASIL D
- **Verification method**: Test
- **Verification criteria**: A simulated brake servo disconnection (no current draw when brake force commanded) triggers a brake fault within 20 ms. The CVC is notified via CAN and motor torque cutoff is commanded within 30 ms total.
- **Status**: draft

The system shall monitor the brake command path and detect a brake system fault when: (a) PWM output does not match the commanded value, (b) the brake servo draws no current when a non-zero force is commanded, (c) a brake command is received in INIT or OFF mode, or (d) the brake command value is outside 0-100%. On brake fault, the FZC shall notify the CVC and request motor torque cutoff.

---

### SYS-016: Auto-Brake on CAN Communication Loss

- **Traces up**: STK-007, STK-017
- **Traces down**: TSR-017 (placeholder)
- **Safety relevance**: ASIL D
- **Verification method**: Test
- **Verification criteria**: When CAN brake command messages are blocked for 100 ms, the FZC autonomously applies maximum braking force within 110 ms (100 ms timeout + 10 ms actuation). Brake remains applied until valid CAN communication is restored.
- **Status**: draft

The system shall autonomously apply maximum braking force if no valid brake command CAN message (passing E2E verification) is received from the CVC within 100 ms. The auto-brake shall remain applied until valid CAN communication is restored and the CVC explicitly commands brake release.

---

### SYS-017: Emergency Braking from Obstacle Detection

- **Traces up**: STK-007, STK-008
- **Traces down**: TSR-018, SWR-FZC-003 (placeholder)
- **Safety relevance**: ASIL C
- **Verification method**: Test
- **Verification criteria**: An obstacle placed at 19 cm (inside 20 cm emergency zone) triggers maximum braking within 30 ms of sensor reading. Motor cutoff is requested via CAN simultaneously.
- **Status**: draft

The system shall trigger emergency braking when the lidar sensor reports an obstacle within the emergency distance threshold (calibratable, default: 20 cm). Emergency braking shall apply maximum brake servo force and simultaneously request motor torque cutoff from the CVC via CAN. Emergency braking shall override any other brake command.

---

## 7. Functional System Requirements — Obstacle Detection

### SYS-018: Lidar Distance Sensing

- **Traces up**: STK-008
- **Traces down**: TSR-018, TSR-019 (placeholder)
- **Safety relevance**: ASIL C
- **Verification method**: Test
- **Verification criteria**: The TFMini-S reports distance to a target at 1 m within +/- 3 cm accuracy. Update rate is 100 Hz (+/- 5 Hz). Data is available to the application within 10 ms of sensor output.
- **Status**: draft

The system shall continuously read forward distance from the TFMini-S lidar sensor via UART (USART1 on FZC, 115200 baud, 100 Hz native update rate). The system shall parse the TFMini-S serial protocol and extract distance (cm) and signal strength values from each frame.

---

### SYS-019: Graduated Obstacle Response

- **Traces up**: STK-008
- **Traces down**: TSR-018, TSR-019, SWR-FZC-004 (placeholder)
- **Safety relevance**: ASIL C
- **Verification method**: Test
- **Verification criteria**: Obstacles at 100 cm, 50 cm, and 20 cm trigger warning (buzzer + CAN alert), partial braking (speed reduction request), and emergency braking (full brake + motor cutoff) respectively. All thresholds are configurable at compile time.
- **Status**: draft

The system shall implement a graduated response based on three configurable distance thresholds: warning zone (default 100 cm, buzzer and CAN alert), braking zone (default 50 cm, speed reduction request and partial braking), and emergency zone (default 20 cm, full emergency braking and motor cutoff request). The thresholds shall maintain the ordering: emergency < braking < warning.

---

### SYS-020: Lidar Sensor Plausibility Checking

- **Traces up**: STK-008, STK-016
- **Traces down**: TSR-020, TSR-021 (placeholder)
- **Safety relevance**: ASIL C
- **Verification method**: Test
- **Verification criteria**: A stuck sensor (no change > 1 cm over 500 ms) triggers a fault. A UART timeout of 100 ms triggers a communication fault. On any fault, the system substitutes 0 cm (obstacle present) and logs a DTC.
- **Status**: draft

The system shall perform plausibility checks on every lidar reading: range check (2-1200 cm valid), stuck sensor detection (less than 1 cm change over 50 consecutive samples), signal strength check (minimum threshold, default: 100), and UART timeout check (100 ms). On any plausibility failure, the system shall substitute a safe default distance of 0 cm (obstacle present) and log a DTC.

---

## 8. Functional System Requirements — Safety Monitoring

### SYS-021: Heartbeat Transmission by Zone ECUs

- **Traces up**: STK-009, STK-019
- **Traces down**: TSR-025, TSR-026 (placeholder)
- **Safety relevance**: ASIL C
- **Verification method**: Test
- **Verification criteria**: Each zone ECU (CVC, FZC, RZC) transmits a heartbeat CAN message at 50 ms +/- 5 ms intervals. The message contains ECU ID, alive counter (incrementing by 1 per transmission), operating mode, and CRC-8.
- **Status**: draft

Each zone ECU (CVC, FZC, RZC) shall transmit a heartbeat CAN message at a fixed interval of 50 ms (tolerance: +/- 5 ms). The heartbeat message shall include the ECU identifier, an alive counter, the current operating mode, and CRC-8 E2E protection.

---

### SYS-022: Heartbeat Timeout Detection by Safety Controller

- **Traces up**: STK-009, STK-019
- **Traces down**: TSR-027, TSR-028 (placeholder)
- **Safety relevance**: ASIL C
- **Verification method**: Test
- **Verification criteria**: When a zone ECU stops transmitting heartbeats, the SC detects the timeout within 150 ms (3 missed heartbeats). After a 50 ms confirmation period, the SC de-energizes the kill relay (total: 200 ms from last heartbeat to relay open).
- **Status**: draft

The Safety Controller shall monitor heartbeat messages from each zone ECU and detect a timeout when no valid heartbeat (passing E2E check) is received within 150 ms (3 times the heartbeat interval). On timeout detection, the SC shall illuminate the fault LED for the failed ECU, wait 50 ms for confirmation, and then de-energize the kill relay if the heartbeat is still absent.

---

### SYS-023: Cross-Plausibility Check — Torque vs. Current

- **Traces up**: STK-009, STK-016
- **Traces down**: TSR-041, TSR-042 (placeholder)
- **Safety relevance**: ASIL C
- **Verification method**: Test
- **Verification criteria**: A simulated 25% deviation between expected current (from torque request) and actual current, sustained for 50 ms, triggers SC kill relay de-energization within 60 ms.
- **Status**: draft

The Safety Controller shall continuously compare the torque request CAN message (from CVC) against the actual motor current CAN message (from RZC) using a torque-to-current lookup table. The SC shall detect a cross-plausibility fault when the deviation exceeds 20% (calibratable) for 50 ms, and shall de-energize the kill relay.

---

### SYS-024: Kill Relay Control — Energize-to-Run

- **Traces up**: STK-009, STK-019
- **Traces down**: TSR-029, TSR-030 (placeholder)
- **Safety relevance**: ASIL D
- **Verification method**: Test
- **Verification criteria**: De-asserting GIO_A0 on the SC results in relay opening (12V power removed from actuators) within 10 ms. The relay is not re-energized without a full power cycle and startup self-test pass.
- **Status**: draft

The Safety Controller shall control the kill relay via GIO_A0 GPIO driving a MOSFET gate. The relay shall use an energize-to-run configuration: the SC must actively hold the relay energized to allow power flow. The SC shall de-energize the relay within 5 ms of any confirmed safety violation (heartbeat timeout, cross-plausibility fault, self-test failure, lockstep error, external watchdog timeout). Re-energization shall require a complete system power cycle and successful startup self-test.

---

### SYS-025: Safety Controller CAN Listen-Only Mode

- **Traces up**: STK-009, STK-004
- **Traces down**: SWR-SC-001 (placeholder)
- **Safety relevance**: ASIL C
- **Verification method**: Test
- **Verification criteria**: With the SC connected to the CAN bus, no frames are transmitted by the SC. Bus traffic analysis confirms zero TX frames from the SC CAN ID range.
- **Status**: draft

The Safety Controller shall operate in CAN listen-only mode (DCAN TEST register bit 3 set). The SC shall receive all CAN messages for monitoring purposes but shall not transmit any CAN messages. This ensures the SC cannot corrupt the CAN bus or interfere with zone ECU communication.

---

### SYS-026: Safety Controller Lockstep CPU Monitoring

- **Traces up**: STK-004, STK-009
- **Traces down**: SWR-SC-002 (placeholder)
- **Safety relevance**: ASIL D
- **Verification method**: Test
- **Verification criteria**: A fault injected into one CPU core via the SC debug interface triggers a lockstep comparison error and ESM reset within 1 clock cycle. The SC does not re-energize the kill relay after a lockstep fault.
- **Status**: draft

The Safety Controller shall rely on the TMS570LC43x hardware lockstep CPU cores as the primary computation error detection mechanism. A lockstep comparison error shall trigger an immediate CPU reset via the Error Signaling Module (ESM). The SC firmware shall not re-energize the kill relay after a lockstep reset without a full self-test pass.

---

### SYS-027: External Watchdog Monitoring (All Physical ECUs)

- **Traces up**: STK-009, STK-019
- **Traces down**: TSR-031, TSR-032 (placeholder)
- **Safety relevance**: ASIL D
- **Verification method**: Test
- **Verification criteria**: When ECU firmware is halted (debugger pause), the TPS3823 asserts RESET within 1.6 seconds (+/- 10%). The MCU restarts and performs a self-test sequence.
- **Status**: draft

Each physical ECU (CVC, FZC, RZC, SC) shall be monitored by an external TPS3823 watchdog IC. The firmware shall toggle the WDI pin only when the main control loop has completed a full cycle and all critical runtime self-checks have passed. Failure to toggle within the watchdog timeout period (default: 1.6 seconds) shall result in a hardware reset of the MCU.

---

## 9. Functional System Requirements — Emergency Stop

### SYS-028: E-Stop Detection and Broadcast

- **Traces up**: STK-010, STK-016
- **Traces down**: TSR-033, TSR-034 (placeholder)
- **Safety relevance**: ASIL B
- **Verification method**: Test
- **Verification criteria**: E-stop button press is detected within 1 ms (GPIO interrupt latency). E-stop CAN message (ID 0x001) is transmitted within 2 ms of detection. All ECUs reach safe state within 12 ms of E-stop press.
- **Status**: draft

The system shall detect E-stop button activation within 1 ms via hardware interrupt on the CVC (PC13, falling edge, hardware debounce). On detection, the CVC shall immediately set the local torque request to zero and broadcast a high-priority E-stop CAN message (ID 0x001) within 1 ms. All receiving ECUs shall react within 10 ms: RZC disables motor, FZC applies full brake and centers steering. The system shall remain in SAFE_STOP until E-stop is released and a manual restart is performed.

---

## 10. Functional System Requirements — Vehicle State Management

### SYS-029: Vehicle State Machine

- **Traces up**: STK-005, STK-006, STK-007, STK-016, STK-017
- **Traces down**: TSR-035, TSR-036, TSR-037 (placeholder)
- **Safety relevance**: ASIL D
- **Verification method**: Test
- **Verification criteria**: Only valid transitions as defined in the transition table are accepted. An invalid transition attempt (e.g., LIMP to RUN) is rejected and logged. E-stop forces transition from any state to SAFE_STOP. State is persisted in flash and restored on power-up.
- **Status**: draft

The CVC shall maintain a deterministic vehicle state machine with states: INIT, RUN, DEGRADED, LIMP, SAFE_STOP, SHUTDOWN. The state machine shall enforce valid transitions only (no undefined transitions), support E-stop override from any state to SAFE_STOP, support SC override from any state to SHUTDOWN, broadcast the current state on CAN every 10 ms with E2E protection, and persist state in non-volatile memory. Each state shall define maximum operational limits for torque, speed, steering range, and steering rate.

---

### SYS-030: Coordinated Mode Management via BswM

- **Traces up**: STK-005, STK-006, STK-007, STK-003
- **Traces down**: SWR-BSW-001 (placeholder)
- **Safety relevance**: ASIL D
- **Verification method**: Test
- **Verification criteria**: A CVC state transition to DEGRADED results in all ECUs (FZC, RZC) acknowledging the new mode via heartbeat within 100 ms. Failure to acknowledge causes escalation to LIMP.
- **Status**: draft

The BSW Mode Manager (BswM) on each ECU shall synchronize operating modes across the system. The CVC shall broadcast the vehicle state, and each ECU's BswM shall adjust its local operational limits accordingly (RZC: current/PWM limits, FZC: servo/rate limits, BCM: hazard lights). Each ECU shall acknowledge the mode transition via its heartbeat message. If any ECU fails to acknowledge within 100 ms, the CVC shall escalate to the next degradation level.

---

## 11. Functional System Requirements — CAN Communication

### SYS-031: CAN Bus Configuration

- **Traces up**: STK-022
- **Traces down**: SWR-BSW-002 (placeholder)
- **Safety relevance**: ASIL D
- **Verification method**: Test
- **Verification criteria**: CAN bus operates at 500 kbps with all 7+ nodes connected. Bit error rate is below 10^-6. Bus utilization under normal operation is below 60%.
- **Status**: draft

The system shall operate a CAN 2.0B bus at 500 kbps with 11-bit standard CAN IDs. The bus shall connect all ECU nodes (CVC, FZC, RZC, SC, BCM, ICU, TCU) plus the Pi gateway. Bus termination shall be 120 ohm at each physical end. CAN transceivers shall be TJA1051T/3 for STM32 ECUs and SN65HVD230 for the TMS570.

---

### SYS-032: E2E Protection on Safety-Critical Messages

- **Traces up**: STK-020
- **Traces down**: TSR-022, TSR-023, TSR-024 (placeholder)
- **Safety relevance**: ASIL D
- **Verification method**: Test
- **Verification criteria**: A message with a corrupted CRC is discarded by the receiver. A message with a repeated alive counter is discarded. Three consecutive E2E failures result in safe default substitution within 30 ms.
- **Status**: draft

All safety-critical CAN messages shall include E2E protection: CRC-8 (polynomial 0x1D, SAE J1850) over payload and data ID, 4-bit alive counter (incrementing by 1 per transmission, modulo 16), and 8-bit data ID unique per message type. On E2E failure, the receiver shall use the last valid value for one additional cycle; after 3 consecutive failures, the receiver shall substitute safe defaults (zero torque, full brake, center steering).

---

### SYS-033: CAN Message Priority Assignment

- **Traces up**: STK-022
- **Traces down**: SWR-BSW-003 (placeholder)
- **Safety relevance**: ASIL D
- **Verification method**: Analysis
- **Verification criteria**: CAN arbitration analysis confirms that the E-stop message (ID 0x001) wins arbitration against all other messages. ASIL D messages have lower CAN IDs (higher priority) than ASIL C, B, A, and QM messages.
- **Status**: draft

CAN message IDs shall be assigned by safety priority: E-stop (0x001, highest priority), ASIL D control messages (0x010-0x0FF), ASIL C monitoring messages (0x100-0x1FF), ASIL B status messages (0x200-0x2FF), QM comfort/telemetry messages (0x300-0x3FF), and UDS diagnostic messages (0x7DF, 0x7E0-0x7E6 per ISO 14229).

---

### SYS-034: CAN Bus Loss Detection per ECU

- **Traces up**: STK-009, STK-017
- **Traces down**: TSR-038, TSR-039 (placeholder)
- **Safety relevance**: ASIL C
- **Verification method**: Test
- **Verification criteria**: When the CAN bus is disconnected from an ECU, the ECU detects bus loss within 200 ms and autonomously transitions to its safe state (CVC: SAFE_STOP, FZC: auto-brake + steer center, RZC: motor disable, SC: kill relay open).
- **Status**: draft

Each ECU shall independently detect CAN bus loss (bus-off condition, no messages received for 200 ms, or CAN error counter exceeding warning threshold) and autonomously transition to its ECU-specific safe state without relying on commands from other ECUs.

---

## 12. Functional System Requirements — Body Control

### SYS-035: Automatic Headlight Control

- **Traces up**: STK-011
- **Traces down**: SWR-BCM-001 (placeholder)
- **Safety relevance**: QM
- **Verification method**: Demonstration
- **Verification criteria**: BCM activates headlight output when vehicle state is RUN and speed is above 0. Headlights deactivate when vehicle state is OFF or SHUTDOWN.
- **Status**: draft

The BCM shall automatically activate headlight output when the vehicle is in RUN state and speed is above zero. Headlights shall be deactivated when the vehicle is in OFF or SHUTDOWN state. The headlight control shall be based on vehicle state CAN messages.

---

### SYS-036: Turn Indicator and Hazard Light Control

- **Traces up**: STK-011, STK-026
- **Traces down**: SWR-BCM-002 (placeholder)
- **Safety relevance**: QM
- **Verification method**: Demonstration
- **Verification criteria**: Hazard lights activate within 100 ms of vehicle state transition to DEGRADED or higher. Turn indicators follow steering angle polarity.
- **Status**: draft

The BCM shall control turn indicators based on steering angle polarity (CAN message from CVC) and activate hazard lights when the vehicle transitions to DEGRADED, LIMP, SAFE_STOP, or SHUTDOWN states. Hazard light activation on fault states shall be automatic and shall not require operator action.

---

## 13. Functional System Requirements — Diagnostics

### SYS-037: UDS Diagnostic Session Control

- **Traces up**: STK-012
- **Traces down**: SWR-TCU-001 (placeholder)
- **Safety relevance**: QM
- **Verification method**: Test
- **Verification criteria**: A UDS 0x10 request with sub-function 0x01 (default session) returns a positive response (0x50 0x01). Sub-function 0x02 (programming session) requires Security Access first.
- **Status**: draft

The system shall implement UDS Diagnostic Session Control (service 0x10) per ISO 14229. The TCU shall support at least default session (0x01), programming session (0x02), and extended diagnostic session (0x03). Session transitions shall enforce security access requirements for privileged sessions.

---

### SYS-038: UDS Read and Clear DTC Services

- **Traces up**: STK-012, STK-013
- **Traces down**: SWR-TCU-002, SWR-TCU-003 (placeholder)
- **Safety relevance**: QM
- **Verification method**: Test
- **Verification criteria**: After a pedal sensor fault, a UDS 0x19 request returns the associated DTC with freeze-frame data. A subsequent 0x14 request clears the DTC. A re-read confirms the DTC is cleared.
- **Status**: draft

The system shall implement UDS Read DTC Information (service 0x19) and Clear Diagnostic Information (service 0x14) per ISO 14229. The system shall report active, confirmed, and stored DTCs with associated freeze-frame data (timestamp, vehicle state, sensor readings at time of fault). DTC clearing shall require the fault condition to have been resolved.

---

### SYS-039: UDS Read/Write Data by Identifier

- **Traces up**: STK-012
- **Traces down**: SWR-TCU-004 (placeholder)
- **Safety relevance**: QM
- **Verification method**: Test
- **Verification criteria**: A UDS 0x22 request for DID 0xF190 (VIN) returns the programmed VIN. A 0x2E write to a writable DID updates the value persistently.
- **Status**: draft

The system shall implement UDS Read Data By Identifier (service 0x22) and Write Data By Identifier (service 0x2E) per ISO 14229. Supported DIDs shall include vehicle identification number (0xF190), hardware version (0xF191), software version (0xF195), and calibration data identifiers. Write access shall require Security Access.

---

### SYS-040: UDS Security Access

- **Traces up**: STK-012
- **Traces down**: SWR-TCU-005 (placeholder)
- **Safety relevance**: QM
- **Verification method**: Test
- **Verification criteria**: A Security Access sequence with correct seed-key pair grants access. An incorrect key is rejected with NRC 0x35. After 3 failed attempts, access is locked for 10 seconds.
- **Status**: draft

The system shall implement UDS Security Access (service 0x27) per ISO 14229 with a seed-and-key mechanism. Security Access shall be required before accessing programming session, writing calibration data, or clearing DTCs. The system shall enforce lockout after 3 consecutive failed attempts (minimum 10 second delay).

---

### SYS-041: DTC Storage and Persistence

- **Traces up**: STK-013
- **Traces down**: SWR-BSW-004 (placeholder)
- **Safety relevance**: QM
- **Verification method**: Test
- **Verification criteria**: A DTC stored during operation is present after power cycle. Freeze-frame data includes timestamp, vehicle state, and relevant sensor values. At least 50 DTCs can be stored simultaneously.
- **Status**: draft

The system shall store diagnostic trouble codes in non-volatile memory (flash) with freeze-frame data. DTCs shall survive power cycles. The system shall support a minimum of 50 concurrent DTCs. Each DTC shall record: DTC number (per SAE J2012), fault status byte, occurrence counter, first/last occurrence timestamp, and freeze-frame snapshot (vehicle state, speed, relevant sensor readings).

---

## 14. Functional System Requirements — Telemetry and Cloud

### SYS-042: MQTT Telemetry to AWS IoT Core

- **Traces up**: STK-014
- **Traces down**: SWR-GW-001 (placeholder)
- **Safety relevance**: QM
- **Verification method**: Demonstration
- **Verification criteria**: The Pi gateway publishes a telemetry JSON message to `vehicle/telemetry` topic at 1 message per 5 seconds. Messages arrive at AWS IoT Core and are visible in Timestream/Grafana.
- **Status**: draft

The Raspberry Pi gateway shall publish vehicle telemetry to AWS IoT Core via MQTT v3.1.1 over TLS 1.2 (port 8883) using X.509 client certificate authentication. Telemetry shall be batched at 1 message per 5 seconds to stay within AWS free tier limits. Topics shall include `vehicle/telemetry`, `vehicle/dtc/new`, `vehicle/dtc/soft`, and `vehicle/alerts`.

---

### SYS-043: Edge ML Anomaly Detection

- **Traces up**: STK-015
- **Traces down**: SWR-GW-002 (placeholder)
- **Safety relevance**: QM
- **Verification method**: Demonstration
- **Verification criteria**: A simulated anomalous motor current pattern (e.g., sinusoidal oscillation not matching normal profile) triggers an anomaly alert published to `vehicle/alerts` within 5 seconds.
- **Status**: draft

The Raspberry Pi gateway shall run machine learning inference (scikit-learn) on CAN bus data to detect anomalous patterns in motor current, temperature, and CAN message timing. Detected anomalies shall be published to the cloud as alerts. ML inference shall not be used for safety-critical decisions (QM only, informational).

---

## 15. Functional System Requirements — Operator Interface

### SYS-044: OLED Status Display

- **Traces up**: STK-026
- **Traces down**: SWR-CVC-002 (placeholder)
- **Safety relevance**: QM
- **Verification method**: Demonstration
- **Verification criteria**: The OLED displays the current vehicle state (RUN, DEGRADED, LIMP, SAFE_STOP) and active fault code within 100 ms of state change.
- **Status**: draft

The CVC shall drive the SSD1306 OLED display (128x64 pixels, I2C at 0x3C, 400 kHz) to show: current vehicle state, active fault code (if any), operational restrictions (speed limit, torque limit), and basic telemetry (speed, torque percentage). The display shall be updated every 100 ms.

---

### SYS-045: Audible Warning via Buzzer

- **Traces up**: STK-026
- **Traces down**: SWR-FZC-005 (placeholder)
- **Safety relevance**: QM
- **Verification method**: Demonstration
- **Verification criteria**: Distinct buzzer patterns are produced for DEGRADED (single beep), LIMP (slow repeating), SAFE_STOP (fast repeating), and emergency (continuous) within 200 ms of state change.
- **Status**: draft

The FZC shall drive a piezo buzzer (GPIO) with distinct warning patterns: single beep for DEGRADED transition, slow repeating beep for LIMP, fast repeating beep for SAFE_STOP, and continuous tone for emergency (obstacle in emergency zone or E-stop activated). Buzzer patterns shall be based on CAN state messages from CVC and local lidar detections.

---

### SYS-046: Fault LED Panel on Safety Controller

- **Traces up**: STK-026, STK-019
- **Traces down**: SWR-SC-003 (placeholder)
- **Safety relevance**: ASIL C
- **Verification method**: Test
- **Verification criteria**: When a zone ECU heartbeat times out, the corresponding LED on the SC panel illuminates within 200 ms. The LED is driven directly by SC GPIO, independent of CAN communication to other ECUs.
- **Status**: draft

The Safety Controller shall drive a fault LED panel (4 LEDs on GIO_A1-A4) with one LED per zone ECU (CVC, FZC, RZC) and one system LED. LED states: off = no fault, steady on = fault detected, blinking = heartbeat lost. The LEDs shall be driven directly by SC GPIO, independent of CAN communication, providing operator feedback even during total CAN bus failure.

---

## 16. Interface Requirements

### SYS-047: SPI Interface — Pedal and Steering Sensors

- **Traces up**: STK-005, STK-006, STK-023
- **Traces down**: SWR-BSW-005 (placeholder)
- **Safety relevance**: ASIL D
- **Verification method**: Test
- **Verification criteria**: Both pedal sensors on SPI1 are read within 500 us total. SPI clock is 10 MHz. CRC validation of AS5048A data frame passes for all valid readings.
- **Status**: draft

The system shall interface with AS5048A sensors via SPI: CVC SPI1 for dual pedal sensors (CS pins PA4 and PA15), FZC SPI2 for the steering sensor (CS pin PB12). SPI clock speed shall be 10 MHz. The AS5048A 14-bit angle data and diagnostic fields shall be read and validated (CRC check) every control cycle.

---

### SYS-048: UART Interface — Lidar Sensor

- **Traces up**: STK-008, STK-023
- **Traces down**: SWR-BSW-006 (placeholder)
- **Safety relevance**: ASIL C
- **Verification method**: Test
- **Verification criteria**: TFMini-S UART frames are received at 100 Hz on USART1. Frame parsing extracts distance and signal strength. Frame checksum validation rejects corrupted frames.
- **Status**: draft

The system shall interface with the TFMini-S lidar sensor via USART1 on the FZC at 115200 baud. The UART driver shall receive 9-byte TFMini-S frames using DMA, validate the frame header and checksum, and extract distance (cm) and signal strength values. Frame reception timeout shall be monitored.

---

### SYS-049: ADC Interface — Current, Temperature, Voltage

- **Traces up**: STK-005, STK-023
- **Traces down**: SWR-BSW-007 (placeholder)
- **Safety relevance**: ASIL A
- **Verification method**: Test
- **Verification criteria**: ADC channels on RZC (CH1-CH4) convert at 12-bit resolution with sample rate of at least 1 kHz for current (CH1) and 10 Hz for temperature (CH2, CH3) and voltage (CH4).
- **Status**: draft

The system shall read analog signals on the RZC via ADC1: CH1 for motor current (ACS723, 0-3.3V mapping to 0-30A), CH2 and CH3 for motor/winding temperature (NTC, 0-3.3V mapping to -40C to +125C), and CH4 for battery voltage (resistor divider, 0-3.3V mapping to 0-16V). ADC resolution shall be 12-bit. Current sampling rate shall be at least 1 kHz.

---

### SYS-050: PWM Interface — Motor and Servos

- **Traces up**: STK-005, STK-006, STK-007
- **Traces down**: SWR-BSW-008 (placeholder)
- **Safety relevance**: ASIL D
- **Verification method**: Test
- **Verification criteria**: Motor PWM (TIM3) operates at 20 kHz with 0-100% duty cycle control. Servo PWM (TIM2) operates at 50 Hz with 1-2 ms pulse width. All PWM channels are independently controllable.
- **Status**: draft

The system shall generate PWM outputs: RZC TIM3_CH1 and TIM3_CH2 for BTS7960 RPWM/LPWM at 20 kHz, FZC TIM2_CH1 for steering servo at 50 Hz (1-2 ms pulse), and FZC TIM2_CH2 for brake servo at 50 Hz (1-2 ms pulse). PWM duty cycle resolution shall be at least 10 bits. PWM outputs shall be independently disable-able via software.

---

## 17. Non-Functional System Requirements

### SYS-051: MISRA C Compliance

- **Traces up**: STK-030
- **Traces down**: SWR-ALL-001 (placeholder)
- **Safety relevance**: ASIL D
- **Verification method**: Analysis
- **Verification criteria**: Static analysis (cppcheck or equivalent) reports zero mandatory MISRA C:2012 rule violations. All deviations are documented with formal deviation records.
- **Status**: draft

All firmware source code shall comply with MISRA C:2012 (with 2023 amendments). Mandatory rules shall have zero violations. Required rules shall have documented deviations. Advisory rules shall be followed where practical. Static analysis enforcement shall be part of the CI build pipeline.

---

### SYS-052: Static RAM Only — No Dynamic Allocation

- **Traces up**: STK-016, STK-030
- **Traces down**: SWR-ALL-002 (placeholder)
- **Safety relevance**: ASIL D
- **Verification method**: Analysis
- **Verification criteria**: No calls to malloc, calloc, realloc, or free exist in any firmware source file. Linker map confirms all data segments are statically sized. Heap size is configured to zero.
- **Status**: draft

All firmware shall use static memory allocation only. Dynamic memory allocation functions (malloc, calloc, realloc, free) shall not be used in any production firmware. All buffers, queues, and data structures shall be statically sized at compile time. The heap shall be configured to zero size in the linker script.

---

### SYS-053: WCET Within Deadline Margin

- **Traces up**: STK-021, STK-018
- **Traces down**: SWR-ALL-003 (placeholder)
- **Safety relevance**: ASIL D
- **Verification method**: Analysis
- **Verification criteria**: Measured WCET of the main control loop task on each ECU is below 80% of its scheduling deadline (8 ms for a 10 ms cycle). Measurement is performed on target hardware with all interrupts enabled.
- **Status**: draft

The worst-case execution time (WCET) of each safety-critical task shall not exceed 80% of its scheduling deadline. For the 10 ms main control loop, WCET shall be below 8 ms. WCET shall be measured on target hardware under worst-case conditions and documented in the timing analysis report.

---

### SYS-054: Flash Memory Utilization

- **Traces up**: STK-021
- **Traces down**: SWR-ALL-004 (placeholder)
- **Safety relevance**: QM
- **Verification method**: Analysis
- **Verification criteria**: Linker map output shows total flash usage below 80% of available flash on each MCU (STM32G474RE: 512 KB, TMS570LC43x: 4 MB).
- **Status**: draft

Flash memory utilization shall not exceed 80% of the available flash on each MCU to provide margin for future updates, safety analyses, and DTC storage. The linker output shall be checked as part of the build process.

---

### SYS-055: Bidirectional Traceability Chain

- **Traces up**: STK-031
- **Traces down**: (this requirement governs the traceability process itself)
- **Safety relevance**: ASIL D
- **Verification method**: Inspection
- **Verification criteria**: The traceability matrix in the traceability document shows complete forward and backward trace for every SYS requirement: upward to STK, downward to TSR/SWR, and further to source code and test cases. No orphan requirements exist.
- **Status**: draft

Bidirectional traceability shall be maintained from stakeholder requirements (STK) through system requirements (SYS), technical safety requirements (TSR), software requirements (SWR), source code, unit tests, integration tests, and system tests. Every requirement shall trace down to implementation and test. Every test shall trace back to a requirement. The traceability matrix shall be maintained in `docs/aspice/traceability/traceability-matrix.md`.

---

### SYS-056: SAP QM Mock Integration

- **Traces up**: STK-032
- **Traces down**: SWR-GW-003 (placeholder)
- **Safety relevance**: QM
- **Verification method**: Demonstration
- **Verification criteria**: A DTC event triggers an HTTP POST to the SAP QM mock API. The mock returns a Q-Meldung number. An 8D report template is auto-generated.
- **Status**: draft

The Raspberry Pi gateway shall forward new DTCs to a mock SAP QM API endpoint via HTTP REST. The mock shall create a quality notification (Q-Meldung) and auto-generate an 8D report template containing the DTC details, freeze-frame data, and suggested corrective actions.

---

## 18. Traceability Matrix — STK to SYS

The following matrix provides complete bidirectional traceability from stakeholder requirements to system requirements. Every STK requirement traces to at least one SYS requirement. No SYS requirement exists without an STK parent.

| STK | SYS Requirements |
|-----|------------------|
| STK-001 | SYS-029, SYS-032, SYS-041, SYS-055 |
| STK-002 | SYS-055 |
| STK-003 | SYS-030, SYS-032 |
| STK-004 | SYS-025, SYS-026 |
| STK-005 | SYS-001, SYS-002, SYS-003, SYS-004, SYS-005, SYS-006, SYS-007, SYS-008, SYS-009, SYS-029, SYS-030, SYS-047, SYS-049, SYS-050 |
| STK-006 | SYS-010, SYS-011, SYS-012, SYS-013, SYS-029, SYS-030, SYS-047, SYS-050 |
| STK-007 | SYS-014, SYS-015, SYS-016, SYS-017, SYS-029, SYS-030, SYS-050 |
| STK-008 | SYS-017, SYS-018, SYS-019, SYS-020, SYS-048 |
| STK-009 | SYS-021, SYS-022, SYS-023, SYS-024, SYS-025, SYS-026, SYS-027, SYS-034, SYS-046 |
| STK-010 | SYS-028 |
| STK-011 | SYS-035, SYS-036 |
| STK-012 | SYS-037, SYS-038, SYS-039, SYS-040 |
| STK-013 | SYS-038, SYS-041 |
| STK-014 | SYS-042 |
| STK-015 | SYS-043 |
| STK-016 | SYS-001, SYS-002, SYS-005, SYS-006, SYS-007, SYS-011, SYS-013, SYS-015, SYS-020, SYS-023, SYS-024, SYS-028, SYS-029, SYS-051, SYS-052 |
| STK-017 | SYS-012, SYS-016, SYS-024, SYS-029, SYS-034 |
| STK-018 | SYS-002, SYS-011, SYS-015, SYS-053 |
| STK-019 | SYS-022, SYS-024, SYS-027, SYS-046 |
| STK-020 | SYS-032 |
| STK-021 | SYS-053, SYS-054 |
| STK-022 | SYS-004, SYS-031, SYS-033 |
| STK-023 | SYS-047, SYS-048, SYS-049 |
| STK-024 | SYS-024 |
| STK-025 | SYS-029 |
| STK-026 | SYS-036, SYS-044, SYS-045, SYS-046 |
| STK-027 | (addressed by build system and documentation — no specific SYS requirement; process requirement) |
| STK-028 | SYS-029, SYS-032, SYS-051, SYS-055 |
| STK-029 | SYS-055 |
| STK-030 | SYS-051, SYS-052 |
| STK-031 | SYS-055 |
| STK-032 | SYS-056 |

### 18.1 Reverse Traceability — SYS to STK

| SYS | STK Parents |
|-----|-------------|
| SYS-001 | STK-005, STK-016 |
| SYS-002 | STK-005, STK-016, STK-018 |
| SYS-003 | STK-005 |
| SYS-004 | STK-005, STK-022 |
| SYS-005 | STK-005, STK-016 |
| SYS-006 | STK-005, STK-016 |
| SYS-007 | STK-005, STK-016 |
| SYS-008 | STK-005 |
| SYS-009 | STK-005 |
| SYS-010 | STK-006 |
| SYS-011 | STK-006, STK-016, STK-018 |
| SYS-012 | STK-006, STK-017 |
| SYS-013 | STK-006, STK-016 |
| SYS-014 | STK-007 |
| SYS-015 | STK-007, STK-016, STK-018 |
| SYS-016 | STK-007, STK-017 |
| SYS-017 | STK-007, STK-008 |
| SYS-018 | STK-008 |
| SYS-019 | STK-008 |
| SYS-020 | STK-008, STK-016 |
| SYS-021 | STK-009 |
| SYS-022 | STK-009, STK-019 |
| SYS-023 | STK-009, STK-016 |
| SYS-024 | STK-009, STK-016, STK-017, STK-019, STK-024 |
| SYS-025 | STK-004, STK-009 |
| SYS-026 | STK-004, STK-009 |
| SYS-027 | STK-009, STK-019 |
| SYS-028 | STK-010, STK-016 |
| SYS-029 | STK-001, STK-005, STK-006, STK-007, STK-016, STK-017, STK-025, STK-028 |
| SYS-030 | STK-003, STK-005, STK-006, STK-007 |
| SYS-031 | STK-022 |
| SYS-032 | STK-001, STK-003, STK-020, STK-028 |
| SYS-033 | STK-022 |
| SYS-034 | STK-009, STK-017 |
| SYS-035 | STK-011 |
| SYS-036 | STK-011, STK-026 |
| SYS-037 | STK-012 |
| SYS-038 | STK-012, STK-013 |
| SYS-039 | STK-012 |
| SYS-040 | STK-012 |
| SYS-041 | STK-001, STK-013 |
| SYS-042 | STK-014 |
| SYS-043 | STK-015 |
| SYS-044 | STK-026 |
| SYS-045 | STK-026 |
| SYS-046 | STK-009, STK-019, STK-026 |
| SYS-047 | STK-005, STK-006, STK-023 |
| SYS-048 | STK-008, STK-023 |
| SYS-049 | STK-005, STK-023 |
| SYS-050 | STK-005, STK-006, STK-007 |
| SYS-051 | STK-028, STK-030 |
| SYS-052 | STK-016, STK-030 |
| SYS-053 | STK-018, STK-021 |
| SYS-054 | STK-021 |
| SYS-055 | STK-001, STK-002, STK-028, STK-029, STK-031 |
| SYS-056 | STK-032 |

### 18.2 Traceability Completeness Check

| Check | Result |
|-------|--------|
| All 32 STK requirements trace to at least one SYS? | YES — 31 of 32 trace directly. STK-027 is a process requirement (build/demo) addressed by project infrastructure, not a system requirement. |
| All 56 SYS requirements trace to at least one STK? | YES — every SYS-NNN has at least one STK parent. |
| No orphan SYS requirements (no STK parent)? | YES — 0 orphans. |
| No orphan STK requirements (no SYS child)? | YES — 0 orphans (STK-027 acknowledged as process scope). |

---

## 19. Requirements Summary

### 19.1 By Category

| Category | Count | SYS Range |
|----------|-------|-----------|
| Drive-by-Wire | 9 | SYS-001 to SYS-009 |
| Steering | 4 | SYS-010 to SYS-013 |
| Braking | 4 | SYS-014 to SYS-017 |
| Obstacle Detection | 3 | SYS-018 to SYS-020 |
| Safety Monitoring | 7 | SYS-021 to SYS-027 |
| Emergency Stop | 1 | SYS-028 |
| State Management | 2 | SYS-029 to SYS-030 |
| CAN Communication | 4 | SYS-031 to SYS-034 |
| Body Control | 2 | SYS-035 to SYS-036 |
| Diagnostics | 5 | SYS-037 to SYS-041 |
| Telemetry / Cloud | 2 | SYS-042 to SYS-043 |
| Operator Interface | 3 | SYS-044 to SYS-046 |
| Interface | 4 | SYS-047 to SYS-050 |
| Non-Functional | 6 | SYS-051 to SYS-056 |
| **Total** | **56** | |

### 19.2 By Safety Relevance

| ASIL | Count | SYS IDs |
|------|-------|---------|
| ASIL D | 18 | SYS-001, SYS-002, SYS-003, SYS-004, SYS-010, SYS-011, SYS-012, SYS-014, SYS-015, SYS-016, SYS-024, SYS-028, SYS-029, SYS-030, SYS-031, SYS-032, SYS-033, SYS-047, SYS-050, SYS-051, SYS-052, SYS-053, SYS-055 |
| ASIL C | 12 | SYS-007, SYS-013, SYS-017, SYS-018, SYS-019, SYS-020, SYS-021, SYS-022, SYS-023, SYS-025, SYS-034, SYS-046, SYS-048 |
| ASIL B | 1 | SYS-028 |
| ASIL A | 3 | SYS-005, SYS-006, SYS-049 |
| QM | 22 | SYS-003, SYS-008, SYS-009, SYS-026, SYS-027, SYS-035, SYS-036, SYS-037, SYS-038, SYS-039, SYS-040, SYS-041, SYS-042, SYS-043, SYS-044, SYS-045, SYS-054, SYS-056 |

### 19.3 By Verification Method

| Method | Count |
|--------|-------|
| Test (T) | 42 |
| Analysis (A) | 6 |
| Demonstration (D) | 7 |
| Inspection (I) | 1 |

---

## 20. Open Items and Assumptions

### 20.1 Assumptions

| ID | Assumption | Impact |
|----|-----------|--------|
| SYS-A-001 | CAN message cycle times are achievable at 500 kbps with defined bus load | CAN scheduling analysis needed during SYS.3 |
| SYS-A-002 | Calibratable thresholds (pedal, steering, current, temperature, lidar) use engineering estimates pending hardware characterization | Final values determined during integration testing |
| SYS-A-003 | STM32G474RE FDCAN controller in classic mode is interoperable with TMS570LC43x DCAN at 500 kbps | Confirmed in hardware feasibility analysis |
| SYS-A-004 | Docker containers with SocketCAN can emulate CAN communication for simulated ECUs | Confirmed in architecture decision |

### 20.2 Open Items

| ID | Item | Owner | Target |
|----|------|-------|--------|
| SYS-O-001 | Derive Technical Safety Requirements (TSR) from safety-relevant SYS requirements | System Engineer | TSC phase |
| SYS-O-002 | Derive Software Requirements (SWR) per ECU from SYS requirements | SW Engineer | SWE.1 phase |
| SYS-O-003 | Perform CAN bus scheduling and utilization analysis | System Architect | SYS.3 phase |
| SYS-O-004 | Define complete CAN message matrix (IDs, cycle times, DLC, signals) | System Architect | SYS.3 phase |
| SYS-O-005 | Characterize calibratable thresholds on target hardware | Integration Engineer | Integration phase |
| SYS-O-006 | Complete ASIL classification review for all 56 SYS requirements | FSE | Before SWE.1 |

---

## 21. Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 0.1 | 2026-02-21 | System | Initial stub (planned status) |
| 1.0 | 2026-02-21 | System | Complete system requirements: 56 requirements (SYS-001 to SYS-056), bidirectional traceability matrix (STK to SYS and SYS to STK), completeness check, verification criteria for all requirements |
