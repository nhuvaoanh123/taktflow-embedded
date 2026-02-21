---
document_id: SWR-SC
title: "Software Requirements — SC"
version: "1.0"
status: draft
aspice_process: SWE.1
ecu: SC
asil: D
date: 2026-02-21
---

# Software Requirements — Safety Controller (SC)

<!-- NO AUTOSAR — independent safety monitor, bare-metal, ~400 LOC -->

## 1. Purpose

This document specifies the complete software requirements for the Safety Controller (SC), the TI TMS570LC43x LaunchPad-based independent safety monitor of the Taktflow Zonal Vehicle Platform. These requirements are derived from system requirements (SYS), technical safety requirements (TSR), and software safety requirements (SSR) per Automotive SPICE 4.0 SWE.1 (Software Requirements Analysis).

The SC is bare-metal firmware (no RTOS, no AUTOSAR BSW) running a simple cooperative main loop with a 10 ms tick. Total code size target: approximately 400 lines of C. The SC monitors all zone ECUs via CAN listen-only mode, performs cross-plausibility checks, controls the kill relay, drives fault LEDs, and relies on the TMS570's hardware lockstep CPU for computation error detection.

## 2. Referenced Documents

| Document ID | Title | Version |
|-------------|-------|---------|
| SYS | System Requirements Specification | 1.0 |
| TSR | Technical Safety Requirements | 1.0 |
| SSR | Software Safety Requirements | 1.0 |
| HSI | Hardware-Software Interface Specification | 0.1 |
| SYSARCH | System Architecture | 1.0 |
| CAN-MATRIX | CAN Message Matrix | 0.1 |

## 3. Requirement Conventions

Same conventions as SWR-CVC document section 3. Note: all SC requirements trace down to `firmware/sc/src/` (no BSW subdirectory -- SC is bare-metal).

---

## 4. CAN Listen-Only Requirements

### SWR-SC-001: DCAN1 Silent Mode Configuration

- **ASIL**: C
- **Traces up**: SYS-025, TSR-024, SSR-SC-001
- **Traces down**: firmware/sc/src/sc_can.c:SC_CAN_Init()
- **Verified by**: TC-SC-001, TC-SC-002
- **Verification method**: Unit test + PIL + hardware test
- **Status**: draft

The SC software shall configure DCAN1 for listen-only (silent) mode by setting bit 3 of the DCAN TEST register. This mode enables the SC to receive all CAN messages on the bus without transmitting any frames, ACKs, or error frames. The initialization shall: (a) enable DCAN1 peripheral clock, (b) set baud rate to 500 kbps (matching the system CAN bus), (c) enter initialization mode, (d) set the TEST register bit 3 (silent mode), (e) configure acceptance masks and filters per SWR-SC-002, (f) exit initialization mode. The software shall verify the TEST register readback confirms silent mode is active. The SC shall never call any CAN transmit function.

---

### SWR-SC-002: CAN Message Acceptance Filtering

- **ASIL**: D
- **Traces up**: SYS-025, TSR-024, SSR-SC-002
- **Traces down**: firmware/sc/src/sc_can.c:SC_CAN_Init()
- **Verified by**: TC-SC-003
- **Verification method**: Unit test + PIL
- **Status**: draft

The SC software shall configure DCAN1 message acceptance filters to accept only the CAN IDs required for safety monitoring:

| Mailbox | CAN ID | Message | Purpose |
|---------|--------|---------|---------|
| 1 | 0x001 | E-stop | E-stop detection |
| 2 | 0x010 | CVC heartbeat | CVC alive monitoring |
| 3 | 0x011 | FZC heartbeat | FZC alive monitoring |
| 4 | 0x012 | RZC heartbeat | RZC alive monitoring |
| 5 | 0x100 | Vehicle state / torque request | Cross-plausibility input |
| 6 | 0x301 | Motor current | Cross-plausibility input |

All other CAN IDs shall be filtered out at the hardware level to reduce processing overhead and eliminate potential for spurious message processing. Each mailbox shall be configured with an exact-match acceptance mask (all ID bits checked).

---

### SWR-SC-003: CAN E2E Receive Validation

- **ASIL**: D
- **Traces up**: SYS-032, TSR-022, TSR-024, SSR-SC-001
- **Traces down**: firmware/sc/src/sc_e2e.c:SC_E2E_Check()
- **Verified by**: TC-SC-004, TC-SC-005
- **Verification method**: Unit test + SIL + fault injection
- **Status**: draft

The SC software shall validate all received safety-critical CAN messages using E2E check: (a) extract alive counter and Data ID from byte 0, (b) recompute CRC-8 (polynomial 0x1D, init 0xFF) over bytes 2-7 plus Data ID and compare with byte 1, (c) verify the alive counter has incremented by exactly 1 from the previously received value. The SC shall maintain independent E2E state (expected alive counters) for each monitored message type: CVC torque request (0x100), RZC motor current (0x301), CVC heartbeat (0x010), FZC heartbeat (0x011), RZC heartbeat (0x012). On E2E failure, the message shall be discarded and a per-message-type E2E failure counter incremented. If the E2E failure counter for any message type exceeds 3 consecutive failures, the SC shall treat it as equivalent to a heartbeat timeout for that ECU.

---

## 5. Heartbeat Monitoring Requirements

### SWR-SC-004: Per-ECU Heartbeat Timeout Counter

- **ASIL**: C
- **Traces up**: SYS-022, TSR-027, SSR-SC-003
- **Traces down**: firmware/sc/src/sc_heartbeat.c:SC_Heartbeat_Monitor()
- **Verified by**: TC-SC-006, TC-SC-007, TC-SC-008
- **Verification method**: Unit test + SIL + fault injection
- **Status**: draft

The SC software shall maintain three independent timeout counters, one per zone ECU (CVC, FZC, RZC). Each counter shall be reset to 0 upon reception of a valid heartbeat message (passing E2E check) from the corresponding ECU. The counters shall be incremented every 10 ms by the SC main loop. When any counter exceeds 15 (150 ms = 3 heartbeat periods), the SC shall declare a heartbeat timeout for that ECU.

---

### SWR-SC-005: Fault LED Activation on Heartbeat Timeout

- **ASIL**: QM
- **Traces up**: SYS-046, TSR-027, SSR-SC-004
- **Traces down**: firmware/sc/src/sc_heartbeat.c:SC_Heartbeat_FaultLED()
- **Verified by**: TC-SC-009
- **Verification method**: PIL + hardware test
- **Status**: draft

Upon heartbeat timeout detection for a specific ECU, the SC software shall drive the corresponding fault LED GPIO: GIO_A1 = HIGH for CVC timeout, GIO_A2 = HIGH for FZC timeout, GIO_A3 = HIGH for RZC timeout. The LED shall remain ON as long as the timeout condition persists. If the heartbeat resumes (counter resets), the LED shall turn OFF.

---

### SWR-SC-006: Heartbeat Timeout Confirmation and Kill Relay

- **ASIL**: D
- **Traces up**: SYS-022, SYS-024, TSR-028, SSR-SC-005
- **Traces down**: firmware/sc/src/sc_heartbeat.c:SC_Heartbeat_ConfirmAndRelay()
- **Verified by**: TC-SC-010, TC-SC-011, TC-SC-012
- **Verification method**: Unit test + SIL + fault injection
- **Status**: draft

After detecting a heartbeat timeout (SWR-SC-004), the SC software shall start a confirmation timer of 50 ms. During the confirmation period, the SC shall continue monitoring for the missing heartbeat. If the heartbeat is not received by the end of the confirmation period (total elapsed: 150 ms timeout + 50 ms confirmation = 200 ms), the software shall call the kill relay de-energize function (SWR-SC-008). If a valid heartbeat is received during the confirmation period, the software shall cancel the confirmation timer and clear the timeout condition.

---

## 6. Cross-Plausibility Requirements

### SWR-SC-007: Torque-Current Lookup Table

- **ASIL**: C
- **Traces up**: SYS-023, TSR-041, SSR-SC-010
- **Traces down**: firmware/sc/src/sc_plausibility.c:SC_Plausibility_ComputeExpected()
- **Verified by**: TC-SC-018, TC-SC-019
- **Verification method**: Unit test + SIL
- **Status**: draft

The SC software shall maintain a 16-entry const lookup table in flash mapping torque request percentage (0%, 7%, 13%, 20%, 27%, 33%, 40%, 47%, 53%, 60%, 67%, 73%, 80%, 87%, 93%, 100%) to expected motor current in milliamps. The table shall be calibrated during hardware integration testing (default values: linear mapping 0-25000 mA). For torque values between table entries, the SC shall use linear interpolation. The SC shall extract the torque request from the CVC state message (CAN ID 0x100, byte 4) and the actual current from the RZC current message (CAN ID 0x301, bytes 2-3).

---

### SWR-SC-008: Cross-Plausibility Debounce and Fault Detection

- **ASIL**: C
- **Traces up**: SYS-023, TSR-041, SSR-SC-011
- **Traces down**: firmware/sc/src/sc_plausibility.c:SC_Plausibility_Check()
- **Verified by**: TC-SC-020, TC-SC-021, TC-SC-022
- **Verification method**: Unit test + SIL + fault injection
- **Status**: draft

The SC software shall compare the expected current (from lookup, SWR-SC-007) with the actual current (from RZC CAN message) every 10 ms. If |expected - actual| > 20% of expected value (or > 2000 mA absolute when expected is near zero), the software shall increment a debounce counter. The counter resets when the difference is within threshold. When the counter reaches 5 (50 ms continuous fault), the software shall declare a cross-plausibility fault and invoke relay de-energize (SWR-SC-010).

---

### SWR-SC-009: Cross-Plausibility Fault Reaction

- **ASIL**: C
- **Traces up**: SYS-023, TSR-042, SSR-SC-012
- **Traces down**: firmware/sc/src/sc_plausibility.c:SC_Plausibility_FaultReaction()
- **Verified by**: TC-SC-023
- **Verification method**: SIL + hardware test
- **Status**: draft

Upon cross-plausibility fault detection, the SC software shall: (a) drive GIO_A4 HIGH (system fault LED), (b) call the kill relay de-energize function (SWR-SC-010), (c) set the de-energize latch flag. The fault LED shall remain ON permanently (until power cycle).

---

## 7. Kill Relay Control Requirements

### SWR-SC-010: Kill Relay GPIO Control

- **ASIL**: D
- **Traces up**: SYS-024, TSR-029, SSR-SC-006
- **Traces down**: firmware/sc/src/sc_relay.c:SC_Relay_Control()
- **Verified by**: TC-SC-013, TC-SC-014
- **Verification method**: PIL + hardware test
- **Status**: draft

The SC software shall control the kill relay via GIO_A0: (a) to energize (close relay): write GIO_A0 = HIGH, read back GIO_A0 data-in register to confirm HIGH, (b) to de-energize (open relay): write GIO_A0 = LOW, read back to confirm LOW. The GIO_A0 shall be initialized to LOW (relay open, safe state) during startup. The relay shall only be energized after successful completion of the startup self-test sequence (SWR-SC-019). The de-energize shall complete within 5 ms of the trigger condition. The software shall set a de-energize latch flag that, once set, prevents any code path from re-energizing the relay until the next power cycle.

---

### SWR-SC-011: Kill Relay De-energize Trigger Logic

- **ASIL**: D
- **Traces up**: SYS-024, TSR-030, SSR-SC-007
- **Traces down**: firmware/sc/src/sc_relay.c:SC_Relay_CheckTriggers()
- **Verified by**: TC-SC-015, TC-SC-016, TC-SC-017
- **Verification method**: Unit test + SIL + fault injection
- **Status**: draft

The SC software shall de-energize the kill relay when any of the following conditions is confirmed: (a) heartbeat timeout confirmed for any zone ECU (SWR-SC-006), (b) cross-plausibility fault detected (SWR-SC-009), (c) startup self-test failure (SWR-SC-019), (d) runtime self-test failure (SWR-SC-020), (e) lockstep CPU ESM interrupt received (immediate, no confirmation), (f) GPIO readback mismatch on GIO_A0 (relay state disagrees with commanded state). The de-energize latch flag shall be set immediately and shall not be clearable by software (only by power cycle).

---

### SWR-SC-012: Kill Relay GPIO Readback Verification

- **ASIL**: D
- **Traces up**: SYS-024, TSR-029
- **Traces down**: firmware/sc/src/sc_relay.c:SC_Relay_VerifyState()
- **Verified by**: TC-SC-024, TC-SC-025
- **Verification method**: Unit test + PIL
- **Status**: draft

The SC software shall verify the kill relay GPIO state every 10 ms (once per main loop) by reading the GIO_A0 data-in register and comparing with the commanded state. If the readback does not match the commanded state for 2 consecutive checks (20 ms), the software shall declare a relay GPIO fault and invoke the de-energize sequence (SWR-SC-011 condition f). This detects GPIO driver failures, stuck-at faults, and wiring faults.

---

## 8. Fault LED Requirements

### SWR-SC-013: Fault LED Panel Management

- **ASIL**: QM
- **Traces up**: SYS-046, TSR-045, SSR-SC-013
- **Traces down**: firmware/sc/src/sc_led.c:SC_LED_Update()
- **Verified by**: TC-SC-026, TC-SC-027
- **Verification method**: PIL + hardware test
- **Status**: draft

The SC software shall update fault LED states every 10 ms based on the monitoring results:

| GPIO | LED | Off | Blink (500ms) | Steady On |
|------|-----|-----|---------------|-----------|
| GIO_A1 | CVC | CVC normal | CVC degraded (heartbeat shows degraded mode) | CVC heartbeat timeout |
| GIO_A2 | FZC | FZC normal | FZC degraded | FZC heartbeat timeout |
| GIO_A3 | RZC | RZC normal | RZC degraded | RZC heartbeat timeout |
| GIO_A4 | System | No fault | — | Cross-plausibility or self-test failure |

The blinking state shall be generated by toggling the GPIO with a 500 ms period software timer (25 main loop iterations on, 25 off). LED state changes shall occur within one main loop iteration (10 ms) of the triggering event.

---

## 9. Lockstep CPU Requirements

### SWR-SC-014: Lockstep ESM Configuration

- **ASIL**: D
- **Traces up**: SYS-026, TSR-030
- **Traces down**: firmware/sc/src/sc_esm.c:SC_ESM_Init()
- **Verified by**: TC-SC-028, TC-SC-029
- **Verification method**: Unit test + PIL + fault injection
- **Status**: draft

The SC software shall enable the TMS570LC43x Error Signaling Module (ESM) for lockstep CPU comparison error detection. The ESM shall be configured to: (a) enable ESM group 1 notification for lockstep compare error (channel 2), (b) register the ESM high-level interrupt handler, (c) configure the ESM to assert the error pin on lockstep compare error. On ESM interrupt for lockstep error, the handler shall immediately call the kill relay de-energize function (SWR-SC-010) without confirmation delay. The ESM configuration shall be verified at startup by reading back the ESM enable registers.

---

### SWR-SC-015: Lockstep ESM Interrupt Handler

- **ASIL**: D
- **Traces up**: SYS-026
- **Traces down**: firmware/sc/src/sc_esm.c:SC_ESM_HighLevelInterrupt()
- **Verified by**: TC-SC-030
- **Verification method**: Fault injection + PIL
- **Status**: draft

The SC ESM high-level interrupt handler shall: (a) read the ESM status register to identify the error source, (b) if lockstep compare error (group 1, channel 2): immediately write GIO_A0 = LOW (de-energize relay), set the de-energize latch, set GIO_A4 = HIGH (system fault LED), (c) clear the ESM error flag, (d) enter an infinite loop (the TPS3823 watchdog will reset the MCU since the main loop is halted). The handler shall execute in fewer than 100 clock cycles to ensure relay de-energize is immediate.

---

## 10. Startup Self-Test Requirements

### SWR-SC-016: SC Startup Self-Test -- Lockstep CPU

- **ASIL**: D
- **Traces up**: SYS-026, TSR-050, SSR-SC-016
- **Traces down**: firmware/sc/src/sc_selftest.c:SC_SelfTest_Lockstep()
- **Verified by**: TC-SC-031
- **Verification method**: PIL + hardware test
- **Status**: draft

The SC software shall trigger the TMS570LC43x lockstep CPU built-in self-test via the ESM module at startup. The self-test shall: (a) trigger the CPU compare logic test, (b) wait for ESM to report the test result, (c) verify no lockstep error flag is set. If the lockstep self-test fails, the SC shall not energize the kill relay and shall blink GIO_A4 once per second (1 blink pattern = step 1 failure).

---

### SWR-SC-017: SC Startup Self-Test -- RAM PBIST

- **ASIL**: D
- **Traces up**: TSR-050, SSR-SC-016
- **Traces down**: firmware/sc/src/sc_selftest.c:SC_SelfTest_RAM()
- **Verified by**: TC-SC-032
- **Verification method**: PIL + hardware test
- **Status**: draft

The SC software shall execute the TMS570 Programmable Built-In Self-Test (PBIST) for RAM covering the full 256 KB at startup. The PBIST shall use March 13N algorithm for full stuck-at and coupling fault coverage. If PBIST fails, the SC shall not energize the kill relay and shall blink GIO_A4 twice per second (2 blink pattern = step 2 failure).

---

### SWR-SC-018: SC Startup Self-Test -- Flash CRC

- **ASIL**: C
- **Traces up**: TSR-050, SSR-SC-016
- **Traces down**: firmware/sc/src/sc_selftest.c:SC_SelfTest_FlashCRC()
- **Verified by**: TC-SC-033
- **Verification method**: Unit test + PIL
- **Status**: draft

The SC software shall compute CRC-32 over the application flash region (0x00000000 to end of application, size determined at link time) and compare with the reference CRC stored in the last flash sector. The CRC polynomial shall be 0x04C11DB7 (standard CRC-32). If the CRC does not match, the SC shall not energize the kill relay and shall blink GIO_A4 three times per second (3 blink pattern = step 3 failure).

---

### SWR-SC-019: SC Startup Self-Test Sequence

- **ASIL**: D
- **Traces up**: SYS-024, SYS-026, SYS-027, TSR-050, SSR-SC-016
- **Traces down**: firmware/sc/src/sc_selftest.c:SC_SelfTest_Startup()
- **Verified by**: TC-SC-034, TC-SC-035, TC-SC-036
- **Verification method**: Unit test + PIL + fault injection
- **Status**: draft

The SC software shall execute the following self-test sequence at power-on before energizing the kill relay:

1. **Lockstep CPU BIST**: Per SWR-SC-016.
2. **RAM PBIST**: Per SWR-SC-017.
3. **Flash CRC**: Per SWR-SC-018.
4. **CAN controller test**: Initialize DCAN1 in internal loopback mode, transmit a test frame with known payload (0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55), verify reception matches. Then switch to listen-only mode. Failure: blink GIO_A4 4 times (step 4).
5. **GPIO readback**: Drive GIO_A0 through GIO_A4 to known states (alternating HIGH/LOW) and read back via data-in registers. Verify each pin matches the commanded state. Failure: blink GIO_A4 5 times (step 5).
6. **Fault LED lamp test**: Illuminate all 4 fault LEDs (GIO_A1-A4) for 500 ms, then turn off. This is a visual confirmation to the operator.
7. **Watchdog test**: Toggle TPS3823 WDI pin and verify the RESET pin is de-asserted (read back a GPIO connected to the TPS3823 reset output). Failure: blink GIO_A4 7 times (step 7).

If all tests pass, energize the kill relay (GIO_A0 = HIGH) and verify readback. If any test fails, do NOT energize the relay, set GIO_A4 to the failure blink pattern, and halt (watchdog will eventually reset for retry on next power cycle).

---

## 11. Runtime Self-Test Requirements

### SWR-SC-020: SC Runtime Periodic Self-Test

- **ASIL**: D
- **Traces up**: TSR-051, SSR-SC-017
- **Traces down**: firmware/sc/src/sc_selftest.c:SC_SelfTest_Runtime()
- **Verified by**: TC-SC-037, TC-SC-038
- **Verification method**: Unit test + SIL
- **Status**: draft

The SC software shall execute a runtime self-test every 60 seconds:

1. **Flash CRC verification**: Recompute CRC-32 over application flash and compare with reference. To avoid blocking the main loop, compute 1/600th of the flash per main loop iteration (spread over 6 seconds at 10 ms/iteration). Maintain partial CRC state between iterations.
2. **RAM test**: Write/read/compare a 32-byte test pattern (0xAA/0x55 alternating) to a reserved RAM region. This is non-destructive to application data.
3. **CAN status check**: Read DCAN1 error status register. Verify not in bus-off or error passive state.
4. **GPIO readback of GIO_A0**: Verify relay GPIO matches commanded state per SWR-SC-012.

If any runtime self-test fails, invoke relay de-energize (SWR-SC-011 condition d) and set GIO_A4 = HIGH.

---

### SWR-SC-021: SC Stack Canary Check

- **ASIL**: D
- **Traces up**: TSR-031
- **Traces down**: firmware/sc/src/sc_selftest.c:SC_SelfTest_StackCanary()
- **Verified by**: TC-SC-039
- **Verification method**: Unit test + fault injection
- **Status**: draft

The SC software shall write a 32-bit canary value (0xDEADBEEF) at the bottom of the stack during initialization. Every main loop iteration, the software shall read the canary location and compare with the expected value. If the canary is corrupted (stack overflow), the software shall immediately de-energize the kill relay and halt. The stack canary check shall execute before the watchdog feed to ensure the watchdog is not fed if the stack is corrupted.

---

## 12. Watchdog Requirements

### SWR-SC-022: SC External Watchdog Feed

- **ASIL**: D
- **Traces up**: SYS-027, TSR-031, SSR-SC-008
- **Traces down**: firmware/sc/src/sc_watchdog.c:SC_Watchdog_Feed()
- **Verified by**: TC-SC-040
- **Verification method**: Unit test + fault injection + PIL
- **Status**: draft

The SC software shall toggle the TPS3823 WDI pin once per main loop iteration, conditioned on: (a) main loop complete (all monitoring functions executed -- heartbeat check, plausibility check, relay trigger evaluation, LED update, self-test increment), (b) RAM test pattern intact (32-byte 0xAA/0x55 at reserved address), (c) DCAN1 not in bus-off state, (d) lockstep ESM error flag not asserted, (e) stack canary intact. If any condition fails, the watchdog shall not be toggled, and the TPS3823 shall reset the MCU after the 1.6 second timeout.

---

## 13. CAN Bus Loss Requirements

### SWR-SC-023: SC CAN Bus Loss Detection

- **ASIL**: C
- **Traces up**: SYS-034, TSR-038, SSR-SC-009
- **Traces down**: firmware/sc/src/sc_can.c:SC_CAN_MonitorBus()
- **Verified by**: TC-SC-041
- **Verification method**: SIL + fault injection
- **Status**: draft

The SC software shall detect CAN bus loss by monitoring the DCAN1 error status register and the message reception timestamp. If no CAN messages are received for 200 ms, the SC shall treat this as equivalent to all heartbeats timing out simultaneously and initiate the confirmation-and-relay sequence (SWR-SC-006) for all three zone ECUs. The SC shall also detect CAN bus-off via the DCAN error passive/bus-off status bits and suppress the watchdog feed if bus-off is detected.

---

## 14. Backup Motor Cutoff Requirements

### SWR-SC-024: SC Backup Motor Cutoff via Kill Relay

- **ASIL**: D
- **Traces up**: TSR-049, SSR-SC-015
- **Traces down**: firmware/sc/src/sc_plausibility.c:SC_Plausibility_BackupCutoff()
- **Verified by**: TC-SC-042, TC-SC-043
- **Verification method**: SIL + hardware test
- **Status**: draft

The SC software shall monitor for the brake-fault-motor-cutoff scenario: if the FZC heartbeat fault bitmask indicates a brake fault (bit 1), and the RZC motor current (from CAN ID 0x301) remains above 1000 mA for 100 ms after the brake fault is reported, the SC shall conclude that the CVC-RZC motor cutoff chain has failed and shall de-energize the kill relay (SWR-SC-011). This provides a hardware-enforced backup for the software motor cutoff path.

---

## 15. Main Loop Requirements

### SWR-SC-025: SC Main Loop Structure and Timing

- **ASIL**: D
- **Traces up**: SYS-053, TSR-046, SSR-SC-014
- **Traces down**: firmware/sc/src/sc_main.c:SC_Main_Loop()
- **Verified by**: TC-SC-044, TC-SC-045
- **Verification method**: Analysis (WCET) + PIL timing measurement
- **Status**: draft

The SC main loop shall execute at a 10 ms period using a hardware timer interrupt to set a tick flag. Each iteration shall execute the following functions in order:

1. `SC_CAN_Receive()` -- Read and validate all pending CAN messages.
2. `SC_Heartbeat_Monitor()` -- Update heartbeat timeout counters.
3. `SC_Plausibility_Check()` -- Perform cross-plausibility comparison.
4. `SC_Relay_CheckTriggers()` -- Evaluate relay de-energize conditions.
5. `SC_LED_Update()` -- Update fault LED states.
6. `SC_SelfTest_Runtime()` -- Increment runtime self-test (1 step per iteration).
7. `SC_SelfTest_StackCanary()` -- Verify stack canary.
8. `SC_Watchdog_Feed()` -- Feed watchdog if all checks passed.

The total loop execution time shall not exceed 2 ms to maintain the 10 ms cycle with adequate margin. The SC shall use a hardware timer to measure loop execution time. If any iteration exceeds 5 ms, a loop overrun shall be flagged internally (no DTC system on SC -- the overrun flag suppresses the watchdog feed, causing a reset).

---

## 16. SC Initialization Requirements

### SWR-SC-026: SC System Initialization

- **ASIL**: D
- **Traces up**: SYS-024, SYS-025, SYS-026, SYS-027
- **Traces down**: firmware/sc/src/sc_main.c:SC_Init()
- **Verified by**: TC-SC-046
- **Verification method**: PIL
- **Status**: draft

The SC software shall execute the following initialization sequence at power-on:

1. Initialize system clocks and peripheral clocks.
2. Configure GIO: A0 as output (LOW = relay safe), A1-A4 as outputs (LOW = LEDs off).
3. Configure the 10 ms tick timer (RTI module or GIO timer).
4. Configure the loop execution timer for WCET measurement.
5. Initialize stack canary (0xDEADBEEF at stack bottom).
6. Initialize RAM test pattern (32-byte 0xAA/0x55 at reserved address).
7. Execute startup self-test sequence (SWR-SC-019).
8. If self-test passes: energize kill relay, enter main loop.
9. If self-test fails: blink failure pattern on GIO_A4, halt (watchdog resets).

The total initialization time (including self-test) shall not exceed 5 seconds. The watchdog must be fed during initialization to prevent premature reset.

---

## 17. Traceability Summary

### 17.1 SWR to Upstream Mapping

| SWR-SC | SYS | TSR | SSR-SC |
|--------|-----|-----|--------|
| SWR-SC-001 | SYS-025 | TSR-024 | SSR-SC-001 |
| SWR-SC-002 | SYS-025 | TSR-024 | SSR-SC-002 |
| SWR-SC-003 | SYS-032 | TSR-022, TSR-024 | SSR-SC-001 |
| SWR-SC-004 | SYS-022 | TSR-027 | SSR-SC-003 |
| SWR-SC-005 | SYS-046 | TSR-027 | SSR-SC-004 |
| SWR-SC-006 | SYS-022, SYS-024 | TSR-028 | SSR-SC-005 |
| SWR-SC-007 | SYS-023 | TSR-041 | SSR-SC-010 |
| SWR-SC-008 | SYS-023 | TSR-041 | SSR-SC-011 |
| SWR-SC-009 | SYS-023 | TSR-042 | SSR-SC-012 |
| SWR-SC-010 | SYS-024 | TSR-029 | SSR-SC-006 |
| SWR-SC-011 | SYS-024 | TSR-030 | SSR-SC-007 |
| SWR-SC-012 | SYS-024 | TSR-029 | — |
| SWR-SC-013 | SYS-046 | TSR-045 | SSR-SC-013 |
| SWR-SC-014 | SYS-026 | TSR-030 | — |
| SWR-SC-015 | SYS-026 | — | — |
| SWR-SC-016 | SYS-026 | TSR-050 | SSR-SC-016 |
| SWR-SC-017 | — | TSR-050 | SSR-SC-016 |
| SWR-SC-018 | — | TSR-050 | SSR-SC-016 |
| SWR-SC-019 | SYS-024, SYS-026, SYS-027 | TSR-050 | SSR-SC-016 |
| SWR-SC-020 | — | TSR-051 | SSR-SC-017 |
| SWR-SC-021 | — | TSR-031 | — |
| SWR-SC-022 | SYS-027 | TSR-031 | SSR-SC-008 |
| SWR-SC-023 | SYS-034 | TSR-038 | SSR-SC-009 |
| SWR-SC-024 | — | TSR-049 | SSR-SC-015 |
| SWR-SC-025 | SYS-053 | TSR-046 | SSR-SC-014 |
| SWR-SC-026 | SYS-024, SYS-025, SYS-026, SYS-027 | — | — |

### 17.2 ASIL Distribution

| ASIL | Count | SWR IDs |
|------|-------|---------|
| D | 14 | SWR-SC-002, 003, 006, 010, 011, 012, 014, 015, 016, 017, 019, 021, 022, 025, 026 |
| C | 7 | SWR-SC-001, 004, 007, 008, 009, 018, 023, 024 |
| QM | 5 | SWR-SC-005, 013, 020 |
| **Total** | **26** | |

### 17.3 Verification Method Summary

| Method | Count |
|--------|-------|
| Unit test | 16 |
| SIL | 10 |
| PIL | 12 |
| Hardware test | 6 |
| Fault injection | 8 |
| Analysis (WCET) | 1 |

---

## 18. Open Items and Assumptions

### 18.1 Assumptions

| ID | Assumption | Impact |
|----|-----------|--------|
| SWR-SC-A-001 | SC runs bare-metal with cooperative main loop (no RTOS) | Timing analysis assumes no preemption |
| SWR-SC-A-002 | TMS570LC43x DCAN1 silent mode works correctly (TEST reg bit 3) | CAN listen-only depends on correct hardware behavior |
| SWR-SC-A-003 | HALCoGen v04.07.01 DCAN1 is used (not DCAN4 due to known mailbox bug) | SWR-SC-001 initialization |
| SWR-SC-A-004 | Kill relay dropout time is less than 10 ms (electromechanical) | FTTI analysis in TSR-046 |
| SWR-SC-A-005 | Total SC code size is approximately 400 LOC | Finishability constraint |

### 18.2 Open Items

| ID | Item | Owner | Target |
|----|------|-------|--------|
| SWR-SC-O-001 | Calibrate torque-to-current lookup table on target hardware | Integration Engineer | Integration |
| SWR-SC-O-002 | Verify DCAN1 silent mode behavior on TMS570LC43x LaunchPad | Integration Engineer | PIL |
| SWR-SC-O-003 | Measure main loop WCET on target hardware | SW Engineer | SWE.3 |
| SWR-SC-O-004 | Verify PBIST execution time does not exceed 2 seconds | SW Engineer | PIL |
| SWR-SC-O-005 | Verify ESM lockstep self-test does not cause unintended reset | SW Engineer | PIL |

---

## 19. Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 0.1 | 2026-02-21 | System | Initial stub |
| 1.0 | 2026-02-21 | System | Complete SWR specification: 26 requirements (SWR-SC-001 to SWR-SC-026), full traceability |
