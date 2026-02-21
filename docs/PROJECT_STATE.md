# Project State — Taktflow Embedded

> Auto-updated before context compression. Read this to restore full context.

**Last updated**: 2026-02-21
**Branch**: `develop`
**Phase**: Phases 0-6 DONE — CVC firmware complete (6 SWCs, 88 tests, ~5,930 LOC), ready for Phase 7 (FZC firmware)

---

## What This Project Is

**Portfolio project** demonstrating ISO 26262 ASIL D automotive functional safety engineering with cloud IoT and edge ML.

**Product**: Zonal Vehicle Platform — 7-ECU zonal architecture (4 physical + 3 simulated) connected via CAN bus, with Raspberry Pi edge gateway, cloud telemetry (AWS), and ML-based anomaly detection. Demonstrates drive-by-wire with full safety lifecycle.

### Architecture: Zonal (Modern E/E) — 7 ECUs

#### 4 Physical ECUs (real hardware, real sensors/actuators)

| Zonal ECU | Role | Hardware | ASIL |
|-----------|------|----------|------|
| Central Vehicle Computer (CVC) | Vehicle brain, pedal input, state machine | STM32G474RE Nucleo | D (SW) |
| Front Zone Controller (FZC) | Steering, braking, lidar, ADAS | STM32G474RE Nucleo | D (SW) |
| Rear Zone Controller (RZC) | Motor, current, temp, battery | STM32G474RE Nucleo | D (SW) |
| Safety Controller (SC) | Independent safety monitor | TI TMS570LC43x LaunchPad | D (HW lockstep) |

#### 3 Simulated ECUs (Docker + SocketCAN, same C codebase)

| ECU | Role | Runtime | ASIL |
|-----|------|---------|------|
| Body Control Module (BCM) | Lights, indicators, door locks | Docker container | QM |
| Instrument Cluster Unit (ICU) | Dashboard gauges, warnings, DTCs | Docker container | QM |
| Telematics Control Unit (TCU) | UDS diagnostics, OBD-II, DTC storage | Docker container | QM |

**Additional**: Raspberry Pi 4 (edge gateway, cloud, ML) + 2× CANable 2.0 (Pi + PC CAN bridge)

**Diverse redundancy**: STM32 (ST) for zone ECUs, TMS570 (TI) for Safety Controller.

---

## Completed Work (Phases 0-4)

### Phase 0: Project Setup & Architecture Docs (DONE)
- Repository scaffold, 28 rule files, hooks, skills, Git Flow
- Hardware feasibility verified for all integration points

### Phase 1: Safety Concept (DONE)
- **Item Definition**: 7 functions, complete interface list, system boundary
- **HARA**: 6 operational situations, 16 hazardous events, ASIL determination
- **Safety Goals**: 8 SGs (SG-001 to SG-008) — 3 ASIL D, 2 ASIL C, 1 ASIL B, 2 ASIL A
- **4 Safe States**: SS-MOTOR-OFF, SS-STEER-CENTER, SS-CONTROLLED-STOP, SS-SYSTEM-SHUTDOWN
- **Functional Safety Concept**: 23 safety mechanisms (SM-001 to SM-023)
- **Functional Safety Requirements**: 25 FSRs (FSR-001 to FSR-025)
- **Safety Plan**: Lifecycle phases, roles, activities, tool qualification

### Phase 2: Safety Analysis (DONE)
- **FMEA**: 50 failure modes across all components (CVC, FZC, RZC, SC, CAN, power)
- **DFA**: 6 common cause failures, 5 cascading failures, beta-factor analysis
- **Hardware Metrics**: SPFM >= 99%, LFM >= 90%, PMHF = 5.2 FIT (all exceed targets)
- **ASIL Decomposition**: 2 decompositions documented with independence arguments

### Phase 3: Requirements & System Architecture (DONE)
- **Stakeholder Requirements**: 32 (STK-001 to STK-032)
- **System Requirements**: 56 (SYS-001 to SYS-056) — 18 ASIL D, 12 C, 1 B, 3 A, 22 QM
- **Technical Safety Requirements**: 51 (TSR-001 to TSR-051) — 26 D, 17 C, 4 B, 4 A
- **SW Safety Requirements**: 81 SSRs across 4 physical ECUs (CVC:23, FZC:24, RZC:17, SC:17)
- **HW Safety Requirements**: 25 HSRs across 4 ECUs (CVC:5, FZC:7, RZC:7, SC:6)
- **System Architecture**: 10 element categories, 24 CAN messages, system state machine, FFI
- **SW Architecture**: Per-ECU decomposition, MPU config, task scheduling, 81 SSR-to-module allocation
- **BSW Architecture**: 16 modules with full C API signatures, 3 platform targets
- **Per-ECU SWRs**: 187 total across 8 documents
  - SWR-CVC: 35 | SWR-FZC: 32 | SWR-RZC: 30 | SWR-SC: 26
  - SWR-BCM: 12 | SWR-ICU: 10 | SWR-TCU: 15 | SWR-BSW: 27
- **Traceability Matrix**: 440 total traced (SG → FSR → TSR → SSR → SWR → module → test)

### Phase 4: CAN Protocol & HSI Design (DONE)
- **CAN Message Matrix**: 31 message types, 16 E2E-protected, 24% worst-case bus load
- **Interface Control Document**: 22 interfaces (CAN, SPI, UART, ADC, PWM, GPIO, I2C, MQTT, SOME/IP, USB-CAN, encoder)
- **HSI Specification**: All 4 physical ECUs with peripheral configs, memory maps, MPU, startup
- **HW Requirements**: 33 (HWR-001 to HWR-033) in 7 categories
- **HW Design**: Per-ECU circuit designs with ASCII schematics, power distribution
- **vECU Architecture**: POSIX MCAL abstraction, Docker containers, CI/CD
- **Pin Mapping**: 53 pins across 4 ECUs with conflict checks and solder bridge mods
- **BOM**: 74 line items, $537/$937 total, 3-phase procurement plan

### Total Requirements Written
| Type | Count |
|------|-------|
| Safety Goals (SG) | 8 |
| Functional Safety Reqs (FSR) | 25 |
| Technical Safety Reqs (TSR) | 51 |
| SW Safety Reqs (SSR) | 81 |
| HW Safety Reqs (HSR) | 25 |
| Stakeholder Reqs (STK) | 32 |
| System Reqs (SYS) | 56 |
| SW Requirements (SWR) | 187 |
| HW Requirements (HWR) | 33 |
| FMEA Failure Modes | 50 |
| **Total** | **~548** |

---

## Phase 5: Shared BSW Layer (DONE)

All 16 AUTOSAR-like BSW modules implemented with TDD (test-first enforcement):

### MCAL (6 modules, 87 tests)
| Module | Tests | Traces | Description |
|--------|-------|--------|-------------|
| Can | 20 | SWR-BSW-001..005 | FDCAN driver, state machine, bus-off recovery |
| Spi | 14 | SWR-BSW-006 | SPI master, AS5048A compatible (CPOL=0, CPHA=1) |
| Adc | 13 | SWR-BSW-007 | ADC group conversion, 12-bit |
| Pwm | 14 | SWR-BSW-008 | Timer PWM, 16-bit duty (0x0000-0x8000) |
| Dio | 12 | SWR-BSW-009 | GPIO atomic access via BSRR |
| Gpt | 14 | SWR-BSW-010 | General purpose timers, microsecond resolution |

### ECUAL (3 modules, 36 tests)
| Module | Tests | Traces | Description |
|--------|-------|--------|-------------|
| CanIf | 9 | SWR-BSW-011..012 | CAN ID ↔ PDU ID routing |
| PduR | 8 | SWR-BSW-013 | PDU routing (Com/Dcm ↔ CanIf) |
| IoHwAb | 19 | SWR-BSW-014 | Sensor/actuator MCAL wrappers |

### Services (6 modules, 58 tests)
| Module | Tests | Traces | Description |
|--------|-------|--------|-------------|
| Com | 9 | SWR-BSW-015..016 | Signal pack/unpack, TX/RX PDU management |
| Dcm | 14 | SWR-BSW-017 | UDS 0x10/0x22/0x3E, session management, NRCs |
| Dem | 8 | SWR-BSW-018..020 | DTC storage, counter-based debouncing |
| WdgM | 8 | SWR-BSW-021 | Alive supervision, watchdog gating |
| BswM | 14 | SWR-BSW-022 | Mode manager (STARTUP→RUN→DEGRADED→SAFE_STOP→SHUTDOWN) |
| E2E | 23 | SWR-BSW-023..025 | CRC-8 SAE J1850, alive counter, Data ID |

### RTE (1 module, 14 tests)
| Module | Tests | Traces | Description |
|--------|-------|--------|-------------|
| Rte | 14 | SWR-BSW-026..027 | Signal buffering, priority-ordered runnable dispatch |

**Total: 16 modules, 195 unit tests, ~5,000 LOC**

Hardware abstraction pattern: `Module_Hw_*` extern functions (mocked in tests, real HAL on target).

---

## Phase 6: Central Vehicle Computer (CVC) Firmware (DONE)

23 files, ~5,930 LOC, 88 unit tests. CVC application SWCs built on shared BSW stack.

### SWCs (6 modules, 88 tests)
| Module | Tests | LOC | ASIL | Description |
|--------|-------|-----|------|-------------|
| Swc_Pedal | 25 | 482 | D | Dual AS5048A, plausibility, stuck detect, torque map, ramp/mode limit |
| Swc_VehicleState | 20 | 356 | D | 6-state × 11-event transition table, BswM integration |
| Swc_EStop | 10 | 158 | D | Debounce, permanent latch, 4× CAN broadcast, fail-safe |
| Swc_Heartbeat | 15 | 216 | D | 50ms TX, alive counter, FZC/RZC timeout (3 misses), recovery |
| Swc_Dashboard | 8 | 314 | QM | OLED state/speed/pedal/faults, 200ms refresh, fault resilience |
| Ssd1306 | 10 | 338 | QM | I2C OLED driver, 5×7 font (95 ASCII), init/clear/cursor/string |

### Configuration
| File | LOC | Description |
|------|-----|-------------|
| Cvc_Cfg.h | 185 | 31 RTE signals, 14 Com PDUs, 18 DTCs, 4 E2E IDs, enums |
| Rte_Cfg_Cvc.c | 97 | Signal table + 8 runnables with priorities |
| Com_Cfg_Cvc.c | 118 | 17 signals, 8 TX + 6 RX PDUs with timeouts |
| Dcm_Cfg_Cvc.c | 122 | 4 DIDs (F190/F191/F195/F010) with callbacks |
| main.c | 338 | BSW init, self-test, 1ms/10ms/100ms tick loop |

### Key Design
- **Const transition table** for state machine — no branching, deterministic
- **Zero-torque latch** on pedal fault — 50 fault-free cycles to clear
- **E-stop permanent latch** — never clears once activated
- **Heartbeat guard** — INIT → RUN only after both FZC + RZC heartbeats confirmed
- **Display fault isolation** — QM OLED fault doesn't affect ASIL D vehicle operation

---

## Next Phase: Phase 7 — Front Zone Controller (FZC) Firmware

The next phase implements FZC application SWCs:
- Swc_Steering: servo control, angle feedback, rate limiting, return-to-center
- Swc_Brake: servo control, brake force mapping, auto-brake on timeout
- Swc_Lidar: TFMini-S parser, distance filtering, emergency thresholds
- Swc_FzcSafety: local plausibility, sensor timeout, DTC reporting
- Swc_Heartbeat: FZC heartbeat TX
- UART MCAL for TFMini-S (FZC-specific)
- RTE/Com configuration, main.c

---

## Repository Structure

```
taktflow-embedded/
├── .claude/
│   ├── settings.json, hooks/, rules/ (28 files), skills/ (3)
├── firmware/                                  — Per-ECU firmware (7 ECUs + shared BSW)
│   ├── cvc/src/, include/, cfg/, test/       — Central Vehicle Computer (STM32, ASIL D)
│   ├── fzc/src/, include/, cfg/, test/       — Front Zone Controller (STM32, ASIL D)
│   ├── rzc/src/, include/, cfg/, test/       — Rear Zone Controller (STM32, ASIL D)
│   ├── sc/src/, include/, test/              — Safety Controller (TMS570, ASIL D, NO AUTOSAR)
│   ├── bcm/src/, include/, cfg/, test/       — Body Control Module (Docker, QM)
│   ├── icu/src/, include/, cfg/, test/       — Instrument Cluster Unit (Docker, QM)
│   ├── tcu/src/, include/, cfg/, test/       — Telematics Control Unit (Docker, QM)
│   └── shared/bsw/                           — AUTOSAR-like BSW
│       ├── mcal/                             — CAN, SPI, ADC, PWM, Dio, Gpt
│       ├── ecual/                            — CanIf, PduR, IoHwAb
│       ├── services/                         — Com, Dcm, Dem, WdgM, BswM, E2E
│       ├── rte/                              — Runtime Environment
│       └── include/                          — Platform_Types, Std_Types
├── docker/                                   — Dockerfile, docker-compose for simulated ECUs
├── gateway/                                  — Raspberry Pi edge gateway (Python)
│   ├── sap_qm_mock/                         — SAP QM mock API
│   ├── tests/, models/                       — Gateway tests, ML models
├── hardware/                                 — Pin mappings, BOM, schematics
├── scripts/                                  — trace-gen.py, baseline-tag.sh
├── test/mil/, sil/, pil/                     — xIL testing
├── docs/
│   ├── INDEX.md                              — Master document registry (entry point)
│   ├── plans/master-plan.md                  — Master plan (source of truth)
│   ├── safety/                               — ISO 26262 (concept, plan, analysis, requirements, validation)
│   ├── aspice/                               — ASPICE deliverables (point of truth)
│   │   ├── plans/                            — Execution plans by process area
│   │   ├── system/                           — SYS.1-3 deliverables
│   │   ├── software/                         — SWE.1-2 deliverables
│   │   ├── hardware-eng/                     — HWE.1-3 deliverables
│   │   ├── verification/                     — SWE.4-6, SYS.4-5 deliverables + xIL reports
│   │   ├── quality/                          — SUP.1 QA plan
│   │   ├── cm/                               — SUP.8 CM strategy, baselines, change requests
│   │   └── traceability/                     — Traceability matrix
│   └── reference/                            — Process playbook, lessons learned
├── CLAUDE.md
└── .gitignore
```

### Document Status

| Area | Files | Status |
|------|-------|--------|
| Safety (ISO 26262) | 15 | Draft (filled) |
| ASPICE deliverables | 25 | Draft (filled) |
| ASPICE execution plans | 8 | Active |
| Hardware docs | 3 | Draft (filled) |
| Verification docs | 14 | Planned stubs |
| Reference/templates | 4 | Active |
| **Total documentation files** | **~70** | |

---

## Hardware Feasibility (Verified)

All integration points researched and confirmed feasible:
- STM32G474RE: enough pins for all 3 zone roles, <1% CPU load at 170MHz
- TMS570LC43x: CAN silent mode works, ~400 lines firmware, DCAN1 via edge connector
- CAN bus: FDCAN (classic mode) + DCAN coexist at 500kbps, proven configuration
- TFMini-S lidar: UART 3.3V logic, 5V power, official STM32 example exists
- AS5048A: SPI dual sensors on same bus with separate CS, 14-bit resolution
- BTS7960: 3.3V logic compatible, built-in current sense + external ACS723
- CANable 2.0 + Pi: candleLight firmware, gs_usb driver, python-can proven
- Edge ML: scikit-learn inference sub-millisecond on Pi 4
- AWS IoT Core: MQTT over TLS, batch to 1msg/5sec for free tier
- SocketCAN + Docker: proven on Linux, WSL2 with USB passthrough on Windows

---

## Git State

- **Branching**: Git Flow (main → develop → feature/)
- **Remote**: github.com/nhuvaoanh123/taktflow-embedded
- **Current branch**: develop

## User Preferences

- Auto commit and push after work
- Plan before implementing
- Update PROJECT_STATE.md before context compression
- Building as portfolio for automotive FSE job
- Zonal architecture, not domain-based
- 7 ECUs: 4 physical + 3 simulated (Docker)
- Focus on finishability — 3× STM32 + 1× TMS570 + Pi
- ~$2K hardware budget
- Refine every document 3 times (see memory/refinement-process.md)
- Search web freely without asking
