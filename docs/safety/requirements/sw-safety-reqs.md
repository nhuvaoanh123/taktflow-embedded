---
document_id: SSR
title: "Software Safety Requirements"
version: "1.0"
status: draft
iso_26262_part: 6
aspice_process: SWE.1
date: 2026-02-21
---

# Software Safety Requirements

## 1. Purpose

This document specifies the software safety requirements (SSR) for the Taktflow Zonal Vehicle Platform, derived from the technical safety requirements (TSR) per ISO 26262-6 Clause 6. Software safety requirements define what the software on each ECU shall do to implement the technical safety requirements. Each SSR is allocated to a specific ECU and traces to one or more TSRs, enabling bidirectional traceability from safety goals through to source code and test cases.

## 2. Referenced Documents

| Document ID | Title | Version |
|-------------|-------|---------|
| TSR | Technical Safety Requirements | 1.0 |
| FSR | Functional Safety Requirements | 1.0 |
| SG | Safety Goals | 1.0 |
| FSC | Functional Safety Concept | 1.0 |
| HSI | Hardware-Software Interface Specification | 0.1 |

## 3. Requirement Conventions

### 3.1 Requirement Structure

Each requirement follows the format:

- **ID**: SSR-{ECU}-NNN (e.g., SSR-CVC-001)
- **Title**: Descriptive name
- **ASIL**: Inherited from parent TSR
- **Traces up**: TSR-xxx
- **Traces down**: firmware/{ecu}/src/{file}:{function} (placeholder until implementation)
- **Verified by**: TC-{ECU}-xxx (placeholder test case IDs)
- **Status**: draft | reviewed | approved | implemented | verified

### 3.2 Naming Convention

SSR IDs are prefixed by the allocated ECU:
- `SSR-CVC-NNN` — Central Vehicle Computer
- `SSR-FZC-NNN` — Front Zone Controller
- `SSR-RZC-NNN` — Rear Zone Controller
- `SSR-SC-NNN` — Safety Controller

### 3.3 ASIL Inheritance

Each SSR inherits the ASIL of its parent TSR. Where an SSR traces to multiple TSRs, it inherits the highest ASIL.

---

## 4. CVC — Central Vehicle Computer Software Safety Requirements

### SSR-CVC-001: SPI1 Initialization for Dual Pedal Sensors

- **ASIL**: D
- **Traces up**: TSR-001
- **Traces down**: firmware/cvc/src/mcal/spi_cfg.c:Spi_Init()
- **Verified by**: TC-CVC-001
- **Status**: draft

The CVC software shall initialize SPI1 in master mode at 1 MHz clock, CPOL=0, CPHA=1 (matching AS5048A requirements). The software shall configure PA4 as chip-select for pedal sensor 1 and PA15 as chip-select for pedal sensor 2, both as GPIO outputs initialized to HIGH (deselected). The initialization shall verify SPI peripheral clock enable and confirm the SPI status register indicates idle state before returning success.

---

### SSR-CVC-002: Pedal Sensor SPI Read Cycle

- **ASIL**: D
- **Traces up**: TSR-001
- **Traces down**: firmware/cvc/src/swc/swc_pedal.c:Swc_Pedal_ReadSensors()
- **Verified by**: TC-CVC-002, TC-CVC-003
- **Status**: draft

The CVC software shall read both AS5048A pedal sensors sequentially within a single 10 ms control cycle. For each sensor: (a) assert the chip-select LOW, (b) transmit the 16-bit read command for the angle register (0x3FFF), (c) receive the 16-bit response, (d) de-assert chip-select HIGH, (e) validate the response parity bit and extract the 14-bit angle value. If the SPI transaction does not complete within 500 us (measured by a hardware timer), the software shall abort the transaction, de-assert chip-select, and retry once. If the retry also fails, the software shall set the sensor status to COMM_FAULT and use the last valid reading.

---

### SSR-CVC-003: Dual Pedal Sensor Plausibility Comparison

- **ASIL**: D
- **Traces up**: TSR-002
- **Traces down**: firmware/cvc/src/swc/swc_pedal.c:Swc_Pedal_CheckPlausibility()
- **Verified by**: TC-CVC-004, TC-CVC-005, TC-CVC-006
- **Status**: draft

The CVC software shall compare the 14-bit angle values from both pedal sensors every 10 ms. The software shall compute the absolute difference and compare it against the plausibility threshold (819 counts, representing 5% of the 16383-count full-scale range). The software shall maintain a debounce counter that increments each cycle the difference exceeds the threshold and resets to zero when the difference is within threshold. When the debounce counter reaches 2 (20 ms), the software shall set the pedal plausibility fault flag to TRUE and invoke the zero-torque reaction (SSR-CVC-006).

---

### SSR-CVC-004: Pedal Sensor Stuck-at Detection

- **ASIL**: D
- **Traces up**: TSR-002
- **Traces down**: firmware/cvc/src/swc/swc_pedal.c:Swc_Pedal_CheckStuck()
- **Verified by**: TC-CVC-007, TC-CVC-008
- **Status**: draft

The CVC software shall detect a stuck pedal sensor by monitoring each sensor's value independently. If a sensor reading does not change by more than 10 counts (out of 16383) for 100 consecutive samples (1 second at 100 Hz), and the pedal is not physically at rest (other sensor shows movement), the software shall flag the stuck sensor as faulty and invoke the zero-torque reaction (SSR-CVC-006).

---

### SSR-CVC-005: Pedal Sensor Individual Diagnostics

- **ASIL**: D
- **Traces up**: TSR-003
- **Traces down**: firmware/cvc/src/swc/swc_pedal.c:Swc_Pedal_DiagCheck()
- **Verified by**: TC-CVC-009, TC-CVC-010
- **Status**: draft

The CVC software shall perform per-sensor diagnostic checks every 10 ms: (a) range check the 14-bit value against 0 to 16383, flagging exactly 0x0000 or 0x3FFF held for 3 consecutive cycles as stuck-at fault, (b) rate-of-change check against a maximum of 1000 counts per 10 ms cycle, flagging exceedance as a glitch fault, (c) read the AS5048A diagnostic register (0x3FFD) every 100 ms (10th cycle) and flag AGC out-of-range or CORDIC overflow as a sensor hardware fault. Each fault type shall be independently tracked and reported via the Dem module with a unique DTC.

---

### SSR-CVC-006: Zero-Torque Latching on Pedal Fault

- **ASIL**: D
- **Traces up**: TSR-004
- **Traces down**: firmware/cvc/src/swc/swc_pedal.c:Swc_Pedal_FaultReaction()
- **Verified by**: TC-CVC-011, TC-CVC-012, TC-CVC-013
- **Status**: draft

Upon any pedal fault detection (plausibility, stuck, range, diagnostic), the CVC software shall: (a) set the internal torque request variable to 0 within 1 ms (same cycle as fault detection), (b) set a latching fault flag that prevents any non-zero torque request, (c) log a DTC via the Dem module with freeze-frame data (both sensor values, fault type, vehicle state). The latching fault flag shall only be cleared when the software verifies 50 consecutive fault-free cycles (500 ms) from both sensors AND receives a manual reset confirmation (E-stop release and re-engage sequence detected).

---

### SSR-CVC-007: Zero-Torque CAN Transmission

- **ASIL**: D
- **Traces up**: TSR-004
- **Traces down**: firmware/cvc/src/swc/swc_torque.c:Swc_Torque_Transmit()
- **Verified by**: TC-CVC-014
- **Status**: draft

The CVC software shall transmit the torque request on CAN ID 0x100 every 10 ms with E2E protection. When the pedal fault flag is active (SSR-CVC-006), the torque request field in the CAN message shall be forced to 0% regardless of any other software state. The E2E alive counter shall continue to increment normally during fault conditions to enable the receiver (RZC) to distinguish between a valid zero-torque command and CAN communication loss.

---

### SSR-CVC-008: CAN E2E Protection — Transmit Side

- **ASIL**: D
- **Traces up**: TSR-022, TSR-023
- **Traces down**: firmware/cvc/src/bsw/e2e/e2e_protect.c:E2E_Protect()
- **Verified by**: TC-CVC-015, TC-CVC-016
- **Status**: draft

The CVC software shall implement the E2E protection transmit function for all safety-critical CAN messages. The function shall: (a) write the Data ID (4-bit, unique per message type) into byte 0 bits 3:0, (b) increment the alive counter (4-bit) by 1 (wrapping 0-15) and write it into byte 0 bits 7:4, (c) compute CRC-8 (polynomial 0x1D, initial value 0xFF) over bytes 2-7 concatenated with the Data ID byte, and (d) write the CRC result into byte 1. The function shall maintain a separate alive counter state per Data ID.

---

### SSR-CVC-009: CAN E2E Protection — Transmit Counter Management

- **ASIL**: D
- **Traces up**: TSR-023
- **Traces down**: firmware/cvc/src/bsw/e2e/e2e_protect.c:E2E_IncrementCounter()
- **Verified by**: TC-CVC-017
- **Status**: draft

The CVC software shall maintain an array of alive counters indexed by Data ID (16 entries for 4-bit Data ID). Each counter shall be initialized to 0 at startup. The counter shall be incremented exactly once per message transmission. The software shall verify that a counter is never incremented without a corresponding CAN transmission to prevent counter desynchronization.

---

### SSR-CVC-010: CAN E2E Protection — Receive Side

- **ASIL**: D
- **Traces up**: TSR-024
- **Traces down**: firmware/cvc/src/bsw/e2e/e2e_check.c:E2E_Check()
- **Verified by**: TC-CVC-018, TC-CVC-019, TC-CVC-020
- **Status**: draft

The CVC software shall implement the E2E protection receive function for all incoming safety-critical CAN messages. The function shall: (a) extract the alive counter and Data ID from byte 0, (b) recompute CRC-8 over bytes 2-7 plus Data ID and compare with byte 1, (c) verify the alive counter has incremented by exactly 1 from the previously received value for the same Data ID, (d) verify the Data ID matches the expected value for the CAN message ID. On any E2E check failure, the function shall return an error code. The caller shall use the last valid value for up to 1 cycle (10 ms); if 3 consecutive E2E failures occur, the caller shall substitute the safe default value (zero torque for torque messages, SAFE_STOP for state messages).

---

### SSR-CVC-011: CVC Heartbeat Transmission

- **ASIL**: C
- **Traces up**: TSR-025
- **Traces down**: firmware/cvc/src/swc/swc_heartbeat.c:Swc_Heartbeat_Transmit()
- **Verified by**: TC-CVC-021
- **Status**: draft

The CVC software shall transmit a heartbeat CAN message on CAN ID 0x010 every 50 ms. The message payload shall contain: ECU ID (0x01), operating mode (4-bit enum matching the vehicle state machine state), fault status bitmask (8-bit: bit 0 = pedal fault, bit 1 = CAN fault, bit 2 = NVM fault, bit 3 = WDT warning, bits 4-7 reserved), alive counter (4-bit), and CRC-8 (8-bit). Total: 4 bytes.

---

### SSR-CVC-012: CVC Heartbeat Conditioning on Self-Check

- **ASIL**: C
- **Traces up**: TSR-026
- **Traces down**: firmware/cvc/src/swc/swc_heartbeat.c:Swc_Heartbeat_CheckConditions()
- **Verified by**: TC-CVC-022, TC-CVC-023
- **Status**: draft

The CVC software shall suppress heartbeat transmission if any of the following conditions are detected: (a) the main loop iteration counter has not incremented since the last heartbeat (indicating a partial loop hang), (b) the stack canary value (a 32-bit magic number 0xDEADBEEF written at the bottom of the stack during initialization) does not match the expected value, (c) the CAN controller status register indicates bus-off state. When suppressed, the heartbeat is not transmitted, which will trigger SC heartbeat timeout detection.

---

### SSR-CVC-013: CVC External Watchdog Feed

- **ASIL**: D
- **Traces up**: TSR-031
- **Traces down**: firmware/cvc/src/swc/swc_watchdog.c:Swc_Watchdog_Feed()
- **Verified by**: TC-CVC-024, TC-CVC-025
- **Status**: draft

The CVC software shall toggle the TPS3823 WDI pin (configured GPIO) once per main loop iteration. The toggle shall only execute when all four conditions are satisfied: (a) main loop completed one full iteration (all runnables returned), (b) stack canary (0xDEADBEEF) intact at the bottom of the main task stack, (c) RAM test pattern (32-byte alternating 0xAA/0x55 pattern at reserved address) matches expected values after write-read-compare, (d) CAN controller not in bus-off state. The toggle shall alternate between HIGH and LOW on each invocation.

---

### SSR-CVC-014: CVC E-Stop GPIO Interrupt Handler

- **ASIL**: B
- **Traces up**: TSR-033
- **Traces down**: firmware/cvc/src/swc/swc_estop.c:Swc_EStop_IRQHandler()
- **Verified by**: TC-CVC-026, TC-CVC-027
- **Status**: draft

The CVC software shall configure GPIO PC13 as an external interrupt input (falling-edge triggered, internal pull-up enabled). The interrupt service routine shall: (a) read the GPIO pin state, (b) if LOW, start a 1 ms software timer for debounce confirmation, (c) after 1 ms, re-read the pin, (d) if still LOW, set the E-stop confirmed flag to TRUE, (e) set the torque request to 0 immediately within the ISR. The ISR shall be configured at the highest NVIC priority (priority 0) to ensure minimum latency.

---

### SSR-CVC-015: CVC E-Stop CAN Broadcast

- **ASIL**: B
- **Traces up**: TSR-034
- **Traces down**: firmware/cvc/src/swc/swc_estop.c:Swc_EStop_Broadcast()
- **Verified by**: TC-CVC-028, TC-CVC-029
- **Status**: draft

Upon confirmed E-stop detection (SSR-CVC-014), the CVC software shall: (a) transition the vehicle state machine to SAFE_STOP, (b) transmit an E-stop CAN message on CAN ID 0x001 with E2E protection, containing the E-stop status (1 = active), alive counter, and CRC-8, (c) continue transmitting the E-stop message every 10 ms until the E-stop button is released (GPIO PC13 returns HIGH). The state machine shall remain in SAFE_STOP until the E-stop is released AND the operator performs a full restart sequence.

---

### SSR-CVC-016: Vehicle State Machine Transition Logic

- **ASIL**: D
- **Traces up**: TSR-035
- **Traces down**: firmware/cvc/src/swc/swc_state.c:Swc_State_ProcessEvent()
- **Verified by**: TC-CVC-030, TC-CVC-031, TC-CVC-032, TC-CVC-033
- **Status**: draft

The CVC software shall implement the vehicle state machine as a lookup table indexed by (current_state, event) returning the next_state. The table shall be a const array in flash memory (not modifiable at runtime). The state machine function shall: (a) validate the current state is within the enum range (0-5), (b) validate the event is within the enum range, (c) look up the transition in the table, (d) if the entry is INVALID_TRANSITION, log the attempted transition as a DTC and return the current state unchanged, (e) if valid, execute the transition, update the current state, and invoke the state-entry actions for the new state. The function shall enforce E-stop override: if the event is E_STOP, transition to SAFE_STOP from any state. The function shall enforce SC override: if the event is SC_KILL, transition to SHUTDOWN from any state.

---

### SSR-CVC-017: Vehicle State Machine Entry Actions

- **ASIL**: D
- **Traces up**: TSR-035
- **Traces down**: firmware/cvc/src/swc/swc_state.c:Swc_State_EntryAction()
- **Verified by**: TC-CVC-034, TC-CVC-035
- **Status**: draft

The CVC software shall execute the following entry actions upon transitioning to each state:

- **INIT**: Begin self-test sequence, enable OLED, set torque limit to 0%.
- **RUN**: Set torque limit to 100%, speed limit to 100%, clear warning displays.
- **DEGRADED**: Set torque limit to 75%, speed limit to 50%, display warning on OLED, send DEGRADED state on CAN.
- **LIMP**: Set torque limit to 30%, speed limit to 20%, display LIMP warning on OLED, send LIMP state on CAN.
- **SAFE_STOP**: Set torque to 0%, disable motor via CAN, request brakes via CAN, display STOP on OLED, store state to NVM (SSR-CVC-019).
- **SHUTDOWN**: Set all outputs to safe default, store final DTCs, stop feeding watchdog.

---

### SSR-CVC-018: Vehicle State CAN Broadcast

- **ASIL**: D
- **Traces up**: TSR-036
- **Traces down**: firmware/cvc/src/swc/swc_state.c:Swc_State_Broadcast()
- **Verified by**: TC-CVC-036
- **Status**: draft

The CVC software shall transmit the vehicle state CAN message on CAN ID 0x100 every 10 ms with E2E protection. The payload shall contain: vehicle state (4-bit), active fault bitmask (16-bit), torque limit percentage (8-bit), speed limit percentage (8-bit), alive counter (4-bit), CRC-8 (8-bit). Total: 6 bytes. The state broadcast shall continue in all states including SAFE_STOP and SHUTDOWN (as long as the CAN controller is operational).

---

### SSR-CVC-019: Vehicle State NVM Persistence

- **ASIL**: D
- **Traces up**: TSR-037
- **Traces down**: firmware/cvc/src/swc/swc_state.c:Swc_State_PersistToNvm()
- **Verified by**: TC-CVC-037, TC-CVC-038
- **Status**: draft

The CVC software shall write the current vehicle state and active fault bitmask to non-volatile memory (dedicated flash sector or backup SRAM) when transitioning to SAFE_STOP or SHUTDOWN. The write shall use a dual-copy scheme: two copies at different flash addresses, each with a CRC-32 checksum. On startup, the software shall read both copies, compare CRC-32, and use the copy with a valid CRC. If both copies have valid but different CRC values, the copy with the higher state severity (SHUTDOWN > SAFE_STOP) shall be used. If neither copy has a valid CRC, the software shall assume a clean start (INIT state).

---

### SSR-CVC-020: CVC CAN Bus Loss Detection

- **ASIL**: C
- **Traces up**: TSR-038
- **Traces down**: firmware/cvc/src/swc/swc_can_monitor.c:Swc_CanMonitor_Check()
- **Verified by**: TC-CVC-039, TC-CVC-040
- **Status**: draft

The CVC software shall monitor CAN bus health by: (a) checking the CAN controller error counters every 10 ms, flagging bus-off (TEC > 255) immediately, (b) maintaining a message reception timestamp and flagging CAN silence if no messages are received for 200 ms, (c) tracking the error warning threshold (TEC or REC > 96) and flagging CAN degraded if sustained for 500 ms. On CAN bus loss, the software shall transition to SAFE_STOP, set torque to 0, and display CAN FAULT on OLED.

---

### SSR-CVC-021: CVC CAN Bus Recovery Attempt

- **ASIL**: C
- **Traces up**: TSR-039
- **Traces down**: firmware/cvc/src/swc/swc_can_monitor.c:Swc_CanMonitor_Recovery()
- **Verified by**: TC-CVC-041
- **Status**: draft

The CVC software shall attempt CAN bus recovery after bus-off by enabling automatic retransmission per the CAN 2.0B protocol. The software shall count recovery attempts: if 3 recovery attempts fail within 10 seconds, the software shall disable the CAN controller and transition to SHUTDOWN. During recovery, the vehicle state shall remain SAFE_STOP (motor disabled, brakes applied via last valid command to FZC).

---

### SSR-CVC-022: CVC OLED Warning Display Update

- **ASIL**: B
- **Traces up**: TSR-043
- **Traces down**: firmware/cvc/src/swc/swc_display.c:Swc_Display_Update()
- **Verified by**: TC-CVC-042
- **Status**: draft

The CVC software shall update the SSD1306 OLED display via I2C within 200 ms of any vehicle state transition from RUN to a non-RUN state. The display shall render: line 1 = state name (e.g., "DEGRADED"), line 2 = triggering fault code (e.g., "F001: PEDAL"), line 3 = limits (e.g., "TQ:75% SPD:50%"). During non-RUN states, the display shall be refreshed at 5 Hz (200 ms). The OLED update function shall have lower RTOS priority than all safety-critical runnables and shall not block safety task execution.

---

### SSR-CVC-023: CVC WCET Compliance for Safety Runnables

- **ASIL**: D
- **Traces up**: TSR-046, TSR-047
- **Traces down**: firmware/cvc/src/rte/rte_schedule.c:Rte_Schedule()
- **Verified by**: TC-CVC-043, TC-CVC-044
- **Status**: draft

The CVC RTOS scheduler shall configure safety-critical runnables at the following periods and priorities:

| Runnable | Period | RTOS Priority | WCET Budget |
|----------|--------|---------------|-------------|
| Swc_PedalPlausibility | 10 ms | High (2) | 500 us |
| Swc_TorqueTransmit | 10 ms | High (2) | 200 us |
| Swc_StateProcess | 10 ms | High (2) | 300 us |
| Swc_HeartbeatTransmit | 50 ms | Medium (3) | 200 us |
| Swc_WatchdogFeed | 100 ms | Medium (3) | 100 us |
| Swc_DisplayUpdate | 200 ms | Low (4) | 2000 us |

Safety runnables (priority 2) shall preempt QM runnables (priority 4). WCET analysis shall confirm no runnable exceeds its budget under worst-case conditions.

---

## 5. FZC — Front Zone Controller Software Safety Requirements

### SSR-FZC-001: Steering Angle Sensor SPI Read

- **ASIL**: D
- **Traces up**: TSR-010
- **Traces down**: firmware/fzc/src/swc/swc_steering.c:Swc_Steering_ReadSensor()
- **Verified by**: TC-FZC-001, TC-FZC-002
- **Status**: draft

The FZC software shall read the steering AS5048A angle sensor via SPI1 every 10 ms. The read sequence shall: (a) assert chip-select LOW, (b) transmit 16-bit read command for angle register (0x3FFF), (c) receive 16-bit response, (d) de-assert chip-select HIGH, (e) validate parity and extract 14-bit angle, (f) convert to degrees using the calibration offset (center position = 0 degrees). If SPI fails within 500 us, retry once. If retry fails, set sensor status to COMM_FAULT and use last valid value.

---

### SSR-FZC-002: Steering Command-vs-Feedback Comparison

- **ASIL**: D
- **Traces up**: TSR-011
- **Traces down**: firmware/fzc/src/swc/swc_steering.c:Swc_Steering_CheckFeedback()
- **Verified by**: TC-FZC-003, TC-FZC-004, TC-FZC-005
- **Status**: draft

The FZC software shall compare the commanded steering angle (received from CVC via CAN) with the measured steering angle (from AS5048A) every 10 ms. The software shall maintain a debounce counter that increments when |commanded - measured| exceeds 5 degrees and resets when within threshold. When the counter reaches 5 (50 ms), the software shall declare a steering position fault and initiate return-to-center (SSR-FZC-004).

---

### SSR-FZC-003: Steering Sensor Range and Rate Check

- **ASIL**: D
- **Traces up**: TSR-011
- **Traces down**: firmware/fzc/src/swc/swc_steering.c:Swc_Steering_DiagCheck()
- **Verified by**: TC-FZC-006, TC-FZC-007
- **Status**: draft

The FZC software shall check every steering angle reading for: (a) mechanical range violation (outside -45 to +45 degrees for 2 consecutive cycles = range fault), (b) slew rate violation (change exceeding 3.6 degrees per 10 ms cycle, equivalent to 360 degrees/second = rate fault), (c) AS5048A diagnostic register check every 100 ms for magnetic field or CORDIC errors. On any fault, invoke return-to-center (SSR-FZC-004) and log a DTC.

---

### SSR-FZC-004: Steering Return-to-Center Execution

- **ASIL**: D
- **Traces up**: TSR-012
- **Traces down**: firmware/fzc/src/swc/swc_steering.c:Swc_Steering_ReturnToCenter()
- **Verified by**: TC-FZC-008, TC-FZC-009, TC-FZC-010
- **Status**: draft

Upon steering fault detection, the FZC software shall compute a rate-limited trajectory from the current angle to 0 degrees (center) at a maximum rate of 30 degrees per second. Every 10 ms, the software shall: (a) compute the next target angle (move 0.3 degrees toward center), (b) convert the target angle to servo PWM duty cycle, (c) write the duty cycle to TIM1 channel 1 compare register. The software shall monitor the AS5048A feedback; if the measured angle does not reach within 2 degrees of center within 200 ms, invoke PWM disable (SSR-FZC-006).

---

### SSR-FZC-005: Steering Return-to-Center CAN Notification

- **ASIL**: D
- **Traces up**: TSR-012
- **Traces down**: firmware/fzc/src/swc/swc_steering.c:Swc_Steering_NotifyCVC()
- **Verified by**: TC-FZC-011
- **Status**: draft

Upon initiating return-to-center, the FZC software shall transmit a steering fault CAN message (CAN ID 0x211, E2E protected) to the CVC within 10 ms. The message shall contain: fault type (4-bit: position, range, rate, sensor), current measured angle (16-bit), target angle (16-bit, = 0 for center), alive counter (4-bit), CRC-8 (8-bit). The CVC shall use this to transition to DEGRADED and reduce speed.

---

### SSR-FZC-006: Steering Servo PWM Disable

- **ASIL**: D
- **Traces up**: TSR-013
- **Traces down**: firmware/fzc/src/swc/swc_steering.c:Swc_Steering_DisablePWM()
- **Verified by**: TC-FZC-012, TC-FZC-013
- **Status**: draft

If return-to-center fails (200 ms timeout without reaching center), the FZC software shall disable the steering servo PWM by: (a) writing 0 to the TIM1 channel 1 compare register (0% duty), (b) clearing the TIM1 CCER output enable bit for channel 1, (c) reconfiguring the TIM1 channel 1 output pin as a GPIO output and driving it LOW. The software shall set an internal latch preventing PWM re-enable until system restart.

---

### SSR-FZC-007: Steering Rate Limiter

- **ASIL**: C
- **Traces up**: TSR-014
- **Traces down**: firmware/fzc/src/swc/swc_steering.c:Swc_Steering_RateLimit()
- **Verified by**: TC-FZC-014, TC-FZC-015
- **Status**: draft

The FZC software shall apply rate limiting to the steering angle command before converting to PWM. Each 10 ms cycle, the maximum permitted change shall be 0.3 degrees (30 degrees/second * 0.01 s). If the delta between the new command and the current position exceeds 0.3 degrees, the software shall clamp the output to current_position +/- 0.3 degrees. The software shall also clamp the absolute angle to -43 to +43 degrees (2-degree margin from mechanical stops at +/-45 degrees).

---

### SSR-FZC-008: Brake Servo PWM Output and Feedback

- **ASIL**: D
- **Traces up**: TSR-015
- **Traces down**: firmware/fzc/src/swc/swc_brake.c:Swc_Brake_WritePWM()
- **Verified by**: TC-FZC-016, TC-FZC-017
- **Status**: draft

The FZC software shall command the brake servo via TIM2 channel 1 PWM at 50 Hz. The duty cycle shall map linearly: 0% braking = 1.0 ms pulse (5% duty at 50 Hz), 100% braking = 2.0 ms pulse (10% duty at 50 Hz). The software shall read back the TIM2 input capture register to measure the actual output pulse width. If |commanded - measured| duty cycle exceeds 2% for 3 consecutive cycles (30 ms), the software shall declare a brake PWM fault.

---

### SSR-FZC-009: Brake Fault CAN Notification

- **ASIL**: D
- **Traces up**: TSR-016
- **Traces down**: firmware/fzc/src/swc/swc_brake.c:Swc_Brake_NotifyFault()
- **Verified by**: TC-FZC-018
- **Status**: draft

Upon brake system fault detection (PWM mismatch or servo non-response), the FZC software shall transmit a brake fault CAN message (CAN ID 0x210, E2E protected) within 10 ms. The message shall contain: fault type (2-bit), commanded brake value (8-bit, percent), measured feedback (8-bit, percent), alive counter (4-bit), CRC-8 (8-bit). The CVC shall react by commanding motor cutoff to the RZC.

---

### SSR-FZC-010: Auto-Brake on CAN Command Timeout

- **ASIL**: D
- **Traces up**: TSR-017
- **Traces down**: firmware/fzc/src/swc/swc_brake.c:Swc_Brake_AutoBrake()
- **Verified by**: TC-FZC-019, TC-FZC-020, TC-FZC-021
- **Status**: draft

The FZC software shall maintain a timestamp of the last valid brake command CAN message reception (CAN ID 0x200, passing E2E check). If the elapsed time since the last valid message exceeds 100 ms, the software shall command maximum braking (100%, 2.0 ms pulse on TIM2). The auto-brake state shall be latching: release requires 5 consecutive valid CAN brake commands from the CVC with sequential E2E alive counters, confirming stable communication recovery.

---

### SSR-FZC-011: Lidar UART Frame Reception

- **ASIL**: C
- **Traces up**: TSR-018
- **Traces down**: firmware/fzc/src/swc/swc_lidar.c:Swc_Lidar_ReceiveFrame()
- **Verified by**: TC-FZC-022, TC-FZC-023
- **Status**: draft

The FZC software shall receive TFMini-S data via UART2 configured at 115200 baud, 8N1. The software shall parse the 9-byte frame by: (a) scanning for the header bytes 0x59, 0x59, (b) reading the remaining 7 bytes, (c) computing the checksum (low byte of sum of bytes 0-7) and comparing with byte 8. Valid frames shall extract: distance (bytes 2-3, little-endian, centimeters) and signal strength (bytes 4-5, little-endian). Invalid frames shall be discarded. The software shall maintain a reception timestamp; if no valid frame is received for 100 ms, a sensor timeout shall be declared.

---

### SSR-FZC-012: Lidar Graduated Response Logic

- **ASIL**: C
- **Traces up**: TSR-019
- **Traces down**: firmware/fzc/src/swc/swc_lidar.c:Swc_Lidar_EvaluateDistance()
- **Verified by**: TC-FZC-024, TC-FZC-025, TC-FZC-026
- **Status**: draft

The FZC software shall evaluate the lidar distance against three compile-time configurable thresholds (default: warning = 100 cm, braking = 50 cm, emergency = 20 cm). The software shall enforce the invariant emergency < braking < warning at compile time via static assertions. The response shall be: (a) distance <= emergency: set brake to 100%, transmit motor cutoff request (CAN ID 0x211), activate continuous buzzer, (b) distance <= braking: set brake to 50%, transmit speed reduction request, activate fast beep buzzer, (c) distance <= warning: activate single beep buzzer, transmit warning CAN message (ID 0x220). The software shall always act on the most critical (smallest distance) zone.

---

### SSR-FZC-013: Lidar Range and Stuck Sensor Detection

- **ASIL**: C
- **Traces up**: TSR-020
- **Traces down**: firmware/fzc/src/swc/swc_lidar.c:Swc_Lidar_PlausibilityCheck()
- **Verified by**: TC-FZC-027, TC-FZC-028, TC-FZC-029
- **Status**: draft

The FZC software shall check each valid lidar frame for: (a) range: distance below 2 cm or above 1200 cm flagged as out-of-range and replaced with 0 cm safe default, (b) signal strength: values below 100 flagged as unreliable and replaced with 0 cm, (c) stuck detection: maintain a counter that increments when |current - previous| < 1 cm, resets otherwise. When counter reaches 50 (approximately 500 ms), flag as stuck sensor and replace with 0 cm.

---

### SSR-FZC-014: Lidar Fault Safe Default Substitution

- **ASIL**: C
- **Traces up**: TSR-021
- **Traces down**: firmware/fzc/src/swc/swc_lidar.c:Swc_Lidar_FaultHandler()
- **Verified by**: TC-FZC-030, TC-FZC-031
- **Status**: draft

On any lidar fault (timeout, range, stuck, signal strength), the FZC software shall: (a) substitute distance = 0 cm (triggers emergency zone per SSR-FZC-012), (b) log a DTC via Dem with fault type and last valid reading, (c) if fault persists for 200 consecutive cycles (2 seconds), transmit a degradation request to CVC via CAN. The safe default of 0 cm is fail-closed: sensor failure causes the system to assume the worst case (obstacle at minimum distance).

---

### SSR-FZC-015: FZC CAN E2E Transmit

- **ASIL**: D
- **Traces up**: TSR-022, TSR-023
- **Traces down**: firmware/fzc/src/bsw/e2e/e2e_protect.c:E2E_Protect()
- **Verified by**: TC-FZC-032
- **Status**: draft

The FZC software shall implement the E2E protection transmit function identically to SSR-CVC-008, using the same CRC-8 polynomial (0x1D, init 0xFF), alive counter management, and Data ID assignment. Each FZC-originated CAN message shall use a unique Data ID distinct from CVC and RZC Data IDs.

---

### SSR-FZC-016: FZC CAN E2E Transmit Counter Management

- **ASIL**: D
- **Traces up**: TSR-023
- **Traces down**: firmware/fzc/src/bsw/e2e/e2e_protect.c:E2E_IncrementCounter()
- **Verified by**: TC-FZC-033
- **Status**: draft

The FZC software shall maintain per-Data-ID alive counters (16 entries), initialized to 0 at startup, incremented exactly once per transmission per Data ID, matching the specification in SSR-CVC-009.

---

### SSR-FZC-017: FZC CAN E2E Receive

- **ASIL**: D
- **Traces up**: TSR-024
- **Traces down**: firmware/fzc/src/bsw/e2e/e2e_check.c:E2E_Check()
- **Verified by**: TC-FZC-034, TC-FZC-035
- **Status**: draft

The FZC software shall implement the E2E protection receive function identically to SSR-CVC-010, using the same CRC-8 polynomial, alive counter verification, and Data ID check. On 3 consecutive E2E failures for brake commands, the safe default shall be maximum braking (100%). On 3 consecutive failures for steering commands, the safe default shall be center position (0 degrees).

---

### SSR-FZC-018: FZC Heartbeat Transmission

- **ASIL**: C
- **Traces up**: TSR-025
- **Traces down**: firmware/fzc/src/swc/swc_heartbeat.c:Swc_Heartbeat_Transmit()
- **Verified by**: TC-FZC-036
- **Status**: draft

The FZC software shall transmit a heartbeat CAN message on CAN ID 0x011 every 50 ms. Payload: ECU ID (0x02), operating mode (4-bit), fault status bitmask (8-bit: bit 0 = steering fault, bit 1 = brake fault, bit 2 = lidar fault, bit 3 = CAN fault, bits 4-7 reserved), alive counter (4-bit), CRC-8 (8-bit). Total: 4 bytes.

---

### SSR-FZC-019: FZC Heartbeat Conditioning

- **ASIL**: C
- **Traces up**: TSR-026
- **Traces down**: firmware/fzc/src/swc/swc_heartbeat.c:Swc_Heartbeat_CheckConditions()
- **Verified by**: TC-FZC-037
- **Status**: draft

The FZC software shall suppress heartbeat transmission under the same conditions as SSR-CVC-012: main loop not iterating, stack canary corrupted, or CAN bus-off state. This ensures the SC detects a hung or corrupted FZC.

---

### SSR-FZC-020: FZC External Watchdog Feed

- **ASIL**: D
- **Traces up**: TSR-031
- **Traces down**: firmware/fzc/src/swc/swc_watchdog.c:Swc_Watchdog_Feed()
- **Verified by**: TC-FZC-038
- **Status**: draft

The FZC software shall toggle the TPS3823 WDI pin under the same four-condition check as SSR-CVC-013: main loop complete, stack canary intact, RAM test passed, CAN not bus-off. The toggle shall alternate HIGH/LOW each iteration.

---

### SSR-FZC-021: FZC CAN Bus Loss Detection

- **ASIL**: C
- **Traces up**: TSR-038
- **Traces down**: firmware/fzc/src/swc/swc_can_monitor.c:Swc_CanMonitor_Check()
- **Verified by**: TC-FZC-039, TC-FZC-040
- **Status**: draft

The FZC software shall detect CAN bus loss using the same criteria as SSR-CVC-020 (bus-off, 200 ms silence, error warning sustained 500 ms). On CAN bus loss, the FZC shall: (a) apply auto-brake (100% per SSR-FZC-010), (b) command steering to center (per SSR-FZC-004), (c) activate continuous buzzer. The FZC shall not attempt CAN recovery — it shall maintain the safe state until power cycle.

---

### SSR-FZC-022: FZC Buzzer Pattern Generation

- **ASIL**: B
- **Traces up**: TSR-044
- **Traces down**: firmware/fzc/src/swc/swc_buzzer.c:Swc_Buzzer_SetPattern()
- **Verified by**: TC-FZC-041, TC-FZC-042
- **Status**: draft

The FZC software shall drive the piezo buzzer GPIO with the following patterns: (a) DEGRADED: toggle GPIO HIGH for 100 ms then LOW (one-shot), (b) LIMP: toggle 500 ms ON / 500 ms OFF (repeating), (c) SAFE_STOP: toggle 100 ms ON / 100 ms OFF (repeating), (d) Emergency: GPIO HIGH (continuous). The buzzer pattern shall be driven by a software timer running at 10 ms resolution. The buzzer shall be activated within 100 ms of state change or local lidar detection.

---

### SSR-FZC-023: FZC WCET Compliance for Safety Runnables

- **ASIL**: D
- **Traces up**: TSR-046, TSR-047
- **Traces down**: firmware/fzc/src/rte/rte_schedule.c:Rte_Schedule()
- **Verified by**: TC-FZC-043, TC-FZC-044
- **Status**: draft

The FZC RTOS scheduler shall configure safety-critical runnables:

| Runnable | Period | RTOS Priority | WCET Budget |
|----------|--------|---------------|-------------|
| Swc_SteeringMonitor | 10 ms | High (2) | 400 us |
| Swc_BrakeMonitor | 10 ms | High (2) | 300 us |
| Swc_LidarMonitor | 10 ms | High (2) | 400 us |
| Swc_HeartbeatTransmit | 50 ms | Medium (3) | 200 us |
| Swc_WatchdogFeed | 100 ms | Medium (3) | 100 us |
| Swc_BuzzerDriver | 10 ms | Low (4) | 100 us |

Safety runnables shall preempt QM tasks.

---

### SSR-FZC-024: FZC Loss-of-Braking Motor Cutoff Request

- **ASIL**: D
- **Traces up**: TSR-048
- **Traces down**: firmware/fzc/src/swc/swc_brake.c:Swc_Brake_RequestMotorCutoff()
- **Verified by**: TC-FZC-045, TC-FZC-046
- **Status**: draft

When the FZC detects a brake servo fault (SSR-FZC-008 PWM mismatch), the software shall transmit both a brake fault CAN message (SSR-FZC-009) and a motor cutoff request CAN message (CAN ID 0x211, E2E protected) within 10 ms. The motor cutoff request shall contain: request type (motor_cutoff = 0x01), reason (brake_fault = 0x02), alive counter (4-bit), CRC-8 (8-bit). The FZC shall continue transmitting the motor cutoff request every 10 ms until the CVC acknowledges receipt or CAN timeout occurs.

---

## 6. RZC — Rear Zone Controller Software Safety Requirements

### SSR-RZC-001: RZC Motor Driver Disable on Zero-Torque

- **ASIL**: D
- **Traces up**: TSR-005
- **Traces down**: firmware/rzc/src/swc/swc_motor.c:Swc_Motor_Disable()
- **Verified by**: TC-RZC-001, TC-RZC-002
- **Status**: draft

Upon receiving a zero-torque CAN command from CVC (CAN ID 0x100, passing E2E check, torque field = 0%), the RZC software shall within 5 ms: (a) set RPWM compare register to 0 (TIM1 CH1), (b) set LPWM compare register to 0 (TIM1 CH2), (c) drive R_EN GPIO LOW, (d) drive L_EN GPIO LOW. The motor driver shall remain disabled until a valid non-zero torque command is received with valid E2E protection and the CVC vehicle state is RUN or DEGRADED.

---

### SSR-RZC-002: RZC Motor Driver State Validation

- **ASIL**: D
- **Traces up**: TSR-005
- **Traces down**: firmware/rzc/src/swc/swc_motor.c:Swc_Motor_ValidateState()
- **Verified by**: TC-RZC-003
- **Status**: draft

Before applying any non-zero PWM to the motor, the RZC software shall verify: (a) the CVC vehicle state (from CAN) is RUN, DEGRADED, or LIMP (not INIT, SAFE_STOP, or SHUTDOWN), (b) no active motor faults (overcurrent, overtemp, direction plausibility), (c) the torque command has a valid E2E protection. If any check fails, the software shall refuse to enable the motor and maintain the disabled state.

---

### SSR-RZC-003: Motor Current ADC Sampling

- **ASIL**: A
- **Traces up**: TSR-006
- **Traces down**: firmware/rzc/src/swc/swc_current.c:Swc_Current_Sample()
- **Verified by**: TC-RZC-004, TC-RZC-005
- **Status**: draft

The RZC software shall configure ADC1 channel 1 for motor current sensing at 1 kHz (timer-triggered conversion). Each ADC result (12-bit) shall be converted to milliamps: current_mA = (adc_value - adc_zero_offset) * (Vref_mV / 4096) / sensitivity_mV_per_A * 1000. The zero-offset (calibrated at zero current) and sensitivity (400 mV/A for ACS723LLCTR-20AB-T) shall be compile-time constants. A moving average filter (4 samples) shall reduce ADC noise while maintaining sufficient bandwidth for 10 ms overcurrent detection.

---

### SSR-RZC-004: Motor Overcurrent Detection and Cutoff

- **ASIL**: A
- **Traces up**: TSR-006
- **Traces down**: firmware/rzc/src/swc/swc_current.c:Swc_Current_CheckOvercurrent()
- **Verified by**: TC-RZC-006, TC-RZC-007, TC-RZC-008
- **Status**: draft

The RZC software shall compare the filtered motor current against the overcurrent threshold (25000 mA default, calibratable). A debounce counter shall increment each 1 ms sample that exceeds the threshold and reset when below. When the counter reaches 10 (10 ms continuous overcurrent), the software shall disable the motor driver (SSR-RZC-001 disable sequence) within 1 ms and log a DTC via Dem. Motor re-enable shall require current below threshold for 500 ms and a CVC reset command.

---

### SSR-RZC-005: Motor Current CAN Broadcast

- **ASIL**: C
- **Traces up**: TSR-007
- **Traces down**: firmware/rzc/src/swc/swc_current.c:Swc_Current_Broadcast()
- **Verified by**: TC-RZC-009
- **Status**: draft

The RZC software shall transmit motor current data on CAN ID 0x301 every 10 ms with E2E protection. Payload: current magnitude (16-bit, milliamps), motor direction (1-bit: 0 = forward RPWM, 1 = reverse LPWM), motor enable status (1-bit: R_EN OR L_EN), alive counter (4-bit), CRC-8 (8-bit). The broadcast shall continue even when the motor is disabled (reporting 0 mA current) to maintain the SC cross-plausibility data stream.

---

### SSR-RZC-006: Motor Temperature ADC Sampling

- **ASIL**: A
- **Traces up**: TSR-008
- **Traces down**: firmware/rzc/src/swc/swc_temp.c:Swc_Temp_Sample()
- **Verified by**: TC-RZC-010, TC-RZC-011
- **Status**: draft

The RZC software shall configure ADC1 channel 2 for NTC temperature sensing at 10 Hz (100 ms period). The ADC result shall be converted to temperature in degrees Celsius using the Steinhart-Hart equation simplified with the beta parameter: T_C = (1 / (1/T0 + (1/B) * ln(R_ntc/R0))) - 273.15, where T0 = 298.15 K, R0 = 10000 ohms, B = 3950 (calibratable). Readings equivalent to below -30 degrees C (open circuit: R very high, ADC near max) or above 150 degrees C (short circuit: R very low, ADC near zero) shall be flagged as sensor faults.

---

### SSR-RZC-007: Motor Temperature Derating Logic

- **ASIL**: A
- **Traces up**: TSR-009
- **Traces down**: firmware/rzc/src/swc/swc_temp.c:Swc_Temp_ApplyDerating()
- **Verified by**: TC-RZC-012, TC-RZC-013, TC-RZC-014
- **Status**: draft

The RZC software shall enforce the derating curve by maintaining a current_limit_percent variable updated every 100 ms based on the measured temperature. The derating levels shall be: below 60 C = 100%, 60-79 C = 75%, 80-99 C = 50%, 100+ C = 0% (motor disabled), sensor fault = 0%. The PWM duty cycle shall be clamped such that the resulting motor current does not exceed current_limit_percent * rated_current. Hysteresis on recovery: motor re-enable after 100 C shutdown requires temperature below 90 C. State transitions (DEGRADED at 60 C, LIMP at 80 C, SAFE_STOP at 100 C) shall be communicated to CVC via CAN within 10 ms.

---

### SSR-RZC-008: RZC CAN E2E Transmit

- **ASIL**: D
- **Traces up**: TSR-022, TSR-023
- **Traces down**: firmware/rzc/src/bsw/e2e/e2e_protect.c:E2E_Protect()
- **Verified by**: TC-RZC-015
- **Status**: draft

The RZC software shall implement the E2E protection transmit function identically to SSR-CVC-008, using the same CRC-8 polynomial (0x1D, init 0xFF), alive counter management, and Data ID assignment. Each RZC-originated CAN message shall use a unique Data ID distinct from CVC and FZC Data IDs.

---

### SSR-RZC-009: RZC CAN E2E Transmit Counter

- **ASIL**: D
- **Traces up**: TSR-023
- **Traces down**: firmware/rzc/src/bsw/e2e/e2e_protect.c:E2E_IncrementCounter()
- **Verified by**: TC-RZC-016
- **Status**: draft

The RZC software shall maintain per-Data-ID alive counters (16 entries), initialized to 0 at startup, incremented exactly once per transmission, matching SSR-CVC-009.

---

### SSR-RZC-010: RZC CAN E2E Receive

- **ASIL**: D
- **Traces up**: TSR-024
- **Traces down**: firmware/rzc/src/bsw/e2e/e2e_check.c:E2E_Check()
- **Verified by**: TC-RZC-017, TC-RZC-018
- **Status**: draft

The RZC software shall implement the E2E protection receive function identically to SSR-CVC-010. On 3 consecutive E2E failures for the torque command (CAN ID 0x100), the safe default shall be zero torque (motor disabled).

---

### SSR-RZC-011: RZC Heartbeat Transmission

- **ASIL**: C
- **Traces up**: TSR-025
- **Traces down**: firmware/rzc/src/swc/swc_heartbeat.c:Swc_Heartbeat_Transmit()
- **Verified by**: TC-RZC-019
- **Status**: draft

The RZC software shall transmit a heartbeat CAN message on CAN ID 0x012 every 50 ms. Payload: ECU ID (0x03), operating mode (4-bit), fault status bitmask (8-bit: bit 0 = overcurrent, bit 1 = overtemp, bit 2 = direction fault, bit 3 = CAN fault, bits 4-7 reserved), alive counter (4-bit), CRC-8 (8-bit). Total: 4 bytes.

---

### SSR-RZC-012: RZC Heartbeat Conditioning

- **ASIL**: C
- **Traces up**: TSR-026
- **Traces down**: firmware/rzc/src/swc/swc_heartbeat.c:Swc_Heartbeat_CheckConditions()
- **Verified by**: TC-RZC-020
- **Status**: draft

The RZC software shall suppress heartbeat transmission under the same conditions as SSR-CVC-012: main loop stalled, stack canary corrupted, or CAN bus-off.

---

### SSR-RZC-013: RZC External Watchdog Feed

- **ASIL**: D
- **Traces up**: TSR-031
- **Traces down**: firmware/rzc/src/swc/swc_watchdog.c:Swc_Watchdog_Feed()
- **Verified by**: TC-RZC-021
- **Status**: draft

The RZC software shall toggle the TPS3823 WDI pin under the same four-condition check as SSR-CVC-013: main loop complete, stack canary intact, RAM test passed, CAN not bus-off.

---

### SSR-RZC-014: RZC CAN Bus Loss Detection

- **ASIL**: C
- **Traces up**: TSR-038
- **Traces down**: firmware/rzc/src/swc/swc_can_monitor.c:Swc_CanMonitor_Check()
- **Verified by**: TC-RZC-022, TC-RZC-023
- **Status**: draft

The RZC software shall detect CAN bus loss using the same criteria as SSR-CVC-020 (bus-off, 200 ms silence, error warning sustained 500 ms). On CAN bus loss, the RZC shall disable the motor driver (R_EN LOW, L_EN LOW, both PWM = 0) and maintain the disabled state until power cycle. The RZC shall not attempt CAN recovery.

---

### SSR-RZC-015: Motor Direction Plausibility Check

- **ASIL**: C
- **Traces up**: TSR-040
- **Traces down**: firmware/rzc/src/swc/swc_motor.c:Swc_Motor_CheckDirection()
- **Verified by**: TC-RZC-024, TC-RZC-025
- **Status**: draft

The RZC software shall verify motor direction by comparing the commanded direction (positive torque = RPWM active = forward, negative torque = LPWM active = reverse) against the encoder direction (TIM3 count direction: up = forward, down = reverse). If a direction mismatch persists for 50 ms (5 consecutive 10 ms checks with encoder showing wrong direction), the software shall disable the motor and log a DTC. The check shall not apply during the first 100 ms after a direction change (motor inertia settling time).

---

### SSR-RZC-016: Motor Shoot-Through Protection

- **ASIL**: C
- **Traces up**: TSR-040
- **Traces down**: firmware/rzc/src/swc/swc_motor.c:Swc_Motor_ShootThroughGuard()
- **Verified by**: TC-RZC-026, TC-RZC-027
- **Status**: draft

The RZC software shall prevent simultaneous non-zero PWM on both RPWM and LPWM by: (a) before writing any PWM value, checking that the opposite channel is at 0% duty, (b) during direction changes, enforcing a dead-time sequence: set both channels to 0%, wait 10 us (verified by timer), then enable the new direction channel. The TIM1 complementary output with dead-time insertion shall be configured as hardware backup (ARR-based dead-time of 10 us). A runtime check shall verify that both RPWM and LPWM compare registers are never simultaneously non-zero; if detected, both channels shall be forced to 0% and a shoot-through fault logged.

---

### SSR-RZC-017: RZC WCET Compliance for Safety Runnables

- **ASIL**: D
- **Traces up**: TSR-046, TSR-047
- **Traces down**: firmware/rzc/src/rte/rte_schedule.c:Rte_Schedule()
- **Verified by**: TC-RZC-028, TC-RZC-029
- **Status**: draft

The RZC RTOS scheduler shall configure safety-critical runnables:

| Runnable | Period | RTOS Priority | WCET Budget |
|----------|--------|---------------|-------------|
| Swc_CurrentMonitor | 1 ms | Highest (1) | 100 us |
| Swc_MotorControl | 10 ms | High (2) | 400 us |
| Swc_DirectionMonitor | 10 ms | High (2) | 200 us |
| Swc_TempMonitor | 100 ms | Medium (3) | 300 us |
| Swc_HeartbeatTransmit | 50 ms | Medium (3) | 200 us |
| Swc_WatchdogFeed | 100 ms | Medium (3) | 100 us |

The 1 kHz current monitor at highest priority ensures 10 ms overcurrent detection.

---

## 7. SC — Safety Controller Software Safety Requirements

### SSR-SC-001: SC CAN E2E Receive — Listen-Only

- **ASIL**: D
- **Traces up**: TSR-022, TSR-024
- **Traces down**: firmware/sc/src/sc_e2e.c:SC_E2E_Check()
- **Verified by**: TC-SC-001, TC-SC-002
- **Status**: draft

The SC software shall receive and validate all safety-critical CAN messages using the E2E check function (CRC-8 verification, alive counter check, Data ID verification). The SC operates in CAN listen-only mode (DCAN1 TEST register bit 3 set) and shall never transmit on the CAN bus. The SC shall maintain independent E2E state (expected alive counters) for each monitored message type: CVC torque request (0x100), RZC motor current (0x301), CVC heartbeat (0x010), FZC heartbeat (0x011), RZC heartbeat (0x012).

---

### SSR-SC-002: SC CAN Message Filtering

- **ASIL**: D
- **Traces up**: TSR-024
- **Traces down**: firmware/sc/src/sc_can.c:SC_CAN_Init()
- **Verified by**: TC-SC-003
- **Status**: draft

The SC software shall configure DCAN1 message acceptance filters to accept only the CAN IDs required for safety monitoring: 0x001 (E-stop), 0x010 (CVC heartbeat), 0x011 (FZC heartbeat), 0x012 (RZC heartbeat), 0x100 (vehicle state / torque request), 0x301 (motor current). All other CAN IDs shall be filtered out at the hardware level to reduce processing overhead and eliminate potential for spurious message processing.

---

### SSR-SC-003: SC Heartbeat Timeout Counter Management

- **ASIL**: C
- **Traces up**: TSR-027
- **Traces down**: firmware/sc/src/sc_heartbeat.c:SC_Heartbeat_Monitor()
- **Verified by**: TC-SC-004, TC-SC-005, TC-SC-006
- **Status**: draft

The SC software shall maintain three independent timeout counters, one per zone ECU (CVC, FZC, RZC). Each counter shall be reset to 0 upon reception of a valid heartbeat message (passing E2E check) from the corresponding ECU. The counters shall be incremented every 10 ms by the SC main loop. When any counter exceeds 15 (150 ms = 3 heartbeat periods), the SC shall declare a heartbeat timeout for that ECU.

---

### SSR-SC-004: SC Fault LED Activation on Heartbeat Timeout

- **ASIL**: C
- **Traces up**: TSR-027
- **Traces down**: firmware/sc/src/sc_heartbeat.c:SC_Heartbeat_FaultLED()
- **Verified by**: TC-SC-007
- **Status**: draft

Upon heartbeat timeout detection for a specific ECU, the SC software shall drive the corresponding fault LED GPIO: GIO_A1 = HIGH for CVC timeout, GIO_A2 = HIGH for FZC timeout, GIO_A3 = HIGH for RZC timeout. The LED shall remain ON as long as the timeout condition persists. If the heartbeat resumes (counter resets), the LED shall turn OFF.

---

### SSR-SC-005: SC Heartbeat Timeout Confirmation and Relay De-energize

- **ASIL**: D
- **Traces up**: TSR-028
- **Traces down**: firmware/sc/src/sc_heartbeat.c:SC_Heartbeat_ConfirmAndRelay()
- **Verified by**: TC-SC-008, TC-SC-009, TC-SC-010
- **Status**: draft

After detecting a heartbeat timeout (SSR-SC-003), the SC software shall start a confirmation timer of 50 ms. During the confirmation period, the SC shall continue monitoring for the missing heartbeat. If the heartbeat is not received by the end of the confirmation period (total elapsed: 150 ms timeout + 50 ms confirmation = 200 ms), the software shall call the kill relay de-energize function (SSR-SC-006). If a valid heartbeat is received during the confirmation period, the software shall cancel the confirmation timer and clear the timeout condition.

---

### SSR-SC-006: SC Kill Relay GPIO Control

- **ASIL**: D
- **Traces up**: TSR-029
- **Traces down**: firmware/sc/src/sc_relay.c:SC_Relay_Control()
- **Verified by**: TC-SC-011, TC-SC-012
- **Status**: draft

The SC software shall control the kill relay via GIO_A0: (a) to energize (close relay): write GIO_A0 = HIGH, read back GIO_A0 data-in register to confirm HIGH, (b) to de-energize (open relay): write GIO_A0 = LOW, read back to confirm LOW. The GIO_A0 shall be initialized to LOW (relay open, safe state) during startup. The relay shall only be energized after successful completion of the startup self-test sequence (SSR-SC-016). The software shall set a de-energize latch flag that, once set, prevents any code path from re-energizing the relay.

---

### SSR-SC-007: SC Kill Relay De-energize Trigger Logic

- **ASIL**: D
- **Traces up**: TSR-030
- **Traces down**: firmware/sc/src/sc_relay.c:SC_Relay_CheckTriggers()
- **Verified by**: TC-SC-013, TC-SC-014, TC-SC-015
- **Status**: draft

The SC software shall de-energize the kill relay when any of the following conditions is confirmed: (a) heartbeat timeout confirmed for any zone ECU (SSR-SC-005), (b) cross-plausibility fault detected (SSR-SC-012), (c) startup self-test failure (SSR-SC-016), (d) runtime self-test failure (SSR-SC-017), (e) lockstep CPU ESM interrupt received (immediate, no confirmation), (f) GPIO readback mismatch on GIO_A0 (relay state disagrees with commanded state). The de-energize latch flag shall be set immediately and shall not be clearable by software (only by power cycle).

---

### SSR-SC-008: SC External Watchdog Feed

- **ASIL**: D
- **Traces up**: TSR-031
- **Traces down**: firmware/sc/src/sc_watchdog.c:SC_Watchdog_Feed()
- **Verified by**: TC-SC-016
- **Status**: draft

The SC software shall toggle the TPS3823 WDI pin once per main loop iteration, conditioned on: (a) main loop complete (all monitoring functions executed), (b) RAM test pattern intact (32-byte 0xAA/0x55 at reserved address), (c) DCAN1 not in bus-off state, (d) lockstep ESM error flag not asserted. If any condition fails, the watchdog shall not be toggled.

---

### SSR-SC-009: SC CAN Bus Loss Detection

- **ASIL**: C
- **Traces up**: TSR-038
- **Traces down**: firmware/sc/src/sc_can.c:SC_CAN_MonitorBus()
- **Verified by**: TC-SC-017
- **Status**: draft

The SC software shall detect CAN bus loss by monitoring the DCAN1 error status register and the message reception timestamp. If no CAN messages are received for 200 ms, the SC shall treat this as equivalent to all heartbeats timing out simultaneously and initiate the confirmation-and-relay sequence (SSR-SC-005) for all three zone ECUs. The SC shall also detect CAN bus-off via the DCAN error passive/bus-off status bits.

---

### SSR-SC-010: SC Torque-Current Cross-Plausibility Lookup

- **ASIL**: C
- **Traces up**: TSR-041
- **Traces down**: firmware/sc/src/sc_plausibility.c:SC_Plausibility_ComputeExpected()
- **Verified by**: TC-SC-018, TC-SC-019
- **Status**: draft

The SC software shall maintain a 16-entry const lookup table mapping torque request percentage (0%, 7%, 13%, 20%, 27%, 33%, 40%, 47%, 53%, 60%, 67%, 73%, 80%, 87%, 93%, 100%) to expected motor current in milliamps. The table shall be calibrated during hardware integration testing. For torque values between table entries, the SC shall use linear interpolation. The SC shall extract the torque request from the CVC state message (CAN ID 0x100, byte 4) and the actual current from the RZC current message (CAN ID 0x301, bytes 2-3).

---

### SSR-SC-011: SC Cross-Plausibility Debounce and Fault Detection

- **ASIL**: C
- **Traces up**: TSR-041
- **Traces down**: firmware/sc/src/sc_plausibility.c:SC_Plausibility_Check()
- **Verified by**: TC-SC-020, TC-SC-021, TC-SC-022
- **Status**: draft

The SC software shall compare the expected current (from lookup, SSR-SC-010) with the actual current (from RZC CAN message) every 10 ms. If |expected - actual| > 20% of expected value (or > 2000 mA absolute when expected is near zero), the software shall increment a debounce counter. The counter resets when the difference is within threshold. When the counter reaches 5 (50 ms continuous fault), the software shall declare a cross-plausibility fault and invoke relay de-energize (SSR-SC-007).

---

### SSR-SC-012: SC Cross-Plausibility Fault Reaction

- **ASIL**: C
- **Traces up**: TSR-042
- **Traces down**: firmware/sc/src/sc_plausibility.c:SC_Plausibility_FaultReaction()
- **Verified by**: TC-SC-023
- **Status**: draft

Upon cross-plausibility fault detection, the SC software shall: (a) drive GIO_A4 HIGH (system fault LED), (b) call the kill relay de-energize function (SSR-SC-006), (c) set the de-energize latch flag. The fault LED shall remain ON permanently (until power cycle).

---

### SSR-SC-013: SC Fault LED Management

- **ASIL**: B
- **Traces up**: TSR-045
- **Traces down**: firmware/sc/src/sc_led.c:SC_LED_Update()
- **Verified by**: TC-SC-024, TC-SC-025
- **Status**: draft

The SC software shall update fault LED states every 10 ms based on the monitoring results: (a) GIO_A1 (CVC): ON if CVC heartbeat timeout, BLINK (500 ms) if CVC reports degraded in heartbeat, OFF if CVC normal, (b) GIO_A2 (FZC): same logic for FZC, (c) GIO_A3 (RZC): same logic for RZC, (d) GIO_A4 (system): ON if cross-plausibility fault or self-test failure. The blinking state shall be generated by toggling the GPIO with a 500 ms period software timer.

---

### SSR-SC-014: SC FTTI Monitoring

- **ASIL**: D
- **Traces up**: TSR-046
- **Traces down**: firmware/sc/src/sc_main.c:SC_Main_Loop()
- **Verified by**: TC-SC-026
- **Status**: draft

The SC main loop shall execute at a 10 ms period. Each iteration shall execute in the following order: (a) receive and validate CAN messages, (b) update heartbeat timeout counters, (c) perform cross-plausibility check, (d) evaluate relay trigger conditions, (e) update fault LEDs, (f) feed watchdog. The total loop execution time shall not exceed 2 ms to maintain the 10 ms cycle with adequate margin. The SC shall use a hardware timer to measure loop execution time and flag an overrun DTC if any iteration exceeds 5 ms.

---

### SSR-SC-015: SC Backup Motor Cutoff via Kill Relay

- **ASIL**: D
- **Traces up**: TSR-049
- **Traces down**: firmware/sc/src/sc_plausibility.c:SC_Plausibility_BackupCutoff()
- **Verified by**: TC-SC-027, TC-SC-028
- **Status**: draft

The SC software shall monitor for the brake-fault-motor-cutoff scenario: if the FZC heartbeat fault bitmask indicates a brake fault (bit 1), and the RZC motor current remains above 1000 mA for 100 ms after the brake fault is reported, the SC shall conclude that the CVC-RZC motor cutoff chain has failed and shall de-energize the kill relay (SSR-SC-007). This provides a hardware-enforced backup for the software motor cutoff path.

---

### SSR-SC-016: SC Startup Self-Test Sequence

- **ASIL**: C
- **Traces up**: TSR-050
- **Traces down**: firmware/sc/src/sc_selftest.c:SC_SelfTest_Startup()
- **Verified by**: TC-SC-029, TC-SC-030, TC-SC-031
- **Status**: draft

The SC software shall execute the following self-test sequence at power-on before energizing the kill relay:

1. Trigger lockstep CPU built-in self-test via ESM and verify no error flag set.
2. Execute PBIST (Programmable Built-In Self-Test) for RAM covering the full 256 KB.
3. Compute CRC-32 over application flash (0x00000000 to end of application) and compare with reference CRC stored in the last flash sector.
4. Initialize DCAN1 in loopback mode, transmit a test frame, verify reception, then switch to listen-only mode.
5. Drive GIO_A0 through GIO_A4 to known states and read back via data-in registers.
6. Illuminate all 4 fault LEDs for 500 ms (lamp test).
7. Toggle TPS3823 WDI and verify RESET pin is de-asserted.

If any test fails, set GIO_A4 to blink at the test step number (1 blink = step 1, etc.) and halt. Do not energize kill relay.

---

### SSR-SC-017: SC Runtime Periodic Self-Test

- **ASIL**: C
- **Traces up**: TSR-051
- **Traces down**: firmware/sc/src/sc_selftest.c:SC_SelfTest_Runtime()
- **Verified by**: TC-SC-032, TC-SC-033
- **Status**: draft

The SC software shall execute a runtime self-test every 60 seconds: (a) recompute flash CRC-32 and compare with reference (spread over 6 seconds to avoid blocking the main loop — compute 1/60th per main loop iteration), (b) write/read/compare a 32-byte test pattern to a reserved RAM region, (c) check DCAN1 status register for bus-off or error passive, (d) read back GIO_A0 and verify it matches the commanded relay state. If any test fails, invoke relay de-energize (SSR-SC-007).

---

## 8. Requirements Traceability Summary

### 8.1 TSR to SSR Mapping

| TSR | SSRs Derived |
|-----|-------------|
| TSR-001 | SSR-CVC-001, SSR-CVC-002 |
| TSR-002 | SSR-CVC-003, SSR-CVC-004 |
| TSR-003 | SSR-CVC-005 |
| TSR-004 | SSR-CVC-006, SSR-CVC-007 |
| TSR-005 | SSR-RZC-001, SSR-RZC-002 |
| TSR-006 | SSR-RZC-003, SSR-RZC-004 |
| TSR-007 | SSR-RZC-005 |
| TSR-008 | SSR-RZC-006 |
| TSR-009 | SSR-RZC-007 |
| TSR-010 | SSR-FZC-001 |
| TSR-011 | SSR-FZC-002, SSR-FZC-003 |
| TSR-012 | SSR-FZC-004, SSR-FZC-005 |
| TSR-013 | SSR-FZC-006 |
| TSR-014 | SSR-FZC-007 |
| TSR-015 | SSR-FZC-008 |
| TSR-016 | SSR-FZC-009 |
| TSR-017 | SSR-FZC-010 |
| TSR-018 | SSR-FZC-011 |
| TSR-019 | SSR-FZC-012 |
| TSR-020 | SSR-FZC-013 |
| TSR-021 | SSR-FZC-014 |
| TSR-022 | SSR-CVC-008, SSR-FZC-015, SSR-RZC-008, SSR-SC-001 |
| TSR-023 | SSR-CVC-009, SSR-FZC-016, SSR-RZC-009 |
| TSR-024 | SSR-CVC-010, SSR-FZC-017, SSR-RZC-010, SSR-SC-002 |
| TSR-025 | SSR-CVC-011, SSR-FZC-018, SSR-RZC-011 |
| TSR-026 | SSR-CVC-012, SSR-FZC-019, SSR-RZC-012 |
| TSR-027 | SSR-SC-003, SSR-SC-004 |
| TSR-028 | SSR-SC-005 |
| TSR-029 | SSR-SC-006 |
| TSR-030 | SSR-SC-007 |
| TSR-031 | SSR-CVC-013, SSR-FZC-020, SSR-RZC-013, SSR-SC-008 |
| TSR-033 | SSR-CVC-014 |
| TSR-034 | SSR-CVC-015 |
| TSR-035 | SSR-CVC-016, SSR-CVC-017 |
| TSR-036 | SSR-CVC-018 |
| TSR-037 | SSR-CVC-019 |
| TSR-038 | SSR-CVC-020, SSR-FZC-021, SSR-RZC-014, SSR-SC-009 |
| TSR-039 | SSR-CVC-021 |
| TSR-040 | SSR-RZC-015, SSR-RZC-016 |
| TSR-041 | SSR-SC-010, SSR-SC-011 |
| TSR-042 | SSR-SC-012 |
| TSR-043 | SSR-CVC-022 |
| TSR-044 | SSR-FZC-022 |
| TSR-045 | SSR-SC-013 |
| TSR-046 | SSR-CVC-023, SSR-FZC-023, SSR-RZC-017, SSR-SC-014 |
| TSR-047 | SSR-CVC-023, SSR-FZC-023, SSR-RZC-017 |
| TSR-048 | SSR-FZC-024 |
| TSR-049 | SSR-SC-015 |
| TSR-050 | SSR-SC-016 |
| TSR-051 | SSR-SC-017 |

### 8.2 SSR Count per ECU

| ECU | SSR Count | ASIL D Count | ASIL C Count | ASIL B Count | ASIL A Count |
|-----|-----------|-------------|-------------|-------------|-------------|
| CVC | 23 | 15 | 4 | 3 | 0 |
| FZC | 24 | 14 | 7 | 2 | 0 |
| RZC | 17 | 7 | 4 | 0 | 5 |
| SC | 17 | 7 | 8 | 1 | 0 |
| **Total** | **81** | **43** | **23** | **6** | **5** |

### 8.3 ASIL Distribution

| ASIL | Count | Percentage |
|------|-------|-----------|
| D | 43 | 53% |
| C | 23 | 28% |
| B | 6 | 7% |
| A | 5 | 6% |
| **Total** | **81** | |

Note: Some SSRs may have slightly different ASIL counts due to multiple trace-up paths. The total reflects unique SSR IDs.

## 9. Open Items and Assumptions

### 9.1 Assumptions

| ID | Assumption | Impact |
|----|-----------|--------|
| SSR-A-001 | FreeRTOS is used as the RTOS on all STM32G474 ECUs | WCET analysis and priority assignment depend on RTOS scheduler behavior |
| SSR-A-002 | The SC runs bare-metal (no RTOS) with a cooperative main loop | SC timing analysis assumes no preemption |
| SSR-A-003 | E2E library code is shared across CVC, FZC, and RZC via firmware/shared/bsw/ | Single implementation reduces divergence risk |
| SSR-A-004 | Flash sector erase time for NVM operations does not exceed 50 ms | CVC state persistence timing |

### 9.2 Open Items

| ID | Item | Owner | Target |
|----|------|-------|--------|
| SSR-O-001 | Define Data ID assignment table for all CAN messages | System Architect | Before SWE.2 |
| SSR-O-002 | Calibrate ACS723 zero-offset on target hardware | Integration Engineer | Hardware integration |
| SSR-O-003 | Calibrate NTC beta parameter on target motor | Integration Engineer | Hardware integration |
| SSR-O-004 | Calibrate torque-to-current lookup table | Integration Engineer | Hardware integration |
| SSR-O-005 | Perform WCET analysis for all safety runnables | SW Engineer | SWE.3 phase |
| SSR-O-006 | Verify RTOS scheduling configuration prevents priority inversion | SW Engineer | SWE.3 phase |

## 10. Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 0.1 | 2026-02-21 | System | Initial stub |
| 1.0 | 2026-02-21 | System | Complete SSR specification: 81 requirements across 4 ECUs (CVC: 23, FZC: 24, RZC: 17, SC: 17), full traceability |
