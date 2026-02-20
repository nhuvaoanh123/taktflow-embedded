# Master Plan: Zonal Vehicle Platform — ASIL D Portfolio

**Status**: IN PROGRESS
**Created**: 2026-02-20
**Updated**: 2026-02-20
**Target**: 17 working days
**Goal**: Hire-ready automotive functional safety + cloud + ML portfolio

---

## Architecture Overview

### Zonal Controller Architecture (Modern E/E) — 7 ECUs (4 Physical + 3 Simulated)

Unlike traditional domain-based architectures (one ECU per function), this project uses a **zonal architecture** — the modern approach adopted by Tesla, VW, BMW. ECUs are organized by physical vehicle zone with a central computer on top.

The architecture uses **7 ECU nodes**: 4 run on real hardware with real sensors and actuators, and 3 run as software-simulated ECUs (Docker containers or native processes using virtual CAN). This mirrors real automotive development where virtual ECUs (vECUs) are used alongside physical prototypes — a standard practice at Tier 1 suppliers using tools like Vector CANoe and dSPACE VEOS.

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
════════════════════╪═══════════════ CAN Bus (500 kbps) ════════════
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
                    │
                    │ CAN bridge (CANable 2.0 USB-CAN on PC)
                    │
        ┌───────────┼────────────────────────┐
        │           │                        │
  ┌─────┴─────┐ ┌───┴───────┐ ┌─────────────┴─┐
  │   BODY    │ │INSTRUMENT │ │  TELEMATICS    │
  │  CONTROL  │ │  CLUSTER  │ │   CONTROL     │
  │  MODULE   │ │   UNIT    │ │    UNIT       │
  │  (BCM)    │ │  (ICU)    │ │   (TCU)       │
  │           │ │           │ │               │
  │• Lights   │ │• Gauges   │ │• UDS diag     │
  │• Indicators│ │• Warnings │ │• DTC storage  │
  │• Door lock│ │• DTC view │ │• OBD-II PIDs  │
  │• Hazards  │ │• Dash UI  │ │• Fault logging│
  └───────────┘ └───────────┘ └───────────────┘
   SIMULATED     SIMULATED      SIMULATED
   (Docker)      (Docker)       (Docker)
```

### ECU Summary Table

| ECU | Role | Type | Hardware / Runtime | ASIL |
|-----|------|------|--------------------|------|
| Central Vehicle Computer (CVC) | Vehicle brain, pedal input, state machine | **Physical** | STM32G474RE Nucleo | D (SW) |
| Front Zone Controller (FZC) | Steering, braking, lidar, ADAS | **Physical** | STM32G474RE Nucleo | D (SW) |
| Rear Zone Controller (RZC) | Motor, current, temp, battery | **Physical** | STM32G474RE Nucleo | D (SW) |
| Safety Controller (SC) | Independent safety monitor | **Physical** | TI TMS570LC43x LaunchPad | D (HW lockstep) |
| Body Control Module (BCM) | Lights, indicators, door locks | **Simulated** | Docker + vcan / CAN bridge | QM |
| Instrument Cluster Unit (ICU) | Dashboard gauges, warnings, DTCs | **Simulated** | Docker + vcan / CAN bridge | QM |
| Telematics Control Unit (TCU) | UDS diagnostics, OBD-II, DTC storage | **Simulated** | Docker + vcan / CAN bridge | QM |

**Diverse redundancy**: CVC/FZC/RZC use STM32 (ST). Safety Controller uses TMS570 (TI). Different vendor, different architecture = real ISO 26262 diverse redundancy.

### Simulated ECU Details

All 3 simulated ECUs are written in **C** (same codebase structure as physical ECUs) compiled for Linux with a POSIX SocketCAN abstraction layer. They run as Docker containers on the development PC, connected to the real CAN bus via a second CANable 2.0 adapter, or to a virtual CAN bus (vcan0) for pure-software testing.

**BCM — Body Control Module (QM)**
- Receives: vehicle state from CVC, speed from RZC, brake status from FZC
- Sends: light status (0x400), indicator state (0x401), door lock status (0x402)
- Logic: auto headlights at speed > 0, hazard lights on emergency brake, turn signals from steering angle
- Adds: realistic comfort-domain CAN traffic, multi-domain network demonstration

**ICU — Instrument Cluster Unit (QM)**
- Receives: ALL CAN messages (listen-only consumer)
- Sends: DTC acknowledgments (0x7FF)
- Output: Terminal UI (ncurses) or web dashboard showing speedometer, tachometer, temp gauge, warning lights
- Adds: visual demo output, proves CAN message decoding across full network

**TCU — Telematics Control Unit (QM)**
- Receives: diagnostic requests (UDS 0x7DF/0x7E0-0x7E6), all DTC broadcasts
- Sends: UDS responses (0x7E8-0x7EE), DTC storage confirmations
- Logic: UDS service handler (0x10 DiagSession, 0x22 ReadDataByID, 0x14 ClearDTCs, 0x19 ReadDTCs)
- Adds: UDS/OBD-II diagnostics stack — massive resume keyword, every OEM cares about this

### CAN Bus Topology

```
Development Modes:

MODE 1: Full Hardware + Simulated (Demo Mode)
  Real CAN Bus ─── CVC ─── FZC ─── RZC ─── SC ─── CANable(Pi) ─── CANable(PC)
                                                                         │
                                                           Docker: BCM, ICU, TCU

MODE 2: Pure Software (CI/CD Mode)
  vcan0 ─── vCVC ─── vFZC ─── vRZC ─── vSC ─── BCM ─── ICU ─── TCU ─── Gateway
  (All 7 ECUs + gateway as processes, no hardware needed)

MODE 3: Partial Hardware (Development Mode)
  Real CAN Bus ─── [1-4 physical ECUs] ─── CANable(PC)
                                              │
                              Docker: remaining ECUs simulated
```

### Additional Systems

| System | Hardware | Purpose |
|--------|----------|---------|
| Edge Gateway | Raspberry Pi 4 (2GB) | Cloud telemetry, ML inference, fault injection GUI |
| CAN Analyzer (Pi) | CANable 2.0 (USB-CAN) | Bus monitoring, SocketCAN interface for Pi |
| CAN Bridge (PC) | CANable 2.0 (USB-CAN) | Bridge real CAN bus to Docker simulated ECUs |
| Cloud | AWS IoT Core + Grafana | Real-time dashboard, data lake, alerts |
| vECU Runtime | Docker + SocketCAN | Run simulated ECUs with same C source code |

### 15 Demo Scenarios (12 Safety + 3 Simulated ECU)

| # | Scenario | Trigger | Observable Result | ECUs Involved |
|---|----------|---------|-------------------|---------------|
| 1 | Normal driving | Pedal input | Motor spins, ICU shows speed, BCM lights on | CVC, RZC, ICU, BCM |
| 2 | Pedal sensor disagreement | Dual sensor mismatch | Limp mode, ICU warning light | CVC, RZC, ICU |
| 3 | Pedal sensor failure | Sensor disconnected | Motor stops, ICU fault display | CVC, RZC, ICU |
| 4 | Object detected | Lidar < threshold | Brake engages, BCM hazard lights | FZC, CVC, RZC, BCM |
| 5 | Motor overcurrent | Current sensor trip | Motor stops, DTC stored in TCU | RZC, CVC, TCU |
| 6 | Motor overtemp | Temp sensor trip | Motor derates then stops, ICU temp warning | RZC, CVC, ICU |
| 7 | Steering fault | Angle sensor lost | Servo returns to center, ICU steering warning | FZC, CVC, ICU |
| 8 | CAN bus loss | Bus disconnected | Safety Controller kills system | SC, all |
| 9 | ECU hang | Missing heartbeat | Safety Controller kills system | SC, all |
| 10 | E-stop pressed | Button pressed | Broadcast stop, everything stops, BCM hazards | CVC, all, BCM |
| 11 | ML anomaly alert | Abnormal current pattern | Cloud dashboard alarm fires | Pi, AWS |
| 12 | CVC vs Safety disagree | Injected conflict | Safety Controller wins (kill relay) | CVC, SC |
| 13 | UDS diagnostic session | TCU receives 0x10 request | TCU responds, reads live data via 0x22 | TCU, all |
| 14 | DTC read/clear cycle | TCU 0x19/0x14 service | TCU lists stored faults, clears on command | TCU |
| 15 | Night driving mode | Speed > 0 in BCM | BCM auto-enables headlights, ICU shows icon | BCM, ICU, RZC |

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
| 6b | CANable 2.0 (USB-CAN, for PC — CAN bridge to simulated ECUs) | 1 | $35 | $35 |
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
| | | | **Total** | **~$977** |
| | | | **Without scope** | **~$577** |

**Budget**: $2,000 — leaves ~$1,023 for shipping, spares, and upgrades (e.g., RPLidar A1 at $100 for 360-degree scanning).

---

## Software Toolchains (All Free)

| MCU | IDE | HAL / Drivers | Debug |
|-----|-----|---------------|-------|
| STM32G474 | STM32CubeIDE | STM32 HAL + CubeMX | Onboard ST-LINK/V3 |
| TMS570LC43x | Code Composer Studio | HALCoGen (TUV-certified process) | Onboard XDS110 |
| Simulated ECUs | VS Code + GCC | POSIX SocketCAN, vsomeip | Docker, GDB |
| Raspberry Pi | VS Code + Python | python-can, paho-mqtt, scikit-learn | SSH |

## Software Architecture — AUTOSAR-like Layered BSW

All physical ECUs (CVC, FZC, RZC) follow an AUTOSAR Classic-inspired layered architecture. This is not a certified AUTOSAR stack — it's an architectural pattern that demonstrates understanding of the BSW structure.

```
┌─────────────────────────────────────────────────────────┐
│  APPLICATION LAYER (SWCs — Software Components)         │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐  │
│  │Swc_Pedal │ │Swc_Steer │ │Swc_Motor │ │Swc_Safety│  │
│  │          │ │          │ │          │ │          │  │
│  │Rte_Read()│ │Rte_Read()│ │Rte_Read()│ │Rte_Read()│  │
│  │Rte_Write│ │Rte_Write│ │Rte_Write│ │Rte_Write│  │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬─────┘  │
├───────┼────────────┼────────────┼────────────┼─────────┤
│  RTE (Runtime Environment)                              │
│  Signal routing: SWC ports ←→ BSW services              │
│  Implemented as: function pointer table + signal buffer │
├─────────────────────────────────────────────────────────┤
│  BSW SERVICES LAYER                                     │
│  ┌─────┐ ┌─────┐ ┌─────┐ ┌──────┐ ┌──────┐ ┌──────┐  │
│  │ Com │ │ Dcm │ │ Dem │ │ WdgM │ │ BswM │ │ NvM  │  │
│  │     │ │     │ │     │ │      │ │      │ │(stub)│  │
│  └──┬──┘ └──┬──┘ └──┬──┘ └──┬───┘ └──┬───┘ └──┬───┘  │
├─────┼───────┼───────┼───────┼────────┼────────┼───────┤
│  ECU ABSTRACTION LAYER                                  │
│  ┌──────┐ ┌──────┐ ┌──────┐                            │
│  │CanIf │ │ PduR │ │IoHwAb│                            │
│  └──┬───┘ └──┬───┘ └──┬───┘                            │
├─────┼────────┼────────┼────────────────────────────────┤
│  MCAL (Microcontroller Abstraction Layer)               │
│  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐    │
│  │ Can │ │ Spi │ │ Adc │ │ Pwm │ │ Gpt │ │ Dio │    │
│  └──┬──┘ └──┬──┘ └──┬──┘ └──┬──┘ └──┬──┘ └──┬──┘    │
├─────┼───────┼───────┼───────┼───────┼───────┼────────┤
│  HARDWARE (STM32G474RE / TMS570LC43x)                  │
└─────────────────────────────────────────────────────────┘
```

### BSW Module Summary

| Module | AUTOSAR Name | What It Does | Complexity |
|--------|-------------|-------------|------------|
| `Can` | CAN Driver (MCAL) | Raw FDCAN peripheral access | Wraps STM32 HAL |
| `Spi` | SPI Driver (MCAL) | Raw SPI peripheral access | Wraps STM32 HAL |
| `Adc` | ADC Driver (MCAL) | Raw ADC DMA scan | Wraps STM32 HAL |
| `Pwm` | PWM Driver (MCAL) | Timer-based PWM output | Wraps STM32 HAL |
| `Gpt` | GPT Driver (MCAL) | System tick, timing | Wraps STM32 HAL |
| `Dio` | DIO Driver (MCAL) | Digital I/O (GPIO) | Wraps STM32 HAL |
| `CanIf` | CAN Interface | HW-independent CAN API, PDU routing up | ~200 LOC |
| `PduR` | PDU Router | Routes PDUs between Com, Dcm, CanIf | ~150 LOC |
| `IoHwAb` | I/O HW Abstraction | Sensor/actuator access for SWCs | ~100 LOC |
| `Com` | Communication | Signal packing/unpacking, I-PDU groups, deadlines | ~400 LOC |
| `Dcm` | Diagnostic Comm Mgr | UDS service dispatch, session management | ~500 LOC |
| `Dem` | Diagnostic Event Mgr | DTC storage, status bits, fault debouncing | ~300 LOC |
| `WdgM` | Watchdog Manager | Supervised entities, alive/deadline monitoring | ~200 LOC |
| `BswM` | BSW Mode Manager | ECU state/mode management, rule-based actions | ~150 LOC |
| `NvM` | NV Memory Manager | Stub — DTC persistence placeholder | ~50 LOC (stub) |
| `Rte` | Runtime Environment | Signal buffer, port connections, runnable scheduling | ~300 LOC |

**Total shared BSW: ~2,500 LOC** — reused across all 3 STM32 ECUs.

### AUTOSAR Adaptive (Simulated ECUs)

The 3 simulated ECUs (BCM, ICU, TCU) use AUTOSAR Adaptive concepts:
- **SOME/IP** via BMW's open-source **vsomeip** library for service-oriented communication
- **ara::com-like** service interfaces between simulated ECUs
- **CAN ↔ SOME/IP gateway** on Raspberry Pi bridges the two worlds

```
Physical ECUs (AUTOSAR Classic)          Simulated ECUs (AUTOSAR Adaptive)
CVC ──┐                                 BCM ──┐
FZC ──┤── CAN 2.0B (500 kbps) ──→ Pi ──┤── SOME/IP (vsomeip)
RZC ──┤                          (bridge)    │
SC  ──┘                                 ICU ──┤
                                        TCU ──┘
```

---

## Phase Table

| Phase | Name | Days | Status |
|-------|------|------|--------|
| 0 | Project Setup & Architecture Docs | 1 | IN PROGRESS |
| 1 | Safety Concept (HARA, Safety Goals, FSC) | 1 | PENDING |
| 2 | Safety Analysis (FMEA, DFA, Hardware Metrics) | 1 | PENDING |
| 3 | Requirements & System Architecture | 1 | PENDING |
| 4 | CAN Protocol & HSI Design | 1 | PENDING |
| 5 | Shared BSW Layer (AUTOSAR-like) | 2 | PENDING |
| 6 | Firmware: Central Vehicle Computer (SWCs) | 1 | PENDING |
| 7 | Firmware: Front Zone Controller (SWCs) | 1 | PENDING |
| 8 | Firmware: Rear Zone Controller (SWCs) | 1 | PENDING |
| 9 | Firmware: Safety Controller (TMS570) | 1 | PENDING |
| 10 | Simulated ECUs: BCM, ICU, TCU (Docker + SOME/IP) | 1.5 | PENDING |
| 11 | Edge Gateway: Pi + CAN + Cloud + ML | 1.5 | PENDING |
| 12 | Unit Tests + Coverage | 1 | PENDING |
| 13 | Hardware Assembly + Integration | 1 | PENDING |
| 14 | Demo Scenarios + Video + Portfolio Polish | 1.5 | PENDING |

---

## Phase 0: Project Setup & Architecture Docs (IN PROGRESS)

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
  - [ ] BCM → ALL: light status, indicator state, door lock status (100 ms cycle)
  - [ ] ICU → CVC: DTC acknowledgment (event-driven)
  - [ ] TCU ↔ ALL: UDS request/response (0x7DF/0x7E0-0x7E6 → 0x7E8-0x7EE, on-demand)
- [ ] E2E protection design (CRC-8, alive counter, data ID per safety message)
- [ ] Hardware-Software Interface per ECU
  - [ ] CVC: pin mapping (SPI1, I2C1, FDCAN1, GPIO)
  - [ ] FZC: pin mapping (TIM2, USART1, SPI2, FDCAN1, GPIO)
  - [ ] RZC: pin mapping (TIM3, TIM4, ADC1, FDCAN1, GPIO)
  - [ ] SC: pin mapping (DCAN1, GIO, RTI)
- [ ] Simulated ECU CAN interface specification
  - [ ] POSIX SocketCAN abstraction layer (replaces STM32 HAL CAN driver)
  - [ ] Docker networking: host network mode for CAN socket access
  - [ ] CAN bridge configuration: CANable on PC ↔ real CAN bus
  - [ ] vcan0 setup for pure-software CI/CD mode
- [ ] Bill of Materials (final, with supplier links)

### Files
- `docs/aspice/can-message-matrix.md`
- `docs/safety/hsi-specification.md`
- `hardware/pin-mapping.md`
- `hardware/bom.md`
- `docs/aspice/vecu-specification.md`

### DONE Criteria
- [ ] Every CAN message defined with ID, signals, timing, E2E protection
- [ ] HSI complete for all 4 physical ECUs
- [ ] Pin assignments verified against Nucleo/LaunchPad schematics
- [ ] Simulated ECU CAN interface specified

---

## Phase 5: Shared BSW Layer (AUTOSAR-like)

Build the reusable BSW modules first — these are shared across all 3 STM32 ECUs.

### 5a: MCAL Drivers (wrapping STM32 HAL)
- [ ] `bsw/mcal/Can.c` — FDCAN1 init, TX, RX (wraps HAL_FDCAN)
- [ ] `bsw/mcal/Spi.c` — SPI init, transfer (wraps HAL_SPI)
- [ ] `bsw/mcal/Adc.c` — ADC DMA scan (wraps HAL_ADC)
- [ ] `bsw/mcal/Pwm.c` — Timer PWM output (wraps HAL_TIM)
- [ ] `bsw/mcal/Dio.c` — Digital I/O (wraps HAL_GPIO)
- [ ] `bsw/mcal/Gpt.c` — System tick, timing (wraps SysTick)
- [ ] Platform abstraction header: `Platform_Types.h` (uint8, uint16, Std_ReturnType)

### 5b: ECU Abstraction Layer
- [ ] `bsw/ecual/CanIf.c` — hardware-independent CAN interface
  - [ ] `CanIf_Transmit(PduId, PduInfo)` — routes PDU down to Can driver
  - [ ] `CanIf_RxIndication()` — callback from Can, routes up to PduR
- [ ] `bsw/ecual/PduR.c` — PDU Router
  - [ ] Routes TX: Com/Dcm → CanIf
  - [ ] Routes RX: CanIf → Com/Dcm (based on PDU ID lookup table)
- [ ] `bsw/ecual/IoHwAb.c` — I/O Hardware Abstraction
  - [ ] `IoHwAb_ReadAnalog(channel)` — wraps ADC read
  - [ ] `IoHwAb_ReadEncoder(channel)` — wraps timer encoder
  - [ ] `IoHwAb_SetPwm(channel, duty)` — wraps PWM set

### 5c: BSW Services
- [ ] `bsw/services/Com.c` — Communication module
  - [ ] Signal-to-PDU packing (little-endian, configurable layout)
  - [ ] PDU-to-signal unpacking
  - [ ] I-PDU transmission deadline monitoring
  - [ ] Rx timeout detection (for safety-critical signals)
  - [ ] E2E integration point (CRC + alive counter wrapping)
- [ ] `bsw/services/Dcm.c` — Diagnostic Communication Manager
  - [ ] UDS service dispatch table
  - [ ] Session management (default, extended, programming)
  - [ ] Response pending (0x78) for long operations
  - [ ] Negative response codes
- [ ] `bsw/services/Dem.c` — Diagnostic Event Manager
  - [ ] DTC storage (in-memory array, configurable size)
  - [ ] DTC status byte management (ISO 14229 status bits)
  - [ ] Fault debouncing (counter-based)
  - [ ] `Dem_ReportErrorStatus()` — called by SWCs to report faults
  - [ ] `Dem_GetDTCStatusAvailabilityMask()`
- [ ] `bsw/services/WdgM.c` — Watchdog Manager
  - [ ] Supervised entity registration
  - [ ] Alive counter monitoring per entity
  - [ ] Deadline monitoring (max execution time)
  - [ ] Only feeds external watchdog (TPS3823) if ALL entities healthy
- [ ] `bsw/services/BswM.c` — BSW Mode Manager
  - [ ] ECU mode: STARTUP → RUN → DEGRADED → SAFE_STOP → SHUTDOWN
  - [ ] Mode-dependent rule actions (e.g., SAFE_STOP → disable Com TX)
- [ ] `bsw/services/E2E.c` — E2E Protection (not standard AUTOSAR module name but fits here)
  - [ ] CRC-8 calculation
  - [ ] Alive counter management
  - [ ] Data ID per message

### 5d: RTE (Runtime Environment)
- [ ] `bsw/rte/Rte.c` — Runtime Environment
  - [ ] Signal buffer (shared memory between SWCs and BSW)
  - [ ] `Rte_Read_<Port>_<Signal>()` — SWC reads a signal
  - [ ] `Rte_Write_<Port>_<Signal>()` — SWC writes a signal
  - [ ] Runnable scheduling table (which SWC runs at which tick rate)
  - [ ] Port connection configuration (compile-time, per ECU)
- [ ] RTE configuration per ECU (CVC, FZC, RZC) — different port mappings

### Files
- `firmware/shared/bsw/mcal/` — Can.c, Spi.c, Adc.c, Pwm.c, Dio.c, Gpt.c
- `firmware/shared/bsw/ecual/` — CanIf.c, PduR.c, IoHwAb.c
- `firmware/shared/bsw/services/` — Com.c, Dcm.c, Dem.c, WdgM.c, BswM.c, E2E.c
- `firmware/shared/bsw/rte/` — Rte.c, Rte_Cfg_Cvc.c, Rte_Cfg_Fzc.c, Rte_Cfg_Rzc.c
- `firmware/shared/bsw/include/` — all headers, Platform_Types.h, Std_Types.h, ComStack_Types.h

### DONE Criteria
- [ ] BSW compiles for STM32 target
- [ ] Can → CanIf → PduR → Com chain sends/receives a CAN message
- [ ] Dcm responds to a basic UDS request (0x10)
- [ ] Dem stores and retrieves a DTC
- [ ] WdgM feeds watchdog only when supervised entities are healthy
- [ ] Rte_Read / Rte_Write pass signals between SWC and Com
- [ ] Same BSW links into CVC, FZC, RZC projects without modification

---

## Phase 6: Firmware — Central Vehicle Computer (STM32G474)

- [ ] Project setup (STM32CubeIDE, CubeMX pin config, link shared BSW)
- [ ] CubeMX: configure FDCAN1, SPI1, I2C1, GPIO (E-stop EXTI, LEDs)
- [ ] Application SWCs (Software Components)
  - [ ] `Swc_Pedal.c` — dual sensor read via Rte_Read(IoHwAb), plausibility check (|S1-S2| < threshold)
  - [ ] `Swc_VehicleState.c` — state machine INIT → RUN → DEGRADED → LIMP → SAFE_STOP
  - [ ] `Swc_Dashboard.c` — OLED display via IoHwAb (vehicle state, speed, fault indicator)
  - [ ] `Swc_EStop.c` — E-stop detection via Rte_Read(Dio), triggers Rte_Write(broadcast stop)
  - [ ] `Swc_Heartbeat.c` — alive counter TX via Com, heartbeat aggregation
- [ ] RTE configuration: `Rte_Cfg_Cvc.c`
  - [ ] Port mapping: pedal SPI → Swc_Pedal, OLED I2C → Swc_Dashboard
  - [ ] Runnable schedule: Swc_Pedal @ 10ms, Swc_VehicleState @ 10ms, Swc_Dashboard @ 100ms
- [ ] `main.c` — BSW init, RTE init, 10ms tick loop, BswM mode management
- [ ] MISRA compliance pass

### Files
- `firmware/cvc/src/` — Swc_Pedal.c, Swc_VehicleState.c, Swc_Dashboard.c, Swc_EStop.c, Swc_Heartbeat.c, main.c
- `firmware/cvc/include/` — corresponding headers
- `firmware/cvc/cfg/` — Rte_Cfg_Cvc.c, Com_Cfg_Cvc.c (signal/PDU mappings for CVC)

### DONE Criteria
- [ ] Dual pedal reading with plausibility check via Rte_Read
- [ ] State machine transitions verified
- [ ] CAN messages TX/RX through full BSW stack (Swc → Rte → Com → PduR → CanIf → Can)
- [ ] OLED displays vehicle state
- [ ] E-stop triggers broadcast stop
- [ ] Dem stores DTC on pedal plausibility failure

---

## Phase 7: Firmware — Front Zone Controller (STM32G474)

- [ ] Project setup (STM32CubeIDE, CubeMX, link shared BSW)
- [ ] CubeMX: configure FDCAN1, TIM2 (PWM), USART1 (DMA), SPI2, GPIO (buzzer)
- [ ] MCAL extensions (FZC-specific)
  - [ ] `Uart.c` — USART1 DMA RX for TFMini-S lidar (not in shared MCAL — only FZC needs UART)
- [ ] Application SWCs
  - [ ] `Swc_Steering.c` — servo control via Rte_Write(Pwm), angle feedback via Rte_Read(Spi), rate limiting, angle limits, return-to-center on fault
  - [ ] `Swc_Brake.c` — servo control via Rte_Write(Pwm), brake force mapping, auto-brake on Com Rx timeout
  - [ ] `Swc_Lidar.c` — TFMini-S frame parser via Rte_Read(Uart), distance filtering, threshold check (warning/brake/emergency)
  - [ ] `Swc_FzcSafety.c` — local plausibility checks, sensor timeout detection, Dem_ReportErrorStatus on failures
  - [ ] `Swc_Heartbeat.c` — alive counter TX via Com
- [ ] RTE configuration: `Rte_Cfg_Fzc.c`
  - [ ] Port mapping: steering SPI → Swc_Steering, lidar UART → Swc_Lidar, servos PWM → Swc_Steering/Brake
  - [ ] Runnable schedule: Swc_Steering @ 10ms, Swc_Brake @ 10ms, Swc_Lidar @ 10ms, Swc_FzcSafety @ 20ms
- [ ] `main.c` — BSW init, RTE init, 10ms tick loop

### Files
- `firmware/fzc/src/` — Swc_Steering.c, Swc_Brake.c, Swc_Lidar.c, Swc_FzcSafety.c, Swc_Heartbeat.c, Uart.c, main.c
- `firmware/fzc/cfg/` — Rte_Cfg_Fzc.c, Com_Cfg_Fzc.c

### DONE Criteria
- [ ] Steering servo tracks angle command through Rte → Pwm
- [ ] Brake servo responds to brake request via Com Rx
- [ ] Lidar reads distance, triggers emergency brake at threshold
- [ ] Return-to-center on steering sensor fault, DTC stored via Dem
- [ ] Auto-brake on Com Rx timeout detection

---

## Phase 8: Firmware — Rear Zone Controller (STM32G474)

- [ ] Project setup (STM32CubeIDE, CubeMX, link shared BSW)
- [ ] CubeMX: configure FDCAN1, TIM3 (PWM H-bridge), TIM4 (encoder), ADC1 (DMA), GPIO
- [ ] Application SWCs
  - [ ] `Swc_Motor.c` — torque request via Rte_Read(Com) → PWM via Rte_Write(Pwm), ramp limiting, direction control
  - [ ] `Swc_CurrentMonitor.c` — overcurrent detection via Rte_Read(Adc), cutoff threshold, filtering, Dem_ReportErrorStatus on trip
  - [ ] `Swc_TempMonitor.c` — derating curve via Rte_Read(Adc), over-temp shutdown, Dem_ReportErrorStatus
  - [ ] `Swc_Battery.c` — voltage monitoring via Rte_Read(Adc), SOC estimate (simple voltage table)
  - [ ] `Swc_RzcSafety.c` — motor safety checks, emergency brake response via Com Rx, safe state transition via BswM
  - [ ] `Swc_Heartbeat.c` — alive counter TX via Com
- [ ] RTE configuration: `Rte_Cfg_Rzc.c`
  - [ ] Port mapping: current ADC → Swc_CurrentMonitor, temp ADC → Swc_TempMonitor, encoder → Swc_Motor
  - [ ] Runnable schedule: Swc_Motor @ 10ms, Swc_CurrentMonitor @ 10ms, Swc_TempMonitor @ 100ms, Swc_Battery @ 1000ms
- [ ] `main.c` — BSW init, RTE init, 10ms tick loop

### Files
- `firmware/rzc/src/` — Swc_Motor.c, Swc_CurrentMonitor.c, Swc_TempMonitor.c, Swc_Battery.c, Swc_RzcSafety.c, Swc_Heartbeat.c, main.c
- `firmware/rzc/cfg/` — Rte_Cfg_Rzc.c, Com_Cfg_Rzc.c

### DONE Criteria
- [ ] Motor responds to torque request via full BSW stack (Com → Rte → Swc_Motor → Rte → Pwm)
- [ ] Overcurrent protection cuts motor within 10 ms, DTC stored via Dem
- [ ] Over-temp derating and shutdown working, DTC stored via Dem
- [ ] Emergency brake command stops motor via Com Rx
- [ ] Battery voltage reported over CAN via Com TX

---

## Phase 9: Firmware — Safety Controller (TMS570LC43x)

**Note**: The Safety Controller does NOT use the AUTOSAR-like BSW stack. It runs a minimal, independent firmware (~400 LOC) generated by HALCoGen. This is intentional — the Safety Controller must be simple, auditable, and architecturally independent from the zone controllers it monitors. Diverse software architecture = real ISO 26262 principle.

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

## Phase 10: Simulated ECUs — BCM, ICU, TCU (Docker + SOME/IP)

### 10a: vECU Runtime Infrastructure
- [ ] POSIX MCAL abstraction (`firmware/shared/bsw/mcal/Can_Posix.c`)
  - [ ] Same `Can_Write()` / `Can_MainFunction_Read()` API as STM32 MCAL
  - [ ] Linux SocketCAN backend (socket, bind, read, write)
  - [ ] Compile-time switch: `#ifdef PLATFORM_POSIX` vs `#ifdef PLATFORM_STM32`
  - [ ] Entire BSW stack (CanIf, PduR, Com, Dcm, Dem) reused unchanged on Linux
- [ ] **vsomeip integration** (BMW open-source SOME/IP)
  - [ ] Install vsomeip library in Docker image
  - [ ] Service interface definitions (BCM offers LightService, ICU offers DisplayService)
  - [ ] SOME/IP-SD (Service Discovery) configuration
  - [ ] CAN ↔ SOME/IP bridge service on Raspberry Pi
- [ ] Dockerfile for vECU base image (Ubuntu, build-essential, can-utils, vsomeip, SocketCAN headers)
- [ ] `docker-compose.yml` for all 3 simulated ECUs + vcan setup
- [ ] Startup script: create vcan0, bring up, launch containers
- [ ] CAN bridge mode: `cangw` or custom bridge for real↔virtual CAN
- [ ] Makefile target: `make vecu` to build all 3 simulated ECUs for Linux

### 10b: Body Control Module (BCM)
- [ ] `firmware/bcm/src/bcm_main.c` — 100 ms main loop
- [ ] `firmware/bcm/src/lights.c` — auto headlight logic (speed > 0 → lights on)
- [ ] `firmware/bcm/src/indicators.c` — turn signals from steering angle, hazard on emergency
- [ ] `firmware/bcm/src/door_lock.c` — lock state management, auto-lock at speed
- [ ] `firmware/bcm/src/can_manager.c` — subscribe to vehicle state, publish light/indicator/lock status
- [ ] CAN messages: 0x400 (light status), 0x401 (indicator state), 0x402 (door lock status)

### 10c: Instrument Cluster Unit (ICU)
- [ ] `firmware/icu/src/icu_main.c` — 50 ms main loop
- [ ] `firmware/icu/src/dashboard.c` — ncurses terminal UI
  - [ ] Speedometer (from RZC encoder data)
  - [ ] Motor temperature gauge
  - [ ] Battery voltage display
  - [ ] Warning lights: overheat, overcurrent, steering fault, CAN fault, e-stop
  - [ ] Active fault list (DTCs received from TCU/ECUs)
- [ ] `firmware/icu/src/can_decoder.c` — decode ALL CAN message IDs, update display model
- [ ] `firmware/icu/src/dtc_display.c` — display active/stored DTCs from TCU

### 10d: Telematics Control Unit (TCU)
- [ ] `firmware/tcu/src/tcu_main.c` — event-driven (UDS request → response)
- [ ] `firmware/tcu/src/uds_server.c` — UDS (ISO 14229) service handler
  - [ ] 0x10 DiagnosticSessionControl (default, extended, programming)
  - [ ] 0x22 ReadDataByIdentifier (live sensor data, SW version, HW version)
  - [ ] 0x14 ClearDiagnosticInformation (clear stored DTCs)
  - [ ] 0x19 ReadDTCInformation (list stored DTCs by status mask)
  - [ ] 0x3E TesterPresent (keep session alive)
  - [ ] 0x7F NegativeResponse (serviceNotSupported, conditionsNotCorrect)
- [ ] `firmware/tcu/src/dtc_store.c` — DTC storage (in-memory, max 64 DTCs)
  - [ ] DTC format: 3-byte DTC number + status byte (ISO 14229 status bits)
  - [ ] Auto-capture DTCs from fault CAN messages (overcurrent, overtemp, sensor fail)
- [ ] `firmware/tcu/src/obd2_pids.c` — OBD-II PID handler (mode 01)
  - [ ] PID 0x0C: engine RPM (from motor encoder)
  - [ ] PID 0x0D: vehicle speed
  - [ ] PID 0x05: coolant temp (from motor temp)
  - [ ] PID 0x04: calculated load (from torque request / max torque)

### Files
- `firmware/bcm/src/`, `firmware/bcm/include/`
- `firmware/icu/src/`, `firmware/icu/include/`
- `firmware/tcu/src/`, `firmware/tcu/include/`
- `firmware/shared/hal_can_posix.c`, `firmware/shared/hal_can_posix.h`
- `docker/Dockerfile.vecu`
- `docker/docker-compose.yml`
- `scripts/vecu-start.sh`

### DONE Criteria
- [ ] All 3 simulated ECUs build and run on Linux
- [ ] BCM sends light/indicator/lock messages on vcan0
- [ ] ICU displays live dashboard from all CAN traffic
- [ ] TCU responds to UDS requests (0x10, 0x22, 0x19, 0x14)
- [ ] Docker-compose brings up all 3 with single command
- [ ] CAN bridge mode connects simulated ECUs to real CAN bus

---

## Phase 11: Edge Gateway — Raspberry Pi + Cloud + ML

### 11a: CAN Interface + Data Logging
- [ ] CANable 2.0 setup (flash candleLight firmware, gs_usb driver)
- [ ] SocketCAN configuration (`ip link set can0 up type can bitrate 500000`)
- [ ] python-can listener (Notifier + Listener pattern)
- [ ] CAN data logger (CSV/Parquet format for ML training)
- [ ] CAN message decoder (parse all message IDs per CAN matrix)

### 11b: Cloud Telemetry
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

### 11c: Edge ML Models
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

### 11d: Fault Injection GUI
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

## Phase 12: Unit Tests + Coverage

- [ ] Test framework setup (Unity for STM32, CCS test for TMS570, pytest for Pi)
- [ ] BSW module unit tests:
  - [ ] `Com.c` — signal packing/unpacking, Rx timeout, deadline monitoring
  - [ ] `CanIf.c` — PDU routing, Tx confirmation, Rx indication
  - [ ] `PduR.c` — routing table lookup, Com vs Dcm dispatch
  - [ ] `Dcm.c` — UDS session management, service dispatch, negative responses
  - [ ] `Dem.c` — DTC storage, status bits, debouncing, overflow
  - [ ] `WdgM.c` — supervised entity alive check, watchdog feed/block
  - [ ] `E2E.c` — CRC calculation, alive counter wrap, data ID
  - [ ] `Rte.c` — signal buffer read/write, port connections
- [ ] Application SWC unit tests:
  - [ ] `Swc_Pedal.c` — plausibility check (agree, disagree, one failed, both failed)
  - [ ] `Swc_Steering.c` — angle limits, rate limiting, return-to-center
  - [ ] `Swc_Brake.c` — auto-brake on timeout, brake force mapping
  - [ ] `Swc_Lidar.c` — distance thresholds, sensor timeout, stuck value
  - [ ] `Swc_Motor.c` — ramp limiting, direction, torque mapping
  - [ ] `Swc_CurrentMonitor.c` — overcurrent detection, threshold, filtering
  - [ ] `Swc_TempMonitor.c` — derating curve, shutdown threshold
  - [ ] `heartbeat.c` (SC) — timeout detection, alive counter validation
- [ ] Simulated ECU tests:
  - [ ] `uds_server.c` — all UDS services, negative responses, session management
  - [ ] `dtc_store.c` — store, read, clear, overflow handling
  - [ ] `obd2_pids.c` — PID response values, unsupported PID handling
  - [ ] `lights.c` — auto headlight logic, hazard trigger conditions
  - [ ] `can_decoder.c` — decode all message IDs, handle unknown IDs
- [ ] Fault injection tests per module
- [ ] Static analysis (cppcheck + MISRA subset)
- [ ] Coverage report (statement, branch, MC/DC for safety-critical)

### Files
- `firmware/cvc/test/`
- `firmware/fzc/test/`
- `firmware/rzc/test/`
- `firmware/sc/test/`
- `firmware/bcm/test/`
- `firmware/icu/test/`
- `firmware/tcu/test/`
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

## Phase 13: Hardware Assembly + Integration

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
- [ ] PC + CANable connected to CAN bus (CAN bridge for simulated ECUs)
- [ ] Flash all physical firmware
- [ ] Launch simulated ECUs via docker-compose
- [ ] Integration test: CAN messages flowing between all 7 ECUs (4 physical + 3 simulated)
- [ ] End-to-end test: pedal → CVC → RZC → motor spins → ICU shows speed
- [ ] Safety chain test: fault → SC detects → kill relay opens → motor stops → BCM hazards
- [ ] Diagnostics test: UDS request from PC → TCU responds with DTCs
- [ ] Cloud test: Pi → AWS → Grafana shows live data from all 7 ECUs

### Files
- `docs/aspice/integration-test-report.md`
- Photos of assembled platform

### DONE Criteria
- [ ] All 7 ECUs (4 physical + 3 simulated) + Pi communicating on CAN bus
- [ ] End-to-end pedal-to-motor working
- [ ] Safety chain (heartbeat → timeout → kill) working
- [ ] Simulated ECUs responding to CAN traffic (BCM lights, ICU dashboard, TCU UDS)
- [ ] Cloud dashboard receiving live data
- [ ] No communication errors in 10-minute run

---

## Phase 14: Demo Scenarios + Video + Portfolio Polish

### 14a: Demo Scenarios
- [ ] Execute and record all 15 demo scenarios
- [ ] Each scenario: setup → trigger → observable result → CAN trace
- [ ] Document results in system test report

### 14b: Video
- [ ] Record each scenario on video (10-15 sec each)
- [ ] Create demo reel (3-5 min combined)
- [ ] Include: platform overview, normal operation, fault scenarios, cloud dashboard, ML alerts

### 14c: Safety Case
- [ ] Safety case document (claims, arguments, evidence)
- [ ] Final traceability verification (no gaps in matrix)

### 14d: Portfolio Polish
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
| **AUTOSAR Classic** | Shared BSW layer (MCAL, CanIf, PduR, Com, Dcm, Dem, WdgM, BswM, RTE) | AUTOSAR, BSW, MCAL, RTE, SWC, Com, Dcm, Dem |
| **AUTOSAR Adaptive** | Simulated ECUs with vsomeip SOME/IP | SOME/IP, vsomeip, service-oriented, ara::com |
| Embedded C (MISRA) | All firmware: CVC, FZC, RZC, SC, BCM, ICU, TCU | MISRA C, state machines, defensive programming |
| Automotive Safety MCU | Safety Controller (TMS570) | TMS570, lockstep, HALCoGen, Cortex-R5 |
| Automotive Safety Process | All safety docs | ISO 26262, ASIL D, HARA, FMEA, DFA, safety case |
| Zonal Architecture | System design (7 ECUs, 4 physical + 3 simulated) | E/E architecture, zonal controllers, SDV |
| CAN Bus Protocol | CAN matrix, E2E, all firmware | CAN 2.0B, E2E protection, CRC, alive counter |
| Sensor Integration | AS5048A, TFMini-S, ACS723, NTC | SPI, UART, ADC, lidar, hall-effect, ToF |
| Motor Control | RZC firmware | PWM, H-bridge, current monitoring, encoder |
| ASPICE Process | All documentation | ASPICE 4.0, V-model, traceability |
| Virtual ECU / SIL | BCM, ICU, TCU (Docker + SocketCAN) | vECU, SIL testing, Docker, SocketCAN, CI/CD |
| UDS Diagnostics | Dcm + Dem in BSW, TCU | UDS (ISO 14229), OBD-II, DTC management, ReadDataByID |
| HIL Testing | Fault injection GUI + CAN bridge | HIL, fault injection, python-can, SocketCAN |
| Edge Computing | Raspberry Pi gateway | Edge ML, gateway, python-can |
| Machine Learning | Motor health, anomaly detection | scikit-learn, Isolation Forest, Random Forest, predictive maintenance |
| Cloud IoT | AWS pipeline | AWS IoT Core, MQTT, Timestream, Grafana |
| Automotive Cybersecurity | CAN anomaly detection | ISO/SAE 21434, intrusion detection |
| Containerization | Simulated ECU runtime | Docker, docker-compose, Linux containers |

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
| CAN bridge timing jitter | Simulated ECUs have delayed responses vs physical | Acceptable for QM-rated simulated ECUs. Document latency in test report. |
| SocketCAN not available on Windows | Dev PC is Windows | Use WSL2 with USB passthrough for CANable, or develop on Raspberry Pi directly. Docker Desktop supports WSL2 backend. |
| UDS implementation scope creep | TCU takes too long | Limit to 5 core services (0x10, 0x22, 0x14, 0x19, 0x3E). No security access (0x27) for portfolio scope. |
| AUTOSAR BSW over-engineering | Spend too long on BSW perfection | Keep modules simplified (~2,500 LOC total). Not a certified stack — demonstrate architecture understanding, not production completeness. |
| vsomeip build complexity | Docker image build issues | Pin vsomeip version, pre-build in base image, fallback to raw SOME/IP over UDP if vsomeip too heavy. |
