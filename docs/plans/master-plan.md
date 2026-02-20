# Master Plan: Mini Vehicle Platform — 7-ECU ASIL D Portfolio

**Status**: IN PROGRESS
**Created**: 2026-02-20
**Target**: 14 working days
**Goal**: Hire-ready automotive functional safety portfolio

---

## Phase Table

| Phase | Name | Days | Status |
|-------|------|------|--------|
| 0 | Project Setup & Architecture Docs | 1 | DONE |
| 1 | Safety Concept (HARA, Safety Goals, FSC) | 1 | PENDING |
| 2 | Safety Analysis (FMEA, DFA, SPFM/LFM) | 1 | PENDING |
| 3 | Requirements & Architecture | 1 | PENDING |
| 4 | CAN Protocol & HSI Design | 1 | PENDING |
| 5 | Firmware: VCU (Vehicle Control Unit) | 1 | PENDING |
| 6 | Firmware: PCU (Powertrain Control Unit) | 1 | PENDING |
| 7 | Firmware: BCU + SCU (Brake + Steering) | 1 | PENDING |
| 8 | Firmware: ADAS + BMS | 1 | PENDING |
| 9 | Firmware: Safety MCU | 1 | PENDING |
| 10 | Unit Tests + Coverage | 1 | PENDING |
| 11 | Hardware Assembly + Integration | 1 | PENDING |
| 12 | Demo Scenarios + Video | 1 | PENDING |
| 13 | Safety Case + Portfolio Polish | 1 | PENDING |

---

## Phase 0: Project Setup & Architecture Docs ✅
- [x] Repository scaffold
- [x] .claude/rules/ — 28 rule files (embedded + ISO 26262)
- [x] .claude/hooks/ — lint-firmware, protect-files
- [x] .claude/skills/ — security-review, plan-feature, firmware-build
- [x] Git Flow branching (main → develop)
- [x] CLAUDE.md, PROJECT_STATE.md

### DONE Criteria
- [x] All rules in place
- [x] Git Flow configured
- [x] Ready to start safety lifecycle

---

## Phase 1: Safety Concept
- [ ] Item definition (system boundary, functions, interfaces, environment)
- [ ] Hazard Analysis and Risk Assessment (HARA)
  - [ ] Identify all operational situations
  - [ ] Identify all hazardous events (target: 15+)
  - [ ] Rate each: Severity, Exposure, Controllability
  - [ ] Assign ASIL per hazardous event
- [ ] Safety goals (one per hazardous event or grouped)
- [ ] Safe states definition (per safety goal)
- [ ] FTTI estimation per safety goal
- [ ] Functional safety concept
  - [ ] Safety mechanisms per safety goal
  - [ ] Warning and degradation concept
  - [ ] Driver warning strategy
- [ ] Safety plan

### Files
- `docs/safety/item-definition.md`
- `docs/safety/hara.md`
- `docs/safety/safety-goals.md`
- `docs/safety/functional-safety-concept.md`
- `docs/safety/safety-plan.md`

### DONE Criteria
- [ ] All hazardous events identified and rated
- [ ] Every safety goal has a safe state and FTTI
- [ ] Functional safety concept covers all safety goals

---

## Phase 2: Safety Analysis
- [ ] System-level FMEA (every component, every failure mode)
- [ ] FMEDA — failure rate classification, diagnostic coverage
- [ ] SPFM and LFM calculation per ECU
- [ ] PMHF estimation
- [ ] Dependent Failure Analysis (DFA)
  - [ ] Common cause failure analysis
  - [ ] Cascading failure analysis
- [ ] ASIL decomposition decisions (if any)

### Files
- `docs/safety/fmea.md`
- `docs/safety/dfa.md`
- `docs/safety/hardware-metrics.md`
- `docs/safety/asil-decomposition.md`

### DONE Criteria
- [ ] Every component has failure modes analyzed
- [ ] SPFM, LFM, PMHF numbers calculated
- [ ] DFA covers all cross-ECU dependencies

---

## Phase 3: Requirements & Architecture
- [ ] Technical Safety Requirements (TSR) from safety goals
- [ ] TSR allocation to ECUs
- [ ] Software Safety Requirements (SSR) per ECU
- [ ] Hardware Safety Requirements (HSR) per ECU
- [ ] System architecture document (all 7 ECUs + CAN bus)
- [ ] Software architecture per ECU (modules, interfaces, state machines)
- [ ] Traceability matrix (SG → FSR → TSR → SSR → module)

### Files
- `docs/safety/technical-safety-requirements.md`
- `docs/safety/sw-safety-requirements.md`
- `docs/safety/hw-safety-requirements.md`
- `docs/safety/traceability-matrix.md`
- `docs/aspice/system-architecture.md`
- `docs/aspice/sw-architecture.md`

### DONE Criteria
- [ ] Every safety goal traces to TSR → SSR → architecture element
- [ ] Traceability matrix complete (no gaps)

---

## Phase 4: CAN Protocol & HSI Design
- [ ] CAN message matrix (ID, sender, receiver, DLC, cycle time, signals)
- [ ] E2E protection design (CRC, alive counter per message)
- [ ] Hardware-Software Interface per ECU
- [ ] Pin mapping per ECU
- [ ] Schematic (KiCad or hand-drawn)
- [ ] Bill of Materials

### Files
- `docs/aspice/can-message-matrix.md`
- `docs/safety/hsi-specification.md`
- `hardware/pin-mapping.md`
- `hardware/bom.md`
- `hardware/schematic/` (KiCad or PDF)

### DONE Criteria
- [ ] Every CAN message defined with ID, signals, timing
- [ ] E2E protection specified for safety-critical messages
- [ ] HSI complete for all 7 ECUs

---

## Phase 5: Firmware — VCU (Vehicle Control Unit)
- [ ] HAL: ADC (dual pedal), GPIO (E-stop, LEDs), CAN, I2C (OLED)
- [ ] Pedal sensor module (dual read, filtering, plausibility check)
- [ ] Vehicle state machine (INIT → NORMAL → DEGRADED → LIMP → SAFE)
- [ ] CAN TX: torque request, brake request, steer request
- [ ] CAN RX: status from all ECUs
- [ ] OLED dashboard display
- [ ] E-stop handling (broadcast CAN stop)
- [ ] MISRA compliance pass

### Files
- `firmware/vcu/src/` — hal_adc.c, hal_can.c, hal_gpio.c, pedal.c, state_machine.c, dashboard.c, main.c
- `firmware/vcu/include/` — corresponding headers

### DONE Criteria
- [ ] Dual pedal reading with plausibility check working
- [ ] State machine transitions verified
- [ ] CAN messages transmitting correctly

---

## Phase 6: Firmware — PCU (Powertrain Control Unit)
- [ ] HAL: PWM (motor), ADC (current, temp), CAN
- [ ] Motor control module (torque request → PWM, ramp limiting)
- [ ] Current monitoring (overcurrent detection, cutoff)
- [ ] Temperature monitoring (derating curve, over-temp shutdown)
- [ ] PCU state machine
- [ ] CAN RX: torque request from VCU, emergency brake from ADAS
- [ ] CAN TX: motor status, fault flags

### Files
- `firmware/pcu/src/`
- `firmware/pcu/include/`

### DONE Criteria
- [ ] Motor responds to torque request
- [ ] Overcurrent and overtemp protections working
- [ ] Emergency brake command stops motor

---

## Phase 7: Firmware — BCU + SCU
### BCU (Brake Control Unit)
- [ ] Brake servo control
- [ ] Hill hold (accelerometer-based)
- [ ] Parking brake on standstill
- [ ] CAN RX: brake request from VCU, emergency brake from ADAS
- [ ] Auto-brake on CAN timeout

### SCU (Steering Control Unit)
- [ ] Steering servo control
- [ ] Angle feedback (potentiometer)
- [ ] Rate limiting, angle limiting
- [ ] Return-to-center on fault
- [ ] CAN RX: steer request from VCU

### Files
- `firmware/bcu/src/`
- `firmware/scu/src/`

### DONE Criteria
- [ ] Brake responds to CAN commands
- [ ] Hill hold activates on incline
- [ ] Steering tracks command with angle feedback
- [ ] Both enter safe state on CAN loss

---

## Phase 8: Firmware — ADAS + BMS
### ADAS (Distance Sensing)
- [ ] Ultrasonic sensor reading + filtering
- [ ] Distance thresholds (warning, brake, emergency)
- [ ] Sensor plausibility (timeout, stuck value)
- [ ] CAN TX: distance, emergency brake request
- [ ] Buzzer warning

### BMS (Battery Management)
- [ ] Cell voltage monitoring (voltage dividers)
- [ ] Pack current monitoring
- [ ] Temperature monitoring
- [ ] SOC estimation (simple voltage-based)
- [ ] Fault detection (OV, UV, OC, OT)
- [ ] Kill relay control
- [ ] CAN TX: power available, SOC, fault flags

### Files
- `firmware/adas/src/`
- `firmware/bms/src/`

### DONE Criteria
- [ ] ADAS triggers emergency brake at threshold distance
- [ ] BMS detects overvoltage/overtemp and opens kill relay
- [ ] Both report status over CAN

---

## Phase 9: Firmware — Safety MCU
- [ ] CAN listen-only mode (sniff all traffic)
- [ ] Heartbeat monitoring per ECU (alive counter check)
- [ ] Message timeout detection
- [ ] Cross-plausibility (torque request vs motor current)
- [ ] System kill relay control
- [ ] Fault LED panel (one LED per ECU: green/red)
- [ ] Safety MCU state machine
- [ ] Independent watchdog (hardware)

### Files
- `firmware/safety_mcu/src/`

### DONE Criteria
- [ ] Detects missing heartbeat within 100ms
- [ ] Kills system on CAN silence
- [ ] LED panel shows per-ECU health

---

## Phase 10: Unit Tests + Coverage
- [ ] Test framework setup (Unity for each ECU target)
- [ ] Unit tests for every safety-critical module:
  - [ ] Pedal plausibility (VCU)
  - [ ] Motor current/temp protection (PCU)
  - [ ] Brake logic + hill hold (BCU)
  - [ ] Steering limits + return-to-center (SCU)
  - [ ] Distance thresholds + sensor timeout (ADAS)
  - [ ] BMS fault detection (OV, UV, OC, OT)
  - [ ] Alive monitoring + timeout (Safety MCU)
- [ ] Fault injection tests per module
- [ ] Static analysis (cppcheck + MISRA)
- [ ] Coverage report (statement, branch, MC/DC)

### Files
- `firmware/*/test/`
- `docs/aspice/unit-test-report.md`
- `docs/aspice/coverage-report.md`
- `docs/aspice/static-analysis-report.md`

### DONE Criteria
- [ ] Every safety module has tests
- [ ] MC/DC coverage documented
- [ ] Zero MISRA mandatory violations

---

## Phase 11: Hardware Assembly + Integration
- [ ] Wire all 7 ECUs on breadboards
- [ ] CAN bus wiring (daisy chain with terminators)
- [ ] Power distribution (12V for motors, 3.3/5V for MCUs)
- [ ] Flash all firmware
- [ ] Integration test: CAN messages flowing between all ECUs
- [ ] End-to-end test: pedal → VCU → PCU → motor spins

### Files
- `docs/aspice/integration-test-report.md`
- Photos of assembled platform

### DONE Criteria
- [ ] All 7 ECUs communicating on CAN bus
- [ ] End-to-end pedal-to-motor working
- [ ] No communication errors in 10-minute run

---

## Phase 12: Demo Scenarios + Video
- [ ] Test all 15 fault scenarios
- [ ] Record each scenario on video (10-15 sec each)
- [ ] Create demo reel (3-5 min combined video)
- [ ] Document each scenario result

### Files
- `docs/aspice/system-test-report.md`
- `media/demo-video/` or YouTube link

### DONE Criteria
- [ ] All 15 scenarios demonstrated and recorded
- [ ] Every fault results in correct safe state

---

## Phase 13: Safety Case + Portfolio Polish
- [ ] Safety case document (claims, arguments, evidence)
- [ ] Final traceability verification (no gaps)
- [ ] README.md — portfolio landing page
  - [ ] Project overview + architecture diagram
  - [ ] Photo of assembled platform
  - [ ] Safety lifecycle summary
  - [ ] Links to all work products
  - [ ] Demo video embed
  - [ ] Skills/standards demonstrated
- [ ] Merge develop → release/1.0.0 → main
- [ ] Tag v1.0.0

### Files
- `docs/safety/safety-case.md`
- `README.md`

### DONE Criteria
- [ ] Safety case references all evidence
- [ ] Traceability matrix 100% complete
- [ ] README is portfolio-ready
- [ ] Tagged v1.0.0 on main
