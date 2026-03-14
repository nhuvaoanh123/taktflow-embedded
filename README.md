# Taktflow Embedded — ISO 26262 ASIL D Zonal Vehicle Platform

A solo portfolio project built from scratch to demonstrate end-to-end automotive embedded systems engineering: hardware, firmware, functional safety, testing, and cloud connectivity in one repo.

**Live SIL demo**: [sil.taktflow-systems.com](https://sil.taktflow-systems.com) — all 7 ECUs running in Docker with real-time telemetry, fault injection, and a dashboard you can interact with now.

---

## What this is

A drive-by-wire zonal vehicle platform — the kind of E/E architecture used in modern EVs, built on a budget from dev boards and off-the-shelf components.

**4 physical ECUs** on real STM32 and TMS570 hardware, connected by CAN bus with real sensors and actuators. **3 simulated ECUs** running the same C codebase on Linux/Docker. A Raspberry Pi acts as the edge gateway, running a closed-loop physics plant simulator, cloud telemetry to AWS IoT, and ML-based anomaly detection.

The whole system is developed following ISO 26262 ASIL D — not because it needs to be, but because learning the process is the point.

---

## Honest numbers

| What | How many |
|------|----------|
| ECUs | 7 (4 physical, 3 Docker) |
| Unit tests | 1,075 — 0 failures |
| C source files | 167 (firmware, excluding test files) |
| BSW modules | 25 (MCAL ×7, ECUAL ×5, Services ×12, RTE ×1) |
| Safety requirements | 482 (SG, FSR, TSR, SSR, HSR, SWR across all levels) |
| Traceability links | 2,736 bidirectional (requirement → code → test) |
| HIL test scenarios | 26 — all passed on physical bench |
| CAN messages | 34, 19 with E2E protection (CRC-8 + alive counter) |
| MISRA deviations | 2 documented (Rule 11.5, 11.8 in Com.c/CanIf.c/Dcm.c) |
| Hardware budget | ~$577 |
| ASPICE documents | 67 draft artifacts — not externally audited |

---

## Hardware

Development boards, not automotive-grade silicon. The safety confidence comes from architecture, monitoring, and process — not from certified hardware.

| Board | Role | ASIL |
|-------|------|------|
| 3x STM32G474RE Nucleo-64 | CVC (central), FZC (front), RZC (rear) | D (SW architecture) |
| TI TMS570LC43x LaunchPad | Safety Controller — dual Cortex-R5F lockstep | D (HW lockstep) |

Sensors: AS5048A SPI angle encoder, ACS723 Hall current, NTC thermistors, TFMini-S LiDAR, quadrature encoder.
Actuators: BTS7960 H-bridge (43A motor driver), MG996R servos, kill relay.
CAN: TJA1051T/3 transceivers, 120Ω termination, TVS protection, USB-CAN adapters for Pi/PC.

---

## Software architecture

```
Application SWCs  (Swc_Pedal, Swc_Motor, Swc_Steering, Swc_Brake, ...)
       ↕ Rte_Read / Rte_Write
RTE   (signal buffers, port connections, runnable scheduling)
       ↕
Services  (Com, Dcm, Dem, WdgM, BswM, E2E, CanTp)
       ↕
ECU Abstraction  (CanIf, PduR, IoHwAb)
       ↕
MCAL  (Can, Spi, Adc, Pwm, Dio, Gpt)
       ↕
Hardware  (STM32G474RE / TMS570LC43x / POSIX shim for Docker)
```

The same SWC source files compile for STM32, TMS570, and Linux. Platform differences live only in the MCAL layer.

### Safety Controller

~400 lines of flat C — no RTOS, no BSW. Monitors heartbeats from the three zone ECUs, cross-checks torque vs. current for plausibility, controls an energize-to-run kill relay. Fed by an external TPS3823 watchdog that only gets its pulse when all checks pass.

### HIL bench

All 4 physical ECUs run on real hardware with no physical peripherals connected — sensor values are injected over CAN by the Raspberry Pi using the IoHwAb override layer. The plant simulator runs on the Pi, computes closed-loop physics at 100 Hz, and feeds virtual sensor frames. All 26 HIL test scenarios have been validated on this bench.

```
Raspberry Pi (plant-sim @ 100 Hz)
        | CAN 0x600 / 0x601 — virtual sensor frames
        v
CAN bus (500 kbps)
        |
        +-- CVC STM32G474RE  --- IoHwAb_Hil.c overrides pedal/encoder reads
        +-- FZC STM32G474RE  --- IoHwAb_Hil.c overrides steering/brake/lidar reads
        +-- RZC STM32G474RE  --- IoHwAb_Hil.c overrides motor current/temp reads
        +-- SC  TMS570LC43x  --- monitors heartbeats, controls kill relay
```

---

## Test results — SIL

18 automated scenarios running on `vcan0` in Docker. All pass in CI. Scenario files in `test/sil/scenarios/`, run by `test/sil/verdict_checker.py`. Each carries `verifies:`, `aspice: SWE.6`, and `iso_reference:` fields.

| ID | Scenario | Requirements |
|----|----------|-------------|
| SIL-001 | Normal system startup — INIT → RUN | TSR-001, SSR-CVC-001 |
| SIL-002 | Pedal ramp — torque tracks pedal input | TSR-003, SSR-CVC-003 |
| SIL-003 | Emergency stop — SAFE_STOP on CVC command | TSR-005, SSR-CVC-007 |
| SIL-004 | CAN busoff recovery — FZC busoff → reconnect | TSR-020, SSR-FZC-016 |
| SIL-005 | Watchdog timeout — CVC WdgM miss → DEM event | TSR-008, SSR-CVC-012 |
| SIL-006 | Battery undervoltage — DEGRADED/LIMP mode | TSR-046, SSR-RZC-001 |
| SIL-007 | Motor overcurrent — DTC 0xE301, cutoff | TSR-007, SSR-RZC-004 |
| SIL-008 | Sensor disagreement — FZC redundant check fail | TSR-012, SSR-FZC-003 |
| SIL-009 | E2E CRC corruption — receiver rejects frame | TSR-022, SSR-CVC-008 |
| SIL-010 | Motor overtemperature — derating on 0x301 | TSR-009, SSR-RZC-006 |
| SIL-011 | Steering sensor failure — FZC SAFE_STOP | TSR-011, SSR-FZC-002 |
| SIL-012 | Multiple simultaneous faults — priority handling | TSR-050, SSR-CVC-015 |
| SIL-013 | Recovery from SAFE_STOP — re-init sequence | TSR-005, SSR-CVC-016 |
| SIL-014 | 10-minute sustained load — no memory leak, no drift | SWR-CVC-018 |
| SIL-015 | Power cycle — NvM persistence across reset | SWR-CVC-020 |
| SIL-016 | Gateway telemetry — MQTT publish to AWS IoT | SWR-GW-001 |
| SIL-017 | ML anomaly detection — Isolation Forest flags fault | SWR-GW-005 |
| SIL-018 | SAP QM handshake — quality message exchange | SWR-GW-008 |

---

## Test results — HIL

26 scenarios on `can0` with physical ECUs. All passed. Scenario files in `test/hil/scenarios/`, run by `test/hil/hil_runner.py` on the Pi. Fault injection via MQTT; E2E tests via direct CAN frame injection.

| Category | Tests | ASIL | Coverage |
|----------|-------|------|----------|
| Closed-loop plant-sim dynamics | 7 | B–D | Motor/steering/brake/battery/lidar/vehicle state |
| Heartbeat & liveness | 5 | QM–C | CVC/FZC/RZC/BCM/ICU periodic message timing |
| Simulated ECU behavior | 6 | QM | BCM lighting, ICU gauges, TCU UDS diagnostics |
| Fault injection via MQTT | 5 | B–D | Overcurrent, steering/brake fault, undervoltage, overtemp |
| E2E protection & CAN integrity | 3 | D | CRC-8, frame rejection, alive counter freeze |

---

## What knowledge this required

Building this required learning or already knowing:

- **Embedded C** — bare-metal STM32 (HAL + custom drivers), TMS570 bare-metal
- **CAN bus** — frame format, timing, bus load analysis, SocketCAN on Linux
- **AUTOSAR concepts** — layered BSW, RTE port model, PDU routing, not the toolchain
- **ISO 26262** — HARA, safety goals, FTTI, ASIL decomposition, requirements flow-down, FMEA, DFA
- **ASPICE** — V-model, work products, traceability, what Level 2 actually means in practice
- **Docker + SocketCAN** — vcan0, bridge networking, multi-container orchestration
- **Python** — plant physics simulation, CI scripts, test runners, CAN tooling
- **AWS IoT Core** — MQTT over TLS, device certificates, Grafana integration
- **ML basics** — Isolation Forest for anomaly detection on time-series sensor data
- **Git + CI/CD** — GitHub Actions, matrix builds, artifact management

Some of this was known before starting. Most of the ISO 26262 and ASPICE depth was learned by doing it wrong first.

---

## What this does not claim

- The hardware is not automotive-grade or certified for road use
- The ISO 26262 documentation has not been through external audit or review
- "ASIL D" describes the process and architecture intent, not a certified product
- The safety case would not satisfy a Tier 1 supplier review as-is
- This is a learning and demonstration project — it shows how the engineering is done, not a finished product

---

## Running it

```bash
# Build all firmware (POSIX/SIL target)
make build

# Run all 1,075 unit tests
make test

# Start the full SIL environment
cd docker && docker compose up

# MISRA C:2012 analysis
make misra
```

Flashing physical hardware requires `arm-none-eabi-gcc`, OpenOCD, and the respective Nucleo/LaunchPad debug adapters. See [`docs/guides/`](docs/guides/) for setup steps.

---

## Structure

```
firmware/
  cvc/, fzc/, rzc/, sc/     — Physical ECU firmware (src, include, cfg, test)
  bcm/, icu/, tcu/           — Simulated ECU firmware (same codebase, POSIX MCAL)
  shared/bsw/                — 25 BSW modules shared across all ECUs
gateway/                     — Raspberry Pi edge gateway
  plant_sim/                 — Closed-loop physics models (motor, brake, steering, battery, LiDAR)
  cloud_connector/           — AWS IoT Core MQTT bridge
  fault_inject/              — REST API for SIL fault injection
  ml_inference/              — Isolation Forest anomaly detection
docker/                      — SIL environment (docker-compose, Caddy reverse proxy)
hardware/                    — BOM, pin mappings, wiring log, schematics
test/sil/, hil/, pil/        — xIL test scenarios and runners
docs/
  safety/                    — ISO 26262 lifecycle artifacts (concept, analysis, requirements)
  aspice/                    — ASPICE 4.0 work products (SYS, SWE, HWE, SUP, MAN)
  plans/                     — Implementation plans (source of truth for what was built and why)
```

---

## Author

**Ngoc An Dao** — Embedded Systems / Functional Safety

Built as a portfolio project. The goal was to go through every layer of automotive embedded development — from HARA to hardware bring-up to cloud telemetry — and have working, testable evidence for each claim.

Questions, corrections, or observations welcome via Issues.
