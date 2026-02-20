# Master Plan: Zonal Vehicle Platform — ASIL D Portfolio

**Status**: IN PROGRESS
**Created**: 2026-02-20
**Updated**: 2026-02-20
**Target**: 14 working days
**Goal**: Hire-ready automotive functional safety + cloud + ML portfolio

---

## Architecture Overview

### Zonal Controller Architecture (Modern E/E)

Unlike traditional domain-based architectures (one ECU per function), this project uses a **zonal architecture** — the modern approach adopted by Tesla, VW, BMW. ECUs are organized by physical vehicle zone with a central computer on top.

```
                                 ┌──────────────────────┐
                                 │      AWS CLOUD       │
                                 │  IoT Core → Grafana  │
                                 └──────────┬───────────┘
                                            │ MQTT
┌───────────────────────────────────────────┐
│  RASPBERRY PI 4 — Edge Gateway            │
│  • CAN telemetry → Cloud                  │
│  • Edge ML inference (anomaly, health)    │
│  • Fault injection GUI (demo scenarios)   │
│  • CAN data logging for ML training       │
│              CANable 2.0 (USB-CAN)        │
└───────────────────┬───────────────────────┘
                    │
════════════════════╪═══════════════ CAN Bus (500 kbps) ════
    │               │              │              │
┌───┴───────┐ ┌─────┴─────┐ ┌─────┴─────┐ ┌─────┴──────┐
│  CENTRAL  │ │FRONT ZONE │ │ REAR ZONE │ │  SAFETY    │
│  VEHICLE  │ │CONTROLLER │ │CONTROLLER │ │ CONTROLLER │
│  COMPUTER │ │           │ │           │ │            │
│ STM32G474 │ │ STM32G474 │ │ STM32G474 │ │  TMS570    │
│           │ │           │ │           │ │  LC43x     │
│• Pedal ×2 │ │• Steering │ │• Motor    │ │            │
│  (AS5048A)│ │  servo    │ │  + PWM    │ │• CAN listen│
│• E-stop   │ │• Brake    │ │• H-bridge │ │  (silent)  │
│• OLED     │ │  servo    │ │• Current  │ │• Heartbeat │
│• Vehicle  │ │• Lidar    │ │  sensor   │ │  monitor   │
│  state    │ │  (TFMini) │ │• Temp     │ │• Kill relay│
│  machine  │ │• Steering │ │  sensors  │ │• Fault LEDs│
│• CAN      │ │  angle    │ │• Encoder  │ │• Ext WDT   │
│  master   │ │  sensor   │ │• Battery  │ │• Lockstep  │
│           │ │• Buzzer   │ │  voltage  │ │  cores     │
└───────────┘ └───────────┘ └───────────┘ └────────────┘
  PHYSICAL      PHYSICAL      PHYSICAL      PHYSICAL
```

### Zonal Mapping (7 Functions → 4 Physical ECUs)

| Zonal ECU | Absorbs | Hardware | ASIL |
|-----------|---------|----------|------|
| Central Vehicle Computer (CVC) | VCU | STM32G474RE Nucleo | D (SW measures) |
| Front Zone Controller (FZC) | BCU + SCU + ADAS | STM32G474RE Nucleo | D (SW measures) |
| Rear Zone Controller (RZC) | PCU + BMS | STM32G474RE Nucleo | D (SW measures) |
| Safety Controller (SC) | Safety MCU | TI TMS570LC43x LaunchPad | D (HW lockstep) |

**Diverse redundancy**: CVC/FZC/RZC use STM32 (ST). Safety Controller uses TMS570 (TI). Different vendor, different architecture = real ISO 26262 diverse redundancy.

### Additional Systems

| System | Hardware | Purpose |
|--------|----------|---------|
| Edge Gateway | Raspberry Pi 4 (2GB) | Cloud telemetry, ML inference, fault injection GUI |
| CAN Analyzer | CANable 2.0 (USB-CAN) | Bus monitoring, SocketCAN interface for Pi |
| Cloud | AWS IoT Core + Grafana | Real-time dashboard, data lake, alerts |

### 12 Demo Fault Scenarios

| # | Scenario | Trigger | Observable Result |
|---|----------|---------|-------------------|
| 1 | Normal driving | Pedal input | Motor spins proportionally |
| 2 | Pedal sensor disagreement | Dual sensor mismatch | Limp mode (motor slows) |
| 3 | Pedal sensor failure | Sensor disconnected | Motor stops |
| 4 | Object detected | Lidar < threshold | Brake engages, motor stops |
| 5 | Motor overcurrent | Current sensor trip | Motor stops (RZC cuts PWM) |
| 6 | Motor overtemp | Temp sensor trip | Motor derates then stops |
| 7 | Steering fault | Angle sensor lost | Servo returns to center |
| 8 | CAN bus loss | Bus disconnected | Safety Controller kills system |
| 9 | ECU hang | Missing heartbeat | Safety Controller kills system |
| 10 | E-stop pressed | Button pressed | Broadcast stop, everything stops |
| 11 | ML anomaly alert | Abnormal current pattern | Cloud dashboard alarm fires |
| 12 | CVC vs Safety disagree | Injected conflict | Safety Controller wins (kill relay) |

---

## Hardware Bill of Materials

| # | Item | Qty | Unit $ | Total $ |
|---|------|-----|--------|---------|
| | **MCUs** | | | |
| 1 | STM32G474RE Nucleo-64 | 3 | $20 | $60 |
| 2 | TI TMS570LC43x LaunchPad (LAUNCHXL2-570LC43) | 1 | $62 | $62 |
| 3 | Raspberry Pi 4 Model B (2GB) | 1 | $45 | $45 |
| | **CAN Bus** | | | |
| 4 | Adafruit CAN Pal (TJA1051T/3 transceiver) | 4 | $4 | $16 |
| 5 | SN65HVD230 breakout (for TMS570) | 1 | $5 | $5 |
| 6 | CANable 2.0 (USB-CAN, for Pi) | 1 | $35 | $35 |
| 7 | 22 AWG twisted pair wire (25 ft) | 1 | $10 | $10 |
| 8 | 120 ohm resistors (bus termination) | 2 | $0.10 | $1 |
| | **Sensors** | | | |
| 9 | AS5048A magnetic angle sensor + magnet | 3 | $15 | $45 |
| 10 | TFMini-S lidar (UART, 0.1-12m, 100Hz) | 1 | $25 | $25 |
| 11 | ACS723 current transducer | 1 | $8 | $8 |
| 12 | NTC 10K thermistors | 3 | $2 | $6 |
| | **Actuators** | | | |
| 13 | 12V brushed DC motor with encoder | 1 | $25 | $25 |
| 14 | BTS7960 H-bridge motor driver | 1 | $10 | $10 |
| 15 | Metal gear servo (brake) | 1 | $20 | $20 |
| 16 | Metal gear servo (steering) | 1 | $20 | $20 |
| | **Safety Hardware** | | | |
| 17 | TPS3823-33DBVT external watchdog IC | 4 | $1.50 | $6 |
| 18 | 30A automotive relay (kill relay) | 1 | $8 | $8 |
| 19 | Industrial E-stop mushroom button | 1 | $8 | $8 |
| 20 | IRLZ44N logic-level MOSFET (relay driver) | 1 | $1 | $1 |
| 21 | 1N4007 flyback diode | 1 | $0.10 | $1 |
| | **UI / Indicators** | | | |
| 22 | SSD1306 OLED 128x64 (I2C) | 1 | $4 | $4 |
| 23 | LEDs (red + green, for fault panel) | 8 | $0.20 | $2 |
| 24 | Piezo buzzer | 1 | $2 | $2 |
| | **Power** | | | |
| 25 | 12V/5A bench power supply | 1 | $20 | $20 |
| 26 | Buck converters (12V→5V, 12V→3.3V) | 3 | $4 | $12 |
| 27 | Raspberry Pi USB-C power supply | 1 | $10 | $10 |
| 28 | MicroSD card 32GB (for Pi) | 1 | $10 | $10 |
| | **Infrastructure** | | | |
| 29 | Mounting board (acrylic or aluminum) | 1 | $20 | $20 |
| 30 | Protoboards + standoffs | 4 | $5 | $20 |
| 31 | Wire kit + JST/Molex connectors | 1 | $25 | $25 |
| 32 | Heat shrink + cable labels | 1 | $10 | $10 |
| | **Test Equipment (optional)** | | | |
| 33 | Rigol DS1054Z oscilloscope | 1 | $400 | $400 |
| | | | **Total** | **~$942** |
| | | | **Without scope** | **~$542** |

**Budget**: $2,000 — leaves ~$1,058 for shipping, spares, and upgrades (e.g., RPLidar A1 at $100 for 360-degree scanning).

---

## Software Toolchains (All Free)

| MCU | IDE | HAL / Drivers | Debug |
|-----|-----|---------------|-------|
| STM32G474 | STM32CubeIDE | STM32 HAL + CubeMX | Onboard ST-LINK/V3 |
| TMS570LC43x | Code Composer Studio | HALCoGen (TUV-certified process) | Onboard XDS110 |
| Raspberry Pi | VS Code + Python | python-can, paho-mqtt, scikit-learn | SSH |

---

## Phase Table

| Phase | Name | Days | Status |
|-------|------|------|--------|
| 0 | Project Setup & Architecture Docs | 1 | DONE |
| 1 | Safety Concept (HARA, Safety Goals, FSC) | 1 | PENDING |
| 2 | Safety Analysis (FMEA, DFA, Hardware Metrics) | 1 | PENDING |
| 3 | Requirements & System Architecture | 1 | PENDING |
| 4 | CAN Protocol & HSI Design | 1 | PENDING |
| 5 | Firmware: Central Vehicle Computer | 1.5 | PENDING |
| 6 | Firmware: Front Zone Controller | 1.5 | PENDING |
| 7 | Firmware: Rear Zone Controller | 1 | PENDING |
| 8 | Firmware: Safety Controller (TMS570) | 1 | PENDING |
| 9 | Edge Gateway: Pi + CAN + Cloud + ML | 1.5 | PENDING |
| 10 | Unit Tests + Coverage | 1 | PENDING |
| 11 | Hardware Assembly + Integration | 1 | PENDING |
| 12 | Demo Scenarios + Video + Portfolio Polish | 1.5 | PENDING |

---

## Phase 0: Project Setup & Architecture Docs ✅

- [x] Repository scaffold
- [x] .claude/rules/ — 28 rule files (embedded + ISO 26262)
- [x] .claude/hooks/ — lint-firmware, protect-files
- [x] .claude/skills/ — security-review, plan-feature, firmware-build
- [x] Git Flow branching (main → develop)
- [x] CLAUDE.md, PROJECT_STATE.md
- [x] Hardware feasibility research (STM32G474, TMS570, CAN bus, sensors)

### DONE Criteria
- [x] All rules in place
- [x] Git Flow configured
- [x] Hardware architecture verified feasible
- [x] Ready to start safety lifecycle

---

## Phase 1: Safety Concept

- [ ] Item definition (system boundary, functions, interfaces, environment)
  - [ ] System: Zonal vehicle platform (4 ECUs + edge gateway)
  - [ ] Functions: drive-by-wire (pedal → motor), steering, braking, distance sensing
  - [ ] Interfaces: CAN bus (500 kbps), lidar (UART), sensors (SPI/ADC), actuators (PWM/GPIO)
  - [ ] Environment: indoor demo platform, 12V power, controlled conditions
- [ ] Hazard Analysis and Risk Assessment (HARA)
  - [ ] Identify all operational situations
  - [ ] Identify all hazardous events (target: 12+)
  - [ ] Rate each: Severity, Exposure, Controllability
  - [ ] Assign ASIL per hazardous event
- [ ] Safety goals (one per hazardous event or grouped)
- [ ] Safe states definition (per safety goal)
- [ ] FTTI estimation per safety goal
- [ ] Functional safety concept
  - [ ] Safety mechanisms per safety goal
  - [ ] Warning and degradation concept
  - [ ] Operator warning strategy (OLED + buzzer + LEDs)
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
  - [ ] CVC failures (pedal sensor, OLED, CAN TX/RX, E-stop)
  - [ ] FZC failures (steering servo, brake servo, lidar, angle sensor, buzzer)
  - [ ] RZC failures (motor driver, current sensor, temp sensor, encoder, battery)
  - [ ] SC failures (CAN listen, heartbeat logic, kill relay, watchdog)
  - [ ] CAN bus failures (open, short, stuck, delayed, corrupted)
  - [ ] Power failures (12V loss, 5V loss, 3.3V loss, ground fault)
- [ ] FMEDA — failure rate classification, diagnostic coverage
  - [ ] TMS570 failure rates from TI safety manual
  - [ ] STM32G474 failure rates from ST safety manual
- [ ] SPFM and LFM calculation per ECU
- [ ] PMHF estimation
- [ ] Dependent Failure Analysis (DFA)
  - [ ] Common cause: shared power supply, shared CAN bus, shared PCB ground
  - [ ] Cascading: CVC fault → wrong torque → RZC overcurrent
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

## Phase 3: Requirements & System Architecture

- [ ] Technical Safety Requirements (TSR) from safety goals
- [ ] TSR allocation to zonal ECUs
- [ ] Software Safety Requirements (SSR) per ECU
  - [ ] CVC: SSR-CVC-001..N
  - [ ] FZC: SSR-FZC-001..N
  - [ ] RZC: SSR-RZC-001..N
  - [ ] SC: SSR-SC-001..N
- [ ] Hardware Safety Requirements (HSR) per ECU
- [ ] System architecture document (4 ECUs + Pi + CAN bus + cloud)
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
  - [ ] CVC → FZC: steer request, brake request (10 ms cycle)
  - [ ] CVC → RZC: torque request (10 ms cycle)
  - [ ] CVC → ALL: vehicle state, E-stop broadcast
  - [ ] FZC → CVC: steering angle, brake status, lidar distance (20 ms cycle)
  - [ ] RZC → CVC: motor status, current, temp, battery voltage (20 ms cycle)
  - [ ] ALL → SC: heartbeat (alive counter, 50 ms cycle)
  - [ ] FZC → CVC/RZC: emergency brake request (event-driven)
- [ ] E2E protection design (CRC-8, alive counter, data ID per safety message)
- [ ] Hardware-Software Interface per ECU
  - [ ] CVC: pin mapping (SPI1, I2C1, FDCAN1, GPIO)
  - [ ] FZC: pin mapping (TIM2, USART1, SPI2, FDCAN1, GPIO)
  - [ ] RZC: pin mapping (TIM3, TIM4, ADC1, FDCAN1, GPIO)
  - [ ] SC: pin mapping (DCAN1, GIO, RTI)
- [ ] Bill of Materials (final, with supplier links)

### Files
- `docs/aspice/can-message-matrix.md`
- `docs/safety/hsi-specification.md`
- `hardware/pin-mapping.md`
- `hardware/bom.md`

### DONE Criteria
- [ ] Every CAN message defined with ID, signals, timing, E2E protection
- [ ] HSI complete for all 4 ECUs
- [ ] Pin assignments verified against Nucleo/LaunchPad schematics

---

## Phase 5: Firmware — Central Vehicle Computer (STM32G474)

- [ ] Project setup (STM32CubeIDE, CubeMX pin config, build system)
- [ ] HAL modules
  - [ ] `hal_can.c` — FDCAN1 init, TX, RX, E2E pack/unpack
  - [ ] `hal_spi.c` — SPI1 for dual AS5048A pedal sensors
  - [ ] `hal_i2c.c` — I2C1 for SSD1306 OLED
  - [ ] `hal_gpio.c` — E-stop input (EXTI), status LEDs
- [ ] Application modules
  - [ ] `pedal.c` — dual sensor read, filtering, plausibility check (|S1-S2| < threshold)
  - [ ] `state_machine.c` — INIT → NORMAL → DEGRADED → LIMP → SAFE_STOP
  - [ ] `dashboard.c` — OLED display (vehicle state, speed, fault indicator)
  - [ ] `can_manager.c` — message dispatch, heartbeat TX, status aggregation
  - [ ] `estop.c` — E-stop detection, broadcast CAN stop message
- [ ] `main.c` — 10 ms main loop, watchdog feed, module tick calls
- [ ] MISRA compliance pass

### Files
- `firmware/cvc/src/` — hal_can.c, hal_spi.c, hal_i2c.c, hal_gpio.c, pedal.c, state_machine.c, dashboard.c, can_manager.c, estop.c, main.c
- `firmware/cvc/include/` — corresponding headers
- `firmware/shared/` — can_protocol.h, e2e.h, e2e.c (shared across all STM32 ECUs)

### DONE Criteria
- [ ] Dual pedal reading with plausibility check working
- [ ] State machine transitions verified
- [ ] CAN messages transmitting and receiving correctly
- [ ] OLED displays vehicle state
- [ ] E-stop triggers broadcast stop

---

## Phase 6: Firmware — Front Zone Controller (STM32G474)

- [ ] Project setup (STM32CubeIDE, CubeMX, link shared/ code)
- [ ] HAL modules
  - [ ] `hal_can.c` — FDCAN1 (shared code from CVC, same driver)
  - [ ] `hal_pwm.c` — TIM2 CH1 (steering servo), TIM2 CH2 (brake servo)
  - [ ] `hal_uart.c` — USART1 for TFMini-S lidar (DMA RX)
  - [ ] `hal_spi.c` — SPI2 for AS5048A steering angle sensor
  - [ ] `hal_gpio.c` — buzzer output
- [ ] Application modules
  - [ ] `steering.c` — servo control, angle feedback, rate limiting, angle limits, return-to-center on fault
  - [ ] `brake.c` — servo control, brake force mapping, auto-brake on CAN timeout
  - [ ] `lidar.c` — TFMini-S frame parser, distance filtering, threshold check (warning/brake/emergency)
  - [ ] `fzc_safety.c` — local plausibility checks, sensor timeout detection, safe state logic
  - [ ] `can_manager.c` — heartbeat TX, command RX, status TX, emergency brake TX
- [ ] `main.c` — 10 ms main loop

### Files
- `firmware/fzc/src/`
- `firmware/fzc/include/`

### DONE Criteria
- [ ] Steering servo tracks angle command with feedback
- [ ] Brake servo responds to brake request
- [ ] Lidar reads distance, triggers emergency brake at threshold
- [ ] Return-to-center on steering sensor fault
- [ ] Auto-brake on CAN timeout

---

## Phase 7: Firmware — Rear Zone Controller (STM32G474)

- [ ] Project setup (STM32CubeIDE, CubeMX, link shared/ code)
- [ ] HAL modules
  - [ ] `hal_can.c` — FDCAN1 (shared driver)
  - [ ] `hal_pwm.c` — TIM3 CH1/CH2 for BTS7960 H-bridge (RPWM/LPWM)
  - [ ] `hal_adc.c` — ADC1 DMA scan: current sensor, thermistors, battery voltage
  - [ ] `hal_encoder.c` — TIM4 encoder mode (hardware quadrature decode)
  - [ ] `hal_gpio.c` — motor enable, status LED
- [ ] Application modules
  - [ ] `motor.c` — torque request → PWM, ramp limiting, direction control
  - [ ] `current_monitor.c` — overcurrent detection, cutoff threshold, filtering
  - [ ] `temp_monitor.c` — derating curve, over-temp shutdown
  - [ ] `battery.c` — voltage monitoring via divider, SOC estimate (simple voltage table)
  - [ ] `rzc_safety.c` — motor safety checks, emergency brake response, safe state
  - [ ] `can_manager.c` — heartbeat TX, torque RX, status TX
- [ ] `main.c` — 10 ms main loop

### Files
- `firmware/rzc/src/`
- `firmware/rzc/include/`

### DONE Criteria
- [ ] Motor responds to torque request proportionally
- [ ] Overcurrent protection cuts motor within 10 ms
- [ ] Over-temp derating and shutdown working
- [ ] Emergency brake command stops motor
- [ ] Battery voltage reported over CAN

---

## Phase 8: Firmware — Safety Controller (TMS570LC43x)

- [ ] Project setup (HALCoGen config + CCS project)
  - [ ] HALCoGen: enable DCAN1, GIO, RTI timer, pinmux
  - [ ] CCS: import HALCoGen project, verify XDS110 connection
  - [ ] Verify with LED toggle and CAN loopback test
- [ ] CAN listen-only mode (DCAN1 silent mode — TEST register bit 3)
- [ ] Heartbeat monitoring
  - [ ] Track alive counter per ECU (CVC, FZC, RZC)
  - [ ] Timeout detection: no heartbeat within 100 ms → fault
  - [ ] Per-ECU health state tracking
- [ ] Cross-plausibility check (torque request vs motor current)
- [ ] Kill relay control (GPIO → MOSFET → relay)
  - [ ] Kill on: any ECU timeout, E-stop received, plausibility failure
  - [ ] Energize-to-run pattern (relay de-energizes = safe state)
- [ ] Fault LED panel (4 LEDs: one per zone ECU, green/red)
- [ ] External watchdog feed (TPS3823 via GPIO toggle, safety-gated)
  - [ ] Only feed watchdog if all safety checks passed this cycle
- [ ] Safety Controller state machine (INIT → MONITORING → FAULT → KILL)

### Files
- `firmware/sc/src/` — main.c, can_monitor.c, heartbeat.c, relay.c, led_panel.c, watchdog.c
- `firmware/sc/include/`

### DONE Criteria
- [ ] Detects missing heartbeat within 100 ms
- [ ] Kills system on CAN silence or ECU hang
- [ ] LED panel shows per-ECU health (green/red)
- [ ] External watchdog resets Safety Controller if it hangs
- [ ] Lockstep CPU error detection demonstrated

---

## Phase 9: Edge Gateway — Raspberry Pi + Cloud + ML

### 9a: CAN Interface + Data Logging
- [ ] CANable 2.0 setup (flash candleLight firmware, gs_usb driver)
- [ ] SocketCAN configuration (`ip link set can0 up type can bitrate 500000`)
- [ ] python-can listener (Notifier + Listener pattern)
- [ ] CAN data logger (CSV/Parquet format for ML training)
- [ ] CAN message decoder (parse all message IDs per CAN matrix)

### 9b: Cloud Telemetry
- [ ] AWS IoT Core setup (thing, certificate, policy)
- [ ] MQTT publisher (paho-mqtt or AWS IoT SDK, TLS on port 8883)
- [ ] Telemetry schema (JSON: ECU status, sensor values, fault flags)
- [ ] Publish rate: 1 message per 5 seconds (batched, stays within free tier)
- [ ] AWS Timestream ingestion (time-series storage)
- [ ] Grafana dashboard
  - [ ] Per-ECU health status indicators
  - [ ] Motor current / temperature time series
  - [ ] Battery voltage gauge
  - [ ] Lidar distance graph
  - [ ] Anomaly score display
  - [ ] Fault event log

### 9c: Edge ML Models
- [ ] Data collection: run normal + fault scenarios, log CAN data
- [ ] Model 1: Motor Health Score (Random Forest)
  - [ ] Features: current mean/variance, temp trend, current-to-torque ratio
  - [ ] Train on PC (scikit-learn), export with joblib
  - [ ] Deploy on Pi, inference at 1 Hz
- [ ] Model 2: CAN Bus Anomaly Detection (Isolation Forest)
  - [ ] Features: message frequency per ID, payload byte distributions, timing jitter
  - [ ] Train on normal baseline, detect injected anomalies
  - [ ] Deploy on Pi, inference at 1 Hz
- [ ] Alert pipeline: anomaly score > threshold → MQTT alert → Grafana alarm

### 9d: Fault Injection GUI
- [ ] Python GUI (tkinter or web-based Flask)
  - [ ] Buttons per demo scenario (1-12)
  - [ ] Live CAN bus status display
  - [ ] Scenario result logging
- [ ] CAN message injection (simulate faults from Pi)

### Files
- `gateway/can_listener.py`
- `gateway/cloud_publisher.py`
- `gateway/ml_inference.py`
- `gateway/fault_injector.py`
- `gateway/models/` — trained model files
- `gateway/config.py` — CAN IDs, MQTT topics, thresholds
- `gateway/requirements.txt`
- `docs/aspice/cloud-architecture.md`

### DONE Criteria
- [ ] Pi receives all CAN messages in real-time
- [ ] Grafana dashboard shows live telemetry
- [ ] Motor health model produces scores
- [ ] Anomaly detector flags injected faults
- [ ] Fault injection GUI triggers demo scenarios

---

## Phase 10: Unit Tests + Coverage

- [ ] Test framework setup (Unity for STM32, CCS test for TMS570, pytest for Pi)
- [ ] Unit tests for every safety-critical module:
  - [ ] `pedal.c` — plausibility check (agree, disagree, one failed, both failed)
  - [ ] `steering.c` — angle limits, rate limiting, return-to-center
  - [ ] `brake.c` — auto-brake on timeout, brake force mapping
  - [ ] `lidar.c` — distance thresholds, sensor timeout, stuck value
  - [ ] `motor.c` — ramp limiting, direction, torque mapping
  - [ ] `current_monitor.c` — overcurrent detection, threshold, filtering
  - [ ] `temp_monitor.c` — derating curve, shutdown threshold
  - [ ] `heartbeat.c` — timeout detection, alive counter validation
  - [ ] `e2e.c` — CRC calculation, alive counter wrap
- [ ] Fault injection tests per module
- [ ] Static analysis (cppcheck + MISRA subset)
- [ ] Coverage report (statement, branch, MC/DC for safety-critical)

### Files
- `firmware/cvc/test/`
- `firmware/fzc/test/`
- `firmware/rzc/test/`
- `firmware/sc/test/`
- `gateway/tests/`
- `docs/aspice/unit-test-report.md`
- `docs/aspice/coverage-report.md`
- `docs/aspice/static-analysis-report.md`

### DONE Criteria
- [ ] Every safety module has tests
- [ ] MC/DC coverage documented for critical modules
- [ ] Zero MISRA mandatory violations
- [ ] All tests pass

---

## Phase 11: Hardware Assembly + Integration

- [ ] Mount 4 ECU boards on platform
- [ ] CAN bus wiring (daisy chain: CVC → FZC → RZC → SC, 120Ω terminators at CVC and SC)
- [ ] Power distribution (12V supply → buck converters → 5V/3.3V rails)
- [ ] Sensor/actuator wiring per HSI spec
  - [ ] Dual pedal sensors → CVC SPI
  - [ ] OLED → CVC I2C
  - [ ] E-stop button → CVC GPIO
  - [ ] Steering servo + brake servo → FZC PWM
  - [ ] TFMini-S lidar → FZC UART
  - [ ] Steering angle sensor → FZC SPI
  - [ ] Motor + H-bridge → RZC PWM
  - [ ] Current sensor → RZC ADC
  - [ ] Thermistors → RZC ADC
  - [ ] Kill relay → SC GPIO (via MOSFET)
  - [ ] Fault LEDs → SC GPIO
  - [ ] External watchdogs → each ECU GPIO
- [ ] Pi + CANable connected to CAN bus
- [ ] Flash all firmware
- [ ] Integration test: CAN messages flowing between all ECUs
- [ ] End-to-end test: pedal → CVC → RZC → motor spins
- [ ] Safety chain test: fault → SC detects → kill relay opens → motor stops
- [ ] Cloud test: Pi → AWS → Grafana shows live data

### Files
- `docs/aspice/integration-test-report.md`
- Photos of assembled platform

### DONE Criteria
- [ ] All 4 ECUs + Pi communicating on CAN bus
- [ ] End-to-end pedal-to-motor working
- [ ] Safety chain (heartbeat → timeout → kill) working
- [ ] Cloud dashboard receiving live data
- [ ] No communication errors in 10-minute run

---

## Phase 12: Demo Scenarios + Video + Portfolio Polish

### 12a: Demo Scenarios
- [ ] Execute and record all 12 fault scenarios
- [ ] Each scenario: setup → trigger → observable result → CAN trace
- [ ] Document results in system test report

### 12b: Video
- [ ] Record each scenario on video (10-15 sec each)
- [ ] Create demo reel (3-5 min combined)
- [ ] Include: platform overview, normal operation, fault scenarios, cloud dashboard, ML alerts

### 12c: Safety Case
- [ ] Safety case document (claims, arguments, evidence)
- [ ] Final traceability verification (no gaps in matrix)

### 12d: Portfolio Polish
- [ ] README.md — portfolio landing page
  - [ ] Project overview + zonal architecture diagram
  - [ ] Photo of assembled platform
  - [ ] Safety lifecycle summary (HARA → safety case)
  - [ ] Links to all work products
  - [ ] Demo video embed
  - [ ] Skills/standards demonstrated
  - [ ] BOM + cost breakdown
- [ ] Merge develop → release/1.0.0 → main
- [ ] Tag v1.0.0

### Files
- `docs/aspice/system-test-report.md`
- `docs/safety/safety-case.md`
- `README.md`
- `media/` — demo video or YouTube link

### DONE Criteria
- [ ] All 12 scenarios demonstrated and recorded
- [ ] Every fault results in correct safe state
- [ ] Safety case references all evidence
- [ ] Traceability matrix 100% complete
- [ ] README is portfolio-ready
- [ ] Tagged v1.0.0 on main

---

## Skill Coverage Matrix

| Skill Area | Where Demonstrated | Resume Keywords |
|------------|-------------------|-----------------|
| Embedded C (MISRA) | Firmware: CVC, FZC, RZC, SC | MISRA C, state machines, defensive programming |
| Automotive Safety MCU | Safety Controller (TMS570) | TMS570, lockstep, HALCoGen, Cortex-R5 |
| Automotive Safety Process | All safety docs | ISO 26262, ASIL D, HARA, FMEA, DFA, safety case |
| Zonal Architecture | System design | E/E architecture, zonal controllers |
| CAN Bus Protocol | CAN matrix, E2E, all firmware | CAN 2.0B, E2E protection, CRC, alive counter |
| Sensor Integration | AS5048A, TFMini-S, ACS723, NTC | SPI, UART, ADC, lidar, hall-effect, ToF |
| Motor Control | RZC firmware | PWM, H-bridge, current monitoring, encoder |
| ASPICE Process | All documentation | ASPICE 4.0, V-model, traceability |
| HIL Testing | Fault injection GUI | HIL, fault injection, python-can, SocketCAN |
| Edge Computing | Raspberry Pi gateway | Edge ML, gateway, python-can |
| Machine Learning | Motor health, anomaly detection | scikit-learn, Isolation Forest, Random Forest, predictive maintenance |
| Cloud IoT | AWS pipeline | AWS IoT Core, MQTT, Timestream, Grafana |
| Automotive Cybersecurity | CAN anomaly detection | ISO/SAE 21434, intrusion detection |

---

## Risk Register

| Risk | Impact | Mitigation |
|------|--------|------------|
| TMS570 toolchain learning curve | 1-2 days lost | Start with LED toggle + CAN loopback. Safety Controller firmware is only ~400 lines. |
| CAN bit timing mismatch | Bus doesn't work | Use online CAN bit timing calculator. Test 2-node first, add nodes one at a time. |
| AURIX scope creep temptation | Timeline blown | AURIX is a stretch goal ONLY. Ship with TMS570 first. |
| AWS free tier exceeded | Unexpected cost | Batch messages to 1/5sec. Budget $3/month worst case. |
| Sensor wiring errors | Debug time | Follow HSI spec exactly. Test each sensor standalone before integration. |
| ML model insufficient training data | Weak demo | Generate synthetic data from fault injection. 30 min of CAN logging = thousands of samples. |
