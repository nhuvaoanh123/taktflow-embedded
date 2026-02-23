---
document_id: PROCUREMENT-REQUIREMENTS-CHRONICLE
title: "Project Requirements and Procurement Chronicle (Merged)"
version: "2.3"
status: draft
date: 2026-02-23
merged_sources:
  - hardware/bom-list.md
  - hardware/bom.md
  - hardware/procurement-validation.md (v1.0)
  - docs/aspice/hardware-eng/hw-requirements.md
  - docs/aspice/system/system-requirements.md
---

## Human-in-the-Loop (HITL) Comment Lock

`HITL` means human-reviewer-owned comment content.

**Marker standard (code-friendly):**
- Markdown: `<!-- HITL-LOCK START:<id> -->` ... `<!-- HITL-LOCK END:<id> -->`
- C/C++/Java/JS/TS: `// HITL-LOCK START:<id>` ... `// HITL-LOCK END:<id>`
- Python/Shell/YAML/TOML: `# HITL-LOCK START:<id>` ... `# HITL-LOCK END:<id>`

**Rules:**
- AI must never edit, reformat, move, or delete text inside any `HITL-LOCK` block.
- Append-only: AI may add new comments/changes only; prior HITL comments stay unchanged.
- If a locked comment needs revision, add a new note outside the lock or ask the human reviewer to unlock it.



# Project Requirements and Procurement Chronicle (Merged)

Single-file merge of requirements baseline + procurement validation, kept in chronological order.

## 1. Chronological Timeline

### 2026-02-21 (Baseline Requirements Published)

1. System requirements baseline created: `56 SYS requirements` with open items SYS-O-001..006 and assumptions SYS-A-001..004.
   - Source: `taktflow-embedded/docs/aspice/system/system-requirements.md`
2. Hardware requirements baseline created: `33 HWR requirements` for power, CAN, sensors, actuators, safety, protection.
   - Source: `taktflow-embedded/docs/aspice/hardware-eng/hw-requirements.md`
3. Procurement BOM list created (core items #1..29).
   - Source: `taktflow-embedded/hardware/bom-list.md`
4. Detailed BOM created (74 entries with alternatives and phases).
   - Source: `taktflow-embedded/hardware/bom.md`

### 2026-02-22 (Orders Placed / Updated)

1. Safety MCU launchpad `LAUNCHXL2-570LC43` x1: under review.
2. USB-CAN adapters ordered: Ecktron UCAN x1, Waveshare USB-CAN x1.
3. SN65HVD230 board ordered x1.
4. TJA1051 modules ordered x2.
5. IRLZ44N pack ordered.
6. 1N4007 pack ordered.
7. E-stop buttons ordered (2 variants, spare included).
8. 12V/30A relay ordered.
9. BTS7960 ordered x1.
10. TFMini-S ordered x1.
11. OLED SSD1306 ordered x1.
12. LED + resistor kit ordered.
13. NTC set (5K + 10K) ordered.
14. JST-XH connector kit ordered.
15. AS5048A modules ordered (user indicated qty 3).
16. SparkFun ACS723 order canceled.
17. MG996R servo pack order canceled.
18. Raspberry Pi 4 order conflict appeared (one arrival entry vs one canceled order).
19. NUCLEO-G474RE x3 ordered (Conrad and another order summary; likely duplicate reporting).
20. Additional order confirms: Pi PSU 5.1V/3A, 12V gear motor, lab PSU, DC/DC down converters, perfboards, standoffs.
21. TSR 2-2433N (24V->3.3V, 2A) x3 ordered as extra regulator parts.

### 2026-02-23 (Consolidation and Validation)

1. All order data consolidated and cross-checked against BOM and requirement docs.
2. Open questions resolved where evidence allows; unresolved items tracked with clear action.
3. User confirmed procurement closure for: Raspberry Pi 4, MicroSD, TJA1051 (full qty), ACS723 current sensor, servos, TPS3823 watchdog ICs, CAN terminations, diametric magnets, and NTC 10k coverage.
4. User confirmed Safety MCU LaunchPad `LAUNCHXL2-570LC43` is on the way and accepted for project fit.
5. User confirmed oscilloscope purchased (optional BOM item) and confirmed 5V rail availability.

## 2. Requirement Check (Project-Level)

### 2.1 System Requirement Open Items (SYS-O)

| ID | Requirement Open Item | Status | Resolution / Action |
|---|---|---|---|
| SYS-O-001 | Derive TSR from SYS | Open (process) | Already has TSR docs in repo; keep as process closure task for formal trace signoff. |
| SYS-O-002 | Derive SWR per ECU | Open (process) | SWR files exist for ECUs; keep as process closure task for completeness review. |
| SYS-O-003 | CAN bus scheduling/utilization analysis | Open | Needs formal analysis artifact update during SYS.3. |
| SYS-O-004 | Complete CAN message matrix | Open | Message matrix exists, but consistency pass still pending. |
| SYS-O-005 | Characterize calibratable thresholds on target HW | Open | Can only close after full bench bring-up hardware is complete. |
| SYS-O-006 | Complete ASIL classification review | Open | Formal safety review task; not a procurement blocker. |

### 2.2 Hardware Requirement Procurement-Critical Check (HWR)

| HWR | Requirement Summary | Procurement Status |
|---|---|---|
| HWR-006 | 12V main power rail via bench PSU | Covered (lab PSU ordered). |
| HWR-007 | Kill relay-gated actuator rail | Covered (relay + MOSFET ordered), integration pending. |
| HWR-008 | 5V regulated rail | Covered (user confirmed 5V rail available). |
| HWR-009 | 3.3V regulated rail | Covered (DC/DC + TSR 3.3V modules available). |
| HWR-014 | CAN topology + 120R terminations | Covered (terminations confirmed done). |
| HWR-016 | STM32 nodes need TJA1051 x3 | Covered (full qty confirmed done). |
| HWR-017 | SC node SN65HVD230 x1 | Covered. |
| HWR-021 | ACS723 current sensor | Covered (confirmed done). |
| HWR-022 | NTC 10k B3950 | Covered (confirmed done). |
| HWR-024 | Two servos (steering + brake) | Covered (confirmed done). |
| HWR-027 | TPS3823 watchdog x4 | Covered (confirmed done). |
| HWR-028 | Kill relay assembly hardware | Covered (relay, IRLZ44N, 1N4007 ordered). |
| HWR-029 | NC E-stop button | Covered (2 ordered). |
| HWR-031 | Fuses and holders | Partial (needs as-built confirmation). |

## 3. BOM Validation Snapshot (Core #1-29)

| BOM # | Requirement | Required | Current | Result |
|---:|---|---:|---:|---|
| 1 | NUCLEO-G474RE | 3 | 3 ordered | Matched |
| 2 | LAUNCHXL2-570LC43 | 1 | 1 confirmed (on the way) | Matched |
| 3 | Raspberry Pi 4 | 1 | confirmed done | Matched |
| 4 | TJA1051 modules | 3 | 3 | Matched |
| 5 | SN65HVD230 module | 1 | 1 | Matched |
| 6 | USB-CAN (Pi) | 1 | 1 substitute | Matched by substitute |
| 7 | USB-CAN (PC) | 1 | 1 substitute | Matched by substitute |
| 8 | AS5048A + diametric magnets | 3 sets | sensors ordered; diametric magnets confirmed | Matched |
| 9 | TFMini-S | 1 | 1 | Matched |
| 10 | ACS723 | 1 | confirmed done | Matched |
| 11 | NTC 10k B3950 | 3 | confirmed done | Matched |
| 12 | 12V motor | 1 | 1 (GM27 gear motor substitute) | Matched by substitute |
| 13 | BTS7960 | 1 | 1 | Matched |
| 14 | Steering servo | 1 | confirmed done | Matched |
| 15 | Brake servo | 1 | confirmed done | Matched |
| 16 | TPS3823 watchdog | 4 | confirmed done | Matched |
| 17 | 12V 30A relay | 1 | 1 | Matched |
| 18 | IRLZ44N | 1 | pack ordered | Matched |
| 19 | 1N4007 | 1 | pack ordered | Matched |
| 20 | NC E-stop | 1 | 2 | Matched (+spare) |
| 21 | SSD1306 OLED | 1 | 1 | Matched |
| 22 | LEDs + resistors | set | set ordered | Matched |
| 23 | Active buzzer | 1 | pack ordered | Matched |
| 24 | Bench PSU >=12V/5A | 1 | lab PSU 0-30V/0-5A | Matched by better substitute |
| 25 | Buck converters | 3 | 3 + additional 3.3V modules | Partial (confirm 5V and 6V rails) |
| 26 | Pi PSU + MicroSD | set | confirmed done | Matched |
| 27 | CAN cable/wiring/connectors | set | partial | Partial |
| 28 | Mounting/protoboard/standoffs | set | partial | Partial |
| 29 | 120 ohm terminators | 2 | confirmed done | Matched |

## 4. Open Questions Addressed (Chronological Resolution Log)

### Q-001 (2026-02-22): Which safety launchpad part number is canonical?

- Evidence: `bom-list.md` uses `LAUNCHXL2-570LC43`; `bom.md` uses `LAUNCHXL2-TMS57012`.
- Resolution: Treat `LAUNCHXL2-570LC43` as active purchase target because that is what was ordered and validated in this procurement cycle.
- Status: Closed for procurement tracking; document consistency update still needed.

### Q-002 (2026-02-22): Are USB-CAN substitutes acceptable vs CANable 2.0?

- Evidence: BOM asks CANable 2.0, alternatives allow non-CANable USB-CAN options.
- Resolution: Accept as `Matched by substitute` if they support required SocketCAN workflow and stable CAN 500 kbps operation.
- Status: Closed for purchasing; open for bring-up verification.

### Q-003 (2026-02-22): Is Raspberry Pi 4 procured?

- Evidence conflict: one entry shows arrival, later explicit order cancellation shown.
- Resolution: user confirmed Raspberry Pi 4 is done; item marked matched.
- Status: Closed.

### Q-004 (2026-02-22): Is power architecture fully covered?

- Evidence: generic DC/DC modules + TSR 3.3V modules ordered.
- Resolution: 3.3V and 5V are covered; 6V rail remains `must-confirm` against HWR-024.
- Status: Partially closed.

### Q-005 (2026-02-22): Are AS5048A sensor sets complete?

- Evidence: AS5048A modules ordered, magnets ordered but diametric type not confirmed.
- Resolution: diametric magnet type confirmed (6 x 2.5 mm diametric).
- Status: Closed.

### Q-006 (2026-02-22): Is thermal sensing requirement met?

- Evidence: mixed 5k + 10k pack purchased.
- Resolution: user confirmed NTC requirement is done.
- Status: Closed.

### Q-007 (2026-02-23): Missing safety-critical components?

- Resolution: user confirmed closure for `TPS3823 x4`, `servos x2`, `ACS723`, `120R terminations`, `MicroSD`, and `Raspberry Pi 4`.
- Status: Closed.

## 5. Final Action List (Ordered by Bring-Up Criticality)

1. Confirm explicit 6V rail implementation for servo power path in assembled hardware.
2. Complete remaining infrastructure completeness check (wiring/connectors/mounting/fuses as-built).

## 6. Optional Equipment Status

| Item | Status | Note |
|---|---|---|
| Oscilloscope (BOM optional #30 / #74) | Purchased | Use for CAN/PWM/relay/watchdog timing evidence during bring-up. |

## 7. Notes

- This file supersedes fragmented procurement snapshots by combining requirements + procurement state in one chronicle.
- Keep future updates append-only under the timeline section with exact date.

## 8. References

- `taktflow-embedded/docs/aspice/system/system-requirements.md`
- `taktflow-embedded/docs/aspice/hardware-eng/hw-requirements.md`
- `taktflow-embedded/hardware/bom-list.md`
- `taktflow-embedded/hardware/bom.md`



