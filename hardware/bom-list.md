---
document_id: BOM-LIST
title: "Procurement BOM List"
version: "1.0"
status: draft
date: 2026-02-21
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


# Procurement BOM List

Scope: 7-ECU zonal demo platform (4 physical ECUs + 3 simulated ECUs), Pi gateway, CAN bus, safety chain.

> **BOM Document Relationship**: This document (`bom-list.md`) is a simplified 29-item procurement checklist. The canonical engineering BOM is `bom.md` (74 items with full part numbers, suppliers, and per-category breakdowns). The **BOM Ref** column below maps each procurement line item to the corresponding `bom.md` item number(s) for cross-reference.

## A. Core Required (Build and Demo)

| # | Item | Qty | Preferred Part / Model | Est. Unit (USD) | Est. Total (USD) | BOM Ref |
|---|------|-----|------------------------|------------------|------------------|---------|
| 1 | STM32 Nucleo-64 board | 3 | `NUCLEO-G474RE` | 20 | 60 | #1 |
| 2 | Safety MCU LaunchPad | 1 | `LAUNCHXL2-570LC43` | 62 | 62 | #2 |
| 3 | Raspberry Pi 4 (2GB or 4GB) | 1 | Pi 4 Model B | 45 | 45 | #3 |
| 4 | CAN transceiver module for STM32 ECUs | 3 | TJA1051-based 3.3V breakout | 4 | 12 | #6 |
| 5 | CAN transceiver module for TMS570 | 1 | SN65HVD230 breakout | 5 | 5 | #7 |
| 6 | USB-CAN adapter (Pi capture/injection) | 1 | CANable 2.0 | 35 | 35 | #8 |
| 7 | USB-CAN adapter (PC bridge to simuilated ECUs) | 1 | CANable 2.0 | 35 | 35 | #8 |
| 8 | AS5048A magnetic angle sensor + magnet | 3 | AS5048A breakout + diametric magnet | 15 | 45 | #13, #14 |
| 9 | TFMini-S lidar (UART) | 1 | Benewake TFMini-S | 25 | 25 | #15 |
| 10 | ACS723 current sensor | 1 | ACS723 module | 8 | 8 | #16 |
| 11 | NTC thermistor 10k | 3 | 10k, B3950 | 2 | 6 | #17 |
| 12 | 12V brushed DC motor (with encoder or mount) | 1 | 12V DC motor | 25 | 25 | #19 |
| 13 | Motor driver | 1 | BTS7960 H-bridge module | 10 | 10 | #20 |
| 14 | Steering servo | 1 | Metal gear servo (MG996R/DS3218 class) | 20 | 20 | #21 |
| 15 | Brake servo | 1 | Metal gear servo (MG996R/DS3218 class) | 20 | 20 | #21 |
| 16 | External watchdog IC | 4 | TPS3823-33DBVT | 1.5 | 6 | #22 |
| 17 | Kill relay | 1 | 12V automotive relay, 30A | 8 | 8 | #24 |
| 18 | Relay driver MOSFET | 1 | IRLZ44N | 1 | 1 | #25 |
| 19 | Flyback diode | 1 | 1N4007 | 0.1 | 1 | #26 |
| 20 | E-stop button | 1 | NC industrial mushroom button | 8 | 8 | #27 |
| 21 | OLED display | 1 | SSD1306 128x64 I2C | 4 | 4 | #28 |
| 22 | Status/fault LEDs + resistors | 1 set | Red/green/amber set | 3 | 3 | #29–31 |
| 23 | Piezo buzzer | 1 | 3.3V/5V active buzzer | 2 | 2 | #32 |
| 24 | Bench PSU 12V/5A or better | 1 | Adjustable lab supply | 20 | 20 | #34 |
| 25 | Buck converters | 3 | LM2596 modules (12V->5V/3.3V/6V) | 4 | 12 | #35 |
| 26 | Pi PSU + MicroSD | 1 set | 5V/3A USB-C + 32GB MicroSD | 20 | 20 | #4, #5 |
| 27 | CAN cable + wiring + connectors | 1 set | 22AWG twisted pair + JST/screw terminals | 35 | 35 | #12, #64–68 |
| 28 | Mounting + protoboards + standoffs | 1 set | Acrylic/plywood + perfboard kit | 40 | 40 | #59–63, #69–73 |
| 29 | Termination resistors | 2 | 120 ohm | 0.1 | 1 | #9 |
| | | | **Core Total (estimated)** | | **~$581** | |

## B. Optional / Recommended

| # | Item | Qty | Purpose | Est. Total (USD) |
|---|------|-----|---------|------------------|
| 30 | Oscilloscope | 1 | PWM/CAN timing debug and signal integrity | 400 |
| 31 | Extra transceivers + sensors | 1 set | Spares for bring-up failures | 30 |
| 32 | Better wiring kit + crimp tools | 1 set | Reliability and maintainability | 40 |
| | | | **Optional Total** | **~$470** |

## C. Minimum Bring-Up Order

1. Boards and CAN (`#1` to `#7`)
2. Safety chain (`#16` to `#20`)
3. Motor control path (`#12` to `#15`)
4. Sensors and UI (`#8` to `#11`, `#21` to `#23`)
5. Power and infrastructure (`#24` to `#29`)

## D. Procurement Tracker

| Item # | Ordered | Received | Tested | Notes |
|--------|---------|----------|--------|-------|
| 1 | [ ] | [ ] | [ ] | |
| 2 | [ ] | [ ] | [ ] | |
| 3 | [ ] | [ ] | [ ] | |
| 4 | [ ] | [ ] | [ ] | |
| 5 | [ ] | [ ] | [ ] | |
| 6 | [ ] | [ ] | [ ] | |
| 7 | [ ] | [ ] | [ ] | |
| 8 | [ ] | [ ] | [ ] | |
| 9 | [ ] | [ ] | [ ] | |
| 10 | [ ] | [ ] | [ ] | |
| 11 | [ ] | [ ] | [ ] | |
| 12 | [ ] | [ ] | [ ] | |
| 13 | [ ] | [ ] | [ ] | |
| 14 | [ ] | [ ] | [ ] | |
| 15 | [ ] | [ ] | [ ] | |
| 16 | [ ] | [ ] | [ ] | |
| 17 | [ ] | [ ] | [ ] | |
| 18 | [ ] | [ ] | [ ] | |
| 19 | [ ] | [ ] | [ ] | |
| 20 | [ ] | [ ] | [ ] | |
| 21 | [ ] | [ ] | [ ] | |
| 22 | [ ] | [ ] | [ ] | |
| 23 | [ ] | [ ] | [ ] | |
| 24 | [ ] | [ ] | [ ] | |
| 25 | [ ] | [ ] | [ ] | |
| 26 | [ ] | [ ] | [ ] | |
| 27 | [ ] | [ ] | [ ] | |
| 28 | [ ] | [ ] | [ ] | |
| 29 | [ ] | [ ] | [ ] | |

