---
document_id: BOM-LIST
title: "Procurement BOM List"
version: "1.0"
status: draft
date: 2026-02-21
---

# Procurement BOM List

Scope: 7-ECU zonal demo platform (4 physical ECUs + 3 simulated ECUs), Pi gateway, CAN bus, safety chain.

## A. Core Required (Build and Demo)

| # | Item | Qty | Preferred Part / Model | Est. Unit (USD) | Est. Total (USD) |
|---|------|-----|------------------------|------------------|------------------|
| 1 | STM32 Nucleo-64 board | 3 | `NUCLEO-G474RE` | 20 | 60 |
| 2 | Safety MCU LaunchPad | 1 | `LAUNCHXL2-570LC43` | 62 | 62 |
| 3 | Raspberry Pi 4 (2GB or 4GB) | 1 | Pi 4 Model B | 45 | 45 |
| 4 | CAN transceiver module for STM32 ECUs | 3 | TJA1051-based 3.3V breakout | 4 | 12 |
| 5 | CAN transceiver module for TMS570 | 1 | SN65HVD230 breakout | 5 | 5 |
| 6 | USB-CAN adapter (Pi capture/injection) | 1 | CANable 2.0 | 35 | 35 |
| 7 | USB-CAN adapter (PC bridge to simulated ECUs) | 1 | CANable 2.0 | 35 | 35 |
| 8 | AS5048A magnetic angle sensor + magnet | 3 | AS5048A breakout + diametric magnet | 15 | 45 |
| 9 | TFMini-S lidar (UART) | 1 | Benewake TFMini-S | 25 | 25 |
| 10 | ACS723 current sensor | 1 | ACS723 module | 8 | 8 |
| 11 | NTC thermistor 10k | 3 | 10k, B3950 | 2 | 6 |
| 12 | 12V brushed DC motor (with encoder or mount) | 1 | 12V DC motor | 25 | 25 |
| 13 | Motor driver | 1 | BTS7960 H-bridge module | 10 | 10 |
| 14 | Steering servo | 1 | Metal gear servo (MG996R/DS3218 class) | 20 | 20 |
| 15 | Brake servo | 1 | Metal gear servo (MG996R/DS3218 class) | 20 | 20 |
| 16 | External watchdog IC | 4 | TPS3823-33DBVT | 1.5 | 6 |
| 17 | Kill relay | 1 | 12V automotive relay, 30A | 8 | 8 |
| 18 | Relay driver MOSFET | 1 | IRLZ44N | 1 | 1 |
| 19 | Flyback diode | 1 | 1N4007 | 0.1 | 1 |
| 20 | E-stop button | 1 | NC industrial mushroom button | 8 | 8 |
| 21 | OLED display | 1 | SSD1306 128x64 I2C | 4 | 4 |
| 22 | Status/fault LEDs + resistors | 1 set | Red/green/amber set | 3 | 3 |
| 23 | Piezo buzzer | 1 | 3.3V/5V active buzzer | 2 | 2 |
| 24 | Bench PSU 12V/5A or better | 1 | Adjustable lab supply | 20 | 20 |
| 25 | Buck converters | 3 | LM2596 modules (12V->5V/3.3V/6V) | 4 | 12 |
| 26 | Pi PSU + MicroSD | 1 set | 5V/3A USB-C + 32GB MicroSD | 20 | 20 |
| 27 | CAN cable + wiring + connectors | 1 set | 22AWG twisted pair + JST/screw terminals | 35 | 35 |
| 28 | Mounting + protoboards + standoffs | 1 set | Acrylic/plywood + perfboard kit | 40 | 40 |
| 29 | Termination resistors | 2 | 120 ohm | 0.1 | 1 |
| | | | **Core Total (estimated)** | | **~$581** |

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
