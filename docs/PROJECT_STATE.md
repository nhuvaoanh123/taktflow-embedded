# Project State — Taktflow Embedded

> Auto-updated before context compression. Read this to restore full context.

**Last updated**: 2026-02-21
**Branch**: `develop`
**Phase**: Phase 0 IN PROGRESS — ASPICE-aligned ALM structure complete, master plan paths updated

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

### 16 Demo Scenarios (12 Safety + 3 Simulated ECU + 1 SAP QM)
1. Normal driving → ICU shows speed, BCM lights on
2. Pedal sensor disagreement → limp mode, ICU warning
3. Pedal sensor failure → motor stops, ICU fault
4. Object detected (lidar) → emergency brake, BCM hazards
5. Motor overcurrent → motor stops, DTC stored in TCU
6. Motor overtemp → derating then stop, ICU temp warning
7. Steering sensor fault → return to center, ICU warning
8. CAN bus loss → Safety Controller kills system
9. ECU hang → Safety Controller kills system
10. E-stop → broadcast stop, BCM hazards
11. ML anomaly alert → cloud dashboard alarm
12. CVC vs Safety disagree → Safety wins
13. UDS diagnostic session → TCU responds to requests
14. DTC read/clear cycle → TCU manages fault codes
15. Night driving mode → BCM auto headlights
16. DTC → SAP QM workflow → Q-Meldung created, 8D report auto-generated

### Key Design Decisions
- **7-ECU hybrid**: 4 physical + 3 simulated (Docker), mirrors real Tier 1 vECU development
- **AUTOSAR Classic BSW**: Layered architecture (MCAL, CanIf, PduR, Com, Dcm, Dem, WdgM, BswM, RTE) on physical ECUs
- **AUTOSAR Adaptive / SOME/IP**: vsomeip on simulated ECUs, CAN↔SOME/IP bridge on Pi
- **Zonal over domain**: Modern E/E architecture, fewer physical ECUs, more resume impact
- **3× STM32 + 1× TMS570**: One toolchain for most firmware, real ASIL D MCU for safety
- **TMS570 for Safety Controller**: Simple firmware (~400 LOC), lockstep cores, TUV-certified, NO AUTOSAR (intentional — diverse architecture)
- **Simulated ECUs in C**: Same BSW stack compiled for Linux with POSIX SocketCAN MCAL
- **UDS/OBD-II in Dcm/Dem**: Diagnostic stack per AUTOSAR module structure
- **Cloud + ML**: AWS IoT Core + Grafana + scikit-learn (motor health, CAN anomaly detection)
- **CAN 2.0B only**: TMS570 DCAN doesn't support FD. STM32 FDCAN runs in classic mode. All compatible.
- **SAP QM mock integration**: DTC → Q-Meldung pipeline, 8D report auto-generation
- **xIL testing strategy**: MIL (Python plant models), SIL (Docker + vcan), PIL (real MCU + sim plant), HIL (full hardware)
- **~$977 hardware budget** (within $2K target)

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

### Document Counts

| Area | Files | Status |
|------|-------|--------|
| Safety (ISO 26262) | 17 | Planned stubs |
| ASPICE deliverables | 32 | Planned stubs |
| ASPICE execution plans | 8 | Active |
| Firmware READMEs | 32 | Active |
| Infrastructure stubs | 9 | Active |
| Scripts | 2 | Stubs |
| Hardware docs | 2 | Draft |
| **Total new files** | **~100** | |

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
