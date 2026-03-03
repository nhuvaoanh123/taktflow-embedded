# Lessons Learned — Consolidated Index

All lessons-learned documents live in this folder. One file per topic.

## Naming Convention

| Prefix | Scope | Example |
|--------|-------|---------|
| `PROCESS-` | Workflow, tooling, process decisions | `PROCESS-security-hardening.md` |
| `SYS-NNN-` | System requirement HITL review | `SYS-001-dual-pedal-sensing.md` |

## Index

### Safety

| File | Topic | Date | Status |
|------|-------|------|--------|
| [PROCESS-safety-case-development.md](safety/PROCESS-safety-case-development.md) | Safety case — HARA, FMEA, DFA, FSR→TSR flow-down, ASIL decomposition | 2026-02-27 | Closed |
| [PROCESS-hil-gap-analysis.md](safety/PROCESS-hil-gap-analysis.md) | HIL gap analysis, fault injection safety, preemptive protection | 2026-03-01 | Closed |
| [SYS-001-dual-pedal-sensing.md](safety/SYS-001-dual-pedal-sensing.md) | ASIL D pedal sensing, shared SPI bus CCF, 1oo2D architecture | 2026-02-27 | Closed |

### Testing

| File | Topic | Date | Status |
|------|-------|------|--------|
| [PROCESS-ci-test-hardening.md](testing/PROCESS-ci-test-hardening.md) | CI test hardening — LP64/ILP32, source inclusion, 99 failures in 4 rounds | 2026-02-28 | Closed |
| [PROCESS-bsw-tdd-development.md](testing/PROCESS-bsw-tdd-development.md) | BSW TDD — 16 modules, test-first hook, AUTOSAR layer ordering | 2026-02-22 | Closed |
| [PROCESS-sil-nightly-ci.md](testing/PROCESS-sil-nightly-ci.md) | SIL nightly CI — integration tests, COM bridge, verdict checker | 2026-03-01 | Closed |

### Infrastructure

| File | Topic | Date | Status |
|------|-------|------|--------|
| [PROCESS-misra-pipeline.md](infrastructure/PROCESS-misra-pipeline.md) | MISRA C:2012 pipeline — 1,536→0 violations, cppcheck CI gotchas | 2026-02-24 | Closed |
| [PROCESS-posix-vcan-porting.md](infrastructure/PROCESS-posix-vcan-porting.md) | POSIX/vCAN porting — HAL abstraction, SocketCAN, Docker per ECU | 2026-02-23 | Closed |
| [PROCESS-sil-demo-integration.md](infrastructure/PROCESS-sil-demo-integration.md) | SIL demo integration — Docker CAN, heartbeat wrap, plant sim tuning | 2026-03-01 | Closed |
| [PROCESS-fault-injection-demo.md](infrastructure/PROCESS-fault-injection-demo.md) | Fault injection & demo — deterministic faults, DTC, SAP QM, ML anomaly | 2026-03-01 | Closed |
| [PROCESS-rzc-heartbeat-overtransmit.md](infrastructure/PROCESS-rzc-heartbeat-overtransmit.md) | RZC heartbeat 5x over-transmit — Com_SendSignal signal ID vs PDU ID, timing bugs | 2026-03-01 | Closed |
| [PROCESS-simulated-relay-sil.md](infrastructure/PROCESS-simulated-relay-sil.md) | Simulated relay — SIL CAN broadcast, BSW routing tax, POSIX test guards, Docker as fault injection | 2026-03-02 | Closed |

### Process

| File | Topic | Date | Status |
|------|-------|------|--------|
| [PROCESS-hitl-review-methodology.md](process/PROCESS-hitl-review-methodology.md) | HITL review methodology — 443 comments, structured review, improvement cycle | 2026-03-01 | Closed |
| [PROCESS-cross-document-consistency.md](process/PROCESS-cross-document-consistency.md) | Cross-document consistency — CAN IDs, bit timing, sensor specs | 2026-02-26 | Closed |
| [PROCESS-traceability-automation.md](process/PROCESS-traceability-automation.md) | Traceability automation — trace-gen.py, CI enforcement, suspect links | 2026-02-25 | Closed |
| [PROCESS-architecture-decisions.md](process/PROCESS-architecture-decisions.md) | Architecture decisions — ADR framework, hybrid ECUs, file-based ALM | 2026-02-28 | Closed |
| [PROCESS-claude-rules-consolidation.md](process/PROCESS-claude-rules-consolidation.md) | Claude rules consolidation — 30→26 files, 3374→1999 lines, context window optimization | 2026-03-01 | Closed |

### Hardware

| File | Topic | Date | Status |
|------|-------|------|--------|
| [PROCESS-hardware-procurement.md](hardware/PROCESS-hardware-procurement.md) | Hardware procurement — BOM management, datasheet verification, budget | 2026-03-01 | Closed |
| [PROCESS-stm32-cubemx-bringup.md](hardware/PROCESS-stm32-cubemx-bringup.md) | STM32 CubeMX bringup — board selection, HSE 24 MHz, FDCAN bit timing, clock tree | 2026-03-03 | Open |

### Security

| File | Topic | Date | Status |
|------|-------|------|--------|
| [PROCESS-security-hardening.md](security/PROCESS-security-hardening.md) | 10-phase security hardening (web project, carried forward) | 2026-02-20 | Closed |

## Rules

- Every process topic or system requirement that undergoes HITL review MUST get a lessons-learned file here
- One file per topic — never combine unrelated lessons
- SYS-NNN files follow the standard template (see [ASPICE lessons-learned framework](../../aspice/system/lessons-learned/README.md))
- PROCESS files are free-form but must include: date, scope, key decisions, and key takeaways
