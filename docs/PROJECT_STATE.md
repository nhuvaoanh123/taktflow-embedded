# Project State — Taktflow Embedded

> Auto-updated before context compression. Read this to restore full context.

**Last updated**: 2026-02-20
**Branch**: `develop`
**Phase**: Phase 0 DONE — Master plan finalized, starting Phase 1

---

## What This Project Is

**Portfolio project** demonstrating ISO 26262 ASIL D automotive functional safety engineering with cloud IoT and edge ML.

**Product**: Zonal Vehicle Platform — 4-ECU zonal architecture connected via CAN bus, with Raspberry Pi edge gateway, cloud telemetry (AWS), and ML-based anomaly detection. Demonstrates drive-by-wire with full safety lifecycle.

### Architecture: Zonal (Modern E/E)

| Zonal ECU | Absorbs Functions | Hardware | ASIL |
|-----------|-------------------|----------|------|
| Central Vehicle Computer (CVC) | VCU | STM32G474RE Nucleo | D (SW) |
| Front Zone Controller (FZC) | BCU + SCU + ADAS | STM32G474RE Nucleo | D (SW) |
| Rear Zone Controller (RZC) | PCU + BMS | STM32G474RE Nucleo | D (SW) |
| Safety Controller (SC) | Safety MCU | TI TMS570LC43x LaunchPad | D (HW lockstep) |

**Additional**: Raspberry Pi 4 (edge gateway, cloud, ML) + CANable 2.0 (USB-CAN)

**Diverse redundancy**: STM32 (ST) for zone ECUs, TMS570 (TI) for Safety Controller.

### 12 Demo Fault Scenarios
1. Normal driving
2. Pedal sensor disagreement → limp mode
3. Pedal sensor failure → motor stops
4. Object detected (lidar) → emergency brake
5. Motor overcurrent → motor stops
6. Motor overtemp → derating then stop
7. Steering sensor fault → return to center
8. CAN bus loss → Safety Controller kills system
9. ECU hang → Safety Controller kills system
10. E-stop → broadcast stop
11. ML anomaly alert → cloud dashboard alarm
12. CVC vs Safety disagree → Safety wins

### Key Design Decisions
- **Zonal over domain**: Modern E/E architecture, fewer ECUs, more resume impact
- **3× STM32 + 1× TMS570**: One toolchain for most firmware, real ASIL D MCU for safety
- **TMS570 for Safety Controller**: Simple firmware (~400 LOC), lockstep cores, TUV-certified
- **Cloud + ML**: AWS IoT Core + Grafana + scikit-learn (motor health, CAN anomaly detection)
- **CAN 2.0B only**: TMS570 DCAN doesn't support FD. STM32 FDCAN runs in classic mode. All compatible.
- **~$942 hardware budget** (within $2K target)

---

## Repository Structure

```
taktflow-embedded/
├── .claude/
│   ├── settings.json, hooks/, rules/ (28 files), skills/ (3)
├── firmware/
│   ├── cvc/src/, cvc/include/, cvc/test/    — Central Vehicle Computer (STM32)
│   ├── fzc/src/, fzc/include/, fzc/test/    — Front Zone Controller (STM32)
│   ├── rzc/src/, rzc/include/, rzc/test/    — Rear Zone Controller (STM32)
│   ├── sc/src/, sc/include/, sc/test/       — Safety Controller (TMS570)
│   └── shared/                               — Shared CAN protocol, E2E library
├── gateway/                                  — Raspberry Pi edge gateway (Python)
├── hardware/                                 — Pin mappings, BOM, schematics
├── docs/
│   ├── plans/master-plan.md                  — UPDATED with zonal architecture
│   ├── safety/                               — EMPTY, awaiting Phase 1
│   ├── aspice/                               — EMPTY, awaiting Phase 3
│   └── reference/                            — process-playbook.md, lessons-learned.md
├── CLAUDE.md
└── .gitignore
```

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
- Focus on finishability — 3× STM32 + 1× TMS570 + Pi
- ~$2K hardware budget
