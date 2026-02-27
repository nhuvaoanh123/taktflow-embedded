---
document_id: BOM
title: "Bill of Materials"
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

## Lessons Learned Rule

Every BOM item in this document that undergoes HITL review discussion MUST have its own lessons-learned file in [`hardware/lessons-learned/`](lessons-learned/). One file per BOM item. File naming: `BOM-<item>.md`.

# Bill of Materials

## 1. Purpose

Complete bill of materials for the Taktflow Zonal Vehicle Platform hardware. All components needed to build the full 4-ECU physical platform with sensors, actuators, safety hardware, power distribution, and edge gateway.

## 2. Budget

| | Amount |
|---|--------|
| **Total Budget** | **$2,000** |
| Total BOM (without test equipment) | $577 |
| Test Equipment (oscilloscope) | $400 |
| Total BOM (with test equipment) | $977 |
| **Remaining Budget** | **$1,023** |

---

## 3. BOM Table

### 3.1 MCU Development Boards

| # | Component | Qty | Unit Cost | Total | Supplier | Part Number | Status | Procurement Priority |
|---|-----------|-----|-----------|-------|----------|-------------|--------|---------------------|
| 1 | STM32G474RE Nucleo-64 | 3 | $16.00 | $48.00 | Mouser / ST Direct | NUCLEO-G474RE | Not ordered | Phase 1 (first) |
| 2 | TMS570LC43x LaunchPad | 1 | $40.00 | $40.00 | Mouser / TI Direct | LAUNCHXL2-570LC43 | Not ordered | Phase 1 (first) |
| 3 | Raspberry Pi 4 Model B (4GB) | 1 | $55.00 | $55.00 | Adafruit / PiShop | SC0194 | Not ordered | Phase 2 |
| 4 | 32GB MicroSD Card (Class 10) | 1 | $9.00 | $9.00 | Amazon | SanDisk Ultra | Not ordered | Phase 2 |
| 5 | RPi 4 USB-C Power Supply (5V/3A) | 1 | $8.00 | $8.00 | Adafruit / Amazon | CanaKit 3.5A | Not ordered | Phase 2 |
| | **Subtotal: MCU Boards** | | | **$160.00** | | | | |

<!-- HITL-LOCK START:COMMENT-BLOCK-BOM-SEC1 -->
**HITL Review (An Dao) — Reviewed: 2026-02-27:** The MCU board selection is appropriate: STM32G474RE Nucleo-64 x3 for CVC/FZC/RZC provides FDCAN, adequate timers, and ADC channels. TMS570LC43x LaunchPad for SC provides lockstep CPU and DCAN. The Raspberry Pi 4 as edge gateway is a standard choice. Pricing appears realistic at $160 subtotal. The Phase 1/Phase 2 prioritization is correct -- boards and RPi are properly sequenced. No safety-critical concerns with the board selections.
<!-- HITL-LOCK END:COMMENT-BLOCK-BOM-SEC1 -->

### 3.2 CAN Bus Components

| # | Component | Qty | Unit Cost | Total | Supplier | Part Number | Status | Procurement Priority |
|---|-----------|-----|-----------|-------|----------|-------------|--------|---------------------|
| 6 | TJA1051T/3 CAN Transceiver Module (3.3V) | 3 | $4.00 | $12.00 | Amazon / AliExpress | TJA1051 breakout module | Not ordered | Phase 1 |
| 7 | SN65HVD230 CAN Transceiver Module (3.3V) | 1 | $3.00 | $3.00 | Amazon / AliExpress | SN65HVD230 breakout | Not ordered | Phase 1 |
| 8 | CANable 2.0 (USB-CAN adapter, candleLight FW) | 2 | $30.00 | $60.00 | CANable.com / Tindie | CANable 2.0 | Not ordered | Phase 1 |
| 9 | 120 ohm Termination Resistors (1/4W, 1%) | 4 | $0.10 | $0.40 | Amazon / Mouser | CF1/4W-120R | Not ordered | Phase 1 |
| 10 | Common-Mode Choke (100 uH, CAN-rated) | 4 | $1.50 | $6.00 | Mouser / Digikey | ACM2012-102-2P | Not ordered | Phase 1 |
| 11 | PESD1CAN TVS Diode Array (CAN ESD protection) | 4 | $0.80 | $3.20 | Mouser / Digikey | PESD1CAN,215 | Not ordered | Phase 1 |
| 12 | 22 AWG Twisted Pair Wire (5 m) | 1 | $5.00 | $5.00 | Amazon | Stranded, yellow+green | Not ordered | Phase 1 |
| | **Subtotal: CAN Bus** | | | **$89.60** | | | | |

<!-- HITL-LOCK START:COMMENT-BLOCK-BOM-SEC2 -->
**HITL Review (An Dao) — Reviewed: 2026-02-27:** CAN bus components are well-selected. TJA1051T/3 (3.3V) for STM32 ECUs and SN65HVD230 for TMS570 SC are both appropriate 3.3V CAN transceivers. Two CANable 2.0 USB-CAN adapters provide adequate bus analysis capability (one per bus segment or monitoring + injection). The 120-ohm terminators, common-mode chokes, and PESD1CAN TVS diodes demonstrate proper CAN bus design with EMC and ESD protection. The twisted pair wire selection is appropriate. All items correctly marked Phase 1.
<!-- HITL-LOCK END:COMMENT-BLOCK-BOM-SEC2 -->

### 3.3 Sensors

| # | Component | Qty | Unit Cost | Total | Supplier | Part Number | Status | Procurement Priority |
|---|-----------|-----|-----------|-------|----------|-------------|--------|---------------------|
| 13 | AS5048A Magnetic Angle Sensor Module (SPI, 14-bit) | 3 | $12.00 | $36.00 | AliExpress / Amazon | AS5048A breakout | Not ordered | Phase 1 |
| 14 | Diametric Magnet (6mm dia x 2.5mm, AS5048A compatible) | 3 | $2.00 | $6.00 | AliExpress / Amazon | Diametrically magnetized | Not ordered | Phase 1 |
| 15 | TFMini-S Lidar Sensor (UART, 0.1-12m, 100Hz) | 1 | $30.00 | $30.00 | Amazon / RobotShop | TFMini-S (Benewake) | Not ordered | Phase 2 |
| 16 | ACS723LLCTR-20AB-T Current Sensor Module (Hall, +/-20A) | 1 | $6.00 | $6.00 | AliExpress / Amazon | ACS723 breakout | Not ordered | Phase 2 |
| 17 | NTC 10k Thermistor (B=3950, glass bead) | 3 | $1.00 | $3.00 | Amazon / AliExpress | NTC 10K 3950 | Not ordered | Phase 2 |
| 18 | Quadrature Encoder (11 PPR, for DC motor shaft) | 1 | $5.00 | $5.00 | Amazon / AliExpress | Incremental encoder | Not ordered | Phase 2 |
| | **Subtotal: Sensors** | | | **$86.00** | | | | |

<!-- HITL-LOCK START:COMMENT-BLOCK-BOM-SEC3 -->
**HITL Review (An Dao) — Reviewed: 2026-02-27:** Sensor selection is appropriate for the system architecture. Three AS5048A modules cover CVC dual-pedal (x2) and FZC steering angle (x1), with diametric magnets for each. The TFMini-S lidar (0.1-12m, 100Hz UART) matches the system requirements for obstacle detection. The ACS723 Hall-effect current sensor (+/-20A) is suitable for motor current monitoring. NTC thermistors (x3) for motor, board, and ambient temperature are standard. The quadrature encoder for motor speed feedback is correct. All sensors have alternatives listed in section 6. Phase 2 prioritization is appropriate.
<!-- HITL-LOCK END:COMMENT-BLOCK-BOM-SEC3 -->

### 3.4 Actuators

| # | Component | Qty | Unit Cost | Total | Supplier | Part Number | Status | Procurement Priority |
|---|-----------|-----|-----------|-------|----------|-------------|--------|---------------------|
| 19 | 12V DC Brushed Motor (with encoder mount) | 1 | $15.00 | $15.00 | Amazon | 775 Motor 12V 12000RPM | Not ordered | Phase 2 |
| 20 | BTS7960 H-Bridge Motor Driver Module (43A) | 1 | $12.00 | $12.00 | Amazon / AliExpress | BTS7960 module | Not ordered | Phase 2 |
| 21 | MG996R Metal Gear Servo (180 degree, 50Hz) | 2 | $8.00 | $16.00 | Amazon | MG996R (TowerPro or equiv.) | Not ordered | Phase 2 |
| | **Subtotal: Actuators** | | | **$43.00** | | | | |

<!-- HITL-LOCK START:COMMENT-BLOCK-BOM-SEC4 -->
**HITL Review (An Dao) — Reviewed: 2026-02-27:** Actuator selection is appropriate. The 12V DC brushed motor (775 type, 12000 RPM) is a standard bench motor. The BTS7960 H-bridge (43A rating) provides significant headroom for the motor. Two MG996R servos for steering and braking are adequate for a demonstration platform. All actuators correctly in Phase 2. One consideration: the 775 motor at 12000 RPM may need a gear reduction for realistic vehicle speed simulation -- ensure this is addressed in the mechanical design.
<!-- HITL-LOCK END:COMMENT-BLOCK-BOM-SEC4 -->

### 3.5 Safety Hardware

| # | Component | Qty | Unit Cost | Total | Supplier | Part Number | Status | Procurement Priority |
|---|-----------|-----|-----------|-------|----------|-------------|--------|---------------------|
| 22 | TPS3823-33DBVT External Watchdog IC (SOT-23-5) | 5 | €1.60 | €8.00 | Reichelt.de | TPS3823-33DBVT (TI) | **Ordered** 2026-02-27 | Phase 1 |
| 23 | SOT-23-5 Breakout Board (for TPS3823 soldering) | 4 | $0.50 | $2.00 | Amazon / AliExpress | SOT-23-5 adapter PCB | Not ordered | Phase 1 |
| 24 | Automotive Relay (12V coil, 30A SPST-NO) | 1 | $4.00 | $4.00 | Amazon / AutoZone | Bosch 0332019150 or equiv. | Not ordered | Phase 1 |
| 25 | IRLZ44N N-Channel MOSFET (logic-level, TO-220) | 1 | $1.50 | $1.50 | Amazon / Mouser | IRLZ44NPBF | Not ordered | Phase 1 |
| 26 | 1N4007 Rectifier Diode (flyback protection) | 2 | $0.10 | $0.20 | Amazon / Mouser | 1N4007 | Not ordered | Phase 1 |
| 27 | E-Stop Button (NC, mushroom head, red, panel mount) | 1 | $5.00 | $5.00 | Amazon / AliExpress | 22mm NC emergency stop | Not ordered | Phase 1 |
| | **Subtotal: Safety HW** | | | **~$21.35** | | | | |

<!-- HITL-LOCK START:COMMENT-BLOCK-BOM-SEC5 -->
**HITL Review (An Dao) — Reviewed: 2026-02-27:** Safety hardware selection is critical and well-considered. Four TPS3823DBVR external watchdog ICs (one per physical ECU) implement independent watchdog supervision -- this is a key ASIL D requirement. The SOT-23-5 breakout boards for hand-soldering the TPS3823 are practical for prototyping. The automotive relay (30A SPST-NO) with IRLZ44N MOSFET driver implements the energize-to-run kill relay (SYS-024). The 1N4007 flyback diodes protect the MOSFET. The NC mushroom-head E-stop button is safety-standard compliant. All safety hardware correctly in Phase 1. This is the highest-priority BOM section from a safety perspective.
<!-- HITL-LOCK END:COMMENT-BLOCK-BOM-SEC5 -->

### 3.6 User Interface

| # | Component | Qty | Unit Cost | Total | Supplier | Part Number | Status | Procurement Priority |
|---|-----------|-----|-----------|-------|----------|-------------|--------|---------------------|
| 28 | SSD1306 OLED Display (128x64, I2C, 0.96 inch) | 1 | $5.00 | $5.00 | Amazon / AliExpress | SSD1306 I2C 0x3C | Not ordered | Phase 2 |
| 29 | LED 3mm Red (through-hole) | 5 | $0.10 | $0.50 | Amazon | Standard red LED | Not ordered | Phase 1 |
| 30 | LED 3mm Green (through-hole) | 5 | $0.10 | $0.50 | Amazon | Standard green LED | Not ordered | Phase 1 |
| 31 | LED 3mm Amber (through-hole) | 2 | $0.10 | $0.20 | Amazon | Standard amber LED | Not ordered | Phase 1 |
| 32 | Active Piezo Buzzer (3.3V/5V, 85dB+) | 1 | $1.50 | $1.50 | Amazon | 5V active buzzer module | Not ordered | Phase 2 |
| 33 | 2N7002 N-MOSFET (SOT-23, for buzzer driver) | 1 | $0.30 | $0.30 | Amazon / Mouser | 2N7002 | Not ordered | Phase 2 |
| | **Subtotal: UI** | | | **$8.00** | | | | |

<!-- HITL-LOCK START:COMMENT-BLOCK-BOM-SEC6 -->
**HITL Review (An Dao) — Reviewed: 2026-02-27:** UI components are modest and appropriate for a development/demo platform. The SSD1306 OLED (128x64, I2C) on CVC provides sufficient resolution for basic status display. LEDs (red, green, amber) cover status and fault indication needs per the pin mapping. The active piezo buzzer with 2N7002 MOSFET driver matches the FZC pin mapping. The $8 subtotal is minimal. All items are correctly prioritized (LEDs in Phase 1 for early status feedback, display/buzzer in Phase 2).
<!-- HITL-LOCK END:COMMENT-BLOCK-BOM-SEC6 -->

### 3.7 Power Supply

| # | Component | Qty | Unit Cost | Total | Supplier | Part Number | Status | Procurement Priority |
|---|-----------|-----|-----------|-------|----------|-------------|--------|---------------------|
| 34 | 12V 10A Bench DC Power Supply | 1 | $25.00 | $25.00 | Amazon | Adjustable PSU 12V 10A | Not ordered | Phase 1 (first) |
| 35 | LM2596 Buck Converter Module (adjustable, 3A) | 2 | $3.00 | $6.00 | Amazon / AliExpress | LM2596 step-down module | Not ordered | Phase 1 |
| 36 | SB560 Schottky Diode (5A, 60V, reverse polarity) | 1 | $0.50 | $0.50 | Amazon / Mouser | SB560 | Not ordered | Phase 1 |
| 37 | ATC/ATO Blade Fuse Holder (inline) | 2 | $2.00 | $4.00 | Amazon / AutoZone | Inline fuse holder | Not ordered | Phase 1 |
| 38 | ATC Blade Fuse 10A | 2 | $0.50 | $1.00 | Amazon / AutoZone | Standard 10A fuse | Not ordered | Phase 1 |
| 39 | ATC Blade Fuse 30A | 2 | $0.50 | $1.00 | Amazon / AutoZone | Standard 30A fuse | Not ordered | Phase 1 |
| 40 | Glass Tube Fuse 3A (fast-blow, 5x20mm) | 4 | $0.30 | $1.20 | Amazon | 3A 250V fast-blow | Not ordered | Phase 2 |
| 41 | Glass Tube Fuse Holder (inline, 5x20mm) | 2 | $1.00 | $2.00 | Amazon | Inline fuse holder | Not ordered | Phase 2 |
| 42 | 6V Voltage Regulator Module (LM7806 or buck) | 2 | $2.50 | $5.00 | Amazon / AliExpress | 6V step-down module | Not ordered | Phase 2 |
| | **Subtotal: Power Supply** | | | **$45.70** | | | | |

<!-- HITL-LOCK START:COMMENT-BLOCK-BOM-SEC7 -->
**HITL Review (An Dao) — Reviewed: 2026-02-27:** Power supply design is practical. The 12V 10A bench PSU provides adequate power for the motor and all ECUs. Two LM2596 buck converters (for 5V and potentially 3.3V rails) are standard. The SB560 Schottky diode for reverse polarity protection is essential. Inline fuse holders with 10A and 30A fuses provide branch protection. The 6V regulators for servo power (separate from logic) is good practice. The glass tube fuses for per-ECU protection are appropriate. One concern: verify that the LM2596 output ripple is acceptable for the ADC reference on the RZC -- a low-noise LDO post-regulator may be needed for analog-sensitive circuits.
<!-- HITL-LOCK END:COMMENT-BLOCK-BOM-SEC7 -->

### 3.8 Passive Components

| # | Component | Qty | Unit Cost | Total | Supplier | Part Number | Status | Procurement Priority |
|---|-----------|-----|-----------|-------|----------|-------------|--------|---------------------|
| 43 | 100 nF Ceramic Capacitor (0.1uF, 50V) | 30 | $0.05 | $1.50 | Amazon / Mouser | MLCC 100nF | Not ordered | Phase 1 |
| 44 | 1 nF Ceramic Capacitor (1000pF, 50V) | 2 | $0.05 | $0.10 | Amazon / Mouser | MLCC 1nF | Not ordered | Phase 1 |
| 45 | 10 uF Electrolytic Capacitor (25V) | 5 | $0.10 | $0.50 | Amazon | Electrolytic 10uF | Not ordered | Phase 1 |
| 46 | 100 uF Electrolytic Capacitor (25V) | 5 | $0.15 | $0.75 | Amazon | Electrolytic 100uF | Not ordered | Phase 1 |
| 47 | 220 uF Electrolytic Capacitor (16V) | 2 | $0.20 | $0.40 | Amazon | Electrolytic 220uF | Not ordered | Phase 1 |
| 48 | 470 uF Electrolytic Capacitor (16V) | 3 | $0.30 | $0.90 | Amazon | Electrolytic 470uF | Not ordered | Phase 1 |
| 49 | 10k ohm Resistor (1/4W, 1%) | 20 | $0.02 | $0.40 | Amazon | Metal film 10k | Not ordered | Phase 1 |
| 50 | 47k ohm Resistor (1/4W, 1%) | 5 | $0.02 | $0.10 | Amazon | Metal film 47k | Not ordered | Phase 1 |
| 51 | 330 ohm Resistor (1/4W, 5%) | 10 | $0.02 | $0.20 | Amazon | Carbon film 330R | Not ordered | Phase 1 |
| 52 | 100 ohm Resistor (1/4W, 5%) | 5 | $0.02 | $0.10 | Amazon | Carbon film 100R | Not ordered | Phase 1 |
| 53 | 4.7k ohm Resistor (1/4W, 1%) | 5 | $0.02 | $0.10 | Amazon | Metal film 4.7k | Not ordered | Phase 1 |
| 54 | 1 uF Ceramic Capacitor (50V) | 5 | $0.10 | $0.50 | Amazon / Mouser | MLCC 1uF | Not ordered | Phase 1 |
| | **Subtotal: Passives** | | | **$5.55** | | | | |

<!-- HITL-LOCK START:COMMENT-BLOCK-BOM-SEC8 -->
**HITL Review (An Dao) — Reviewed: 2026-02-27:** Passive component selection is comprehensive. The quantities are generous (e.g., 30x 100nF bypass caps, 20x 10k resistors) which is good for prototyping with spares. The 10k resistors serve as pull-ups (SPI CS, I2C), voltage divider references (NTC), and pull-downs (MOSFET gates). The 47k resistors are for the battery voltage divider (47k/10k = ~5.7x ratio for 12V to ADC range). The 4.7k resistors are for I2C pull-ups. The 330-ohm LED current limiters are standard. All passives correctly in Phase 1. No gaps identified.
<!-- HITL-LOCK END:COMMENT-BLOCK-BOM-SEC8 -->

### 3.9 Protection Components

| # | Component | Qty | Unit Cost | Total | Supplier | Part Number | Status | Procurement Priority |
|---|-----------|-----|-----------|-------|----------|-------------|--------|---------------------|
| 55 | BZX84C3V3 Zener Diode (3.3V, SOD-323 or THT) | 5 | $0.15 | $0.75 | Mouser / Amazon | BZX84C3V3 | Not ordered | Phase 1 |
| 56 | 3.3V Bidirectional TVS Diode (SOD-323 or THT) | 6 | $0.30 | $1.80 | Mouser / Amazon | PESD3V3U1UA | Not ordered | Phase 1 |
| 57 | 1N5819 Schottky Diode (1A, 40V, flyback) | 4 | $0.15 | $0.60 | Amazon / Mouser | 1N5819 | Not ordered | Phase 1 |
| 58 | Resettable PTC Fuse (polyfuse, 3A, 6V) | 2 | $0.50 | $1.00 | Amazon / Mouser | MF-R300 | Not ordered | Phase 2 |
| | **Subtotal: Protection** | | | **$4.15** | | | | |

<!-- HITL-LOCK START:COMMENT-BLOCK-BOM-SEC9 -->
**HITL Review (An Dao) — Reviewed: 2026-02-27:** Protection components are essential for hardware safety. The BZX84C3V3 Zener diodes clamp ADC inputs to 3.3V (protecting against overvoltage from the voltage divider or sensor faults). The 3.3V bidirectional TVS diodes protect digital inputs (E-stop, encoder, lidar RX). The 1N5819 Schottky diodes provide additional flyback protection. The resettable PTC fuses (polyfuses) add self-recovering overcurrent protection. Quantities match the pin mapping requirements. All protection components should be verified against the specific circuit in the schematic design phase.
<!-- HITL-LOCK END:COMMENT-BLOCK-BOM-SEC9 -->

### 3.10 Infrastructure and Mounting

| # | Component | Qty | Unit Cost | Total | Supplier | Part Number | Status | Procurement Priority |
|---|-----------|-----|-----------|-------|----------|-------------|--------|---------------------|
| 59 | Base Plate (plywood or acrylic, 400x300mm) | 1 | $8.00 | $8.00 | Home Depot / Amazon | 3/4 inch plywood | Not ordered | Phase 1 |
| 60 | M3 Standoffs (Nylon, 10mm, M/F) + Screws | 20 | $0.15 | $3.00 | Amazon | M3 nylon standoff kit | Not ordered | Phase 1 |
| 61 | Proto/Perf Board (5x7cm, double-sided) | 5 | $1.00 | $5.00 | Amazon | Universal PCB | Not ordered | Phase 1 |
| 62 | Screw Terminal Block (2-position, 5.08mm pitch) | 10 | $0.30 | $3.00 | Amazon | 2P screw terminal | Not ordered | Phase 1 |
| 63 | Screw Terminal Block (3-position, 5.08mm pitch) | 6 | $0.40 | $2.40 | Amazon | 3P screw terminal | Not ordered | Phase 1 |
| 64 | Dupont Jumper Wires (M-F, 20cm, 40-pack) | 2 | $3.00 | $6.00 | Amazon | Dupont jumper cable | Not ordered | Phase 1 |
| 65 | Dupont Jumper Wires (M-M, 20cm, 40-pack) | 1 | $3.00 | $3.00 | Amazon | Dupont jumper cable | Not ordered | Phase 1 |
| 66 | 22 AWG Hookup Wire Kit (multiple colors, stranded) | 1 | $10.00 | $10.00 | Amazon | 22 AWG 6-color kit | Not ordered | Phase 1 |
| 67 | 18 AWG Hookup Wire (red + black, stranded, 5m each) | 1 | $5.00 | $5.00 | Amazon | 18 AWG 2-color | Not ordered | Phase 1 |
| 68 | 16 AWG Hookup Wire (red + black, stranded, 3m each) | 1 | $5.00 | $5.00 | Amazon | 16 AWG 2-color | Not ordered | Phase 1 |
| 69 | Cable Ties (100-pack, small) | 1 | $3.00 | $3.00 | Amazon | 4-inch cable ties | Not ordered | Phase 1 |
| 70 | Heat Shrink Tubing Assortment | 1 | $5.00 | $5.00 | Amazon | 2:1 heat shrink kit | Not ordered | Phase 1 |
| 71 | JST-XH Connector Kit (2/3/4 pin, male+female) | 1 | $8.00 | $8.00 | Amazon | JST-XH assortment | Not ordered | Phase 1 |
| 72 | Relay Socket (5-pin, panel mount, for automotive relay) | 1 | $2.00 | $2.00 | Amazon / AutoZone | 5-pin relay socket | Not ordered | Phase 1 |
| 73 | Breadboard (830 tie-point, full size) | 2 | $4.00 | $8.00 | Amazon | BB-830 breadboard | Not ordered | Phase 1 |
| | **Subtotal: Infrastructure** | | | **$76.40** | | | | |

<!-- HITL-LOCK START:COMMENT-BLOCK-BOM-SEC10 -->
**HITL Review (An Dao) — Reviewed: 2026-02-27:** Infrastructure and mounting is the largest non-board BOM section at $76.40, which is expected for a 4-ECU bench platform. The base plate (plywood/acrylic), M3 nylon standoffs, and proto boards provide a practical mounting solution. Screw terminal blocks (2P and 3P) are appropriate for power and signal distribution. The wire selection covers three gauges: 22 AWG for signals, 18 AWG for power distribution, and 16 AWG for motor/relay power -- this is correctly sized. JST-XH connectors provide reliable sensor/actuator connections. Two full-size breadboards support prototyping. The relay socket for the automotive relay is a good detail. Cable ties and heat shrink complete the assembly materials. All items correctly in Phase 1 since the physical platform must be built before sensors/actuators can be mounted.
<!-- HITL-LOCK END:COMMENT-BLOCK-BOM-SEC10 -->

### 3.11 Test Equipment

| # | Component | Qty | Unit Cost | Total | Supplier | Part Number | Status | Procurement Priority |
|---|-----------|-----|-----------|-------|----------|-------------|--------|---------------------|
| 74 | Rigol DS1054Z Digital Oscilloscope (50 MHz, 4 ch) | 1 | $400.00 | $400.00 | Amazon / Tequipment | DS1054Z | Not ordered | Phase 3 (optional) |
| | **Subtotal: Test Equipment** | | | **$400.00** | | | | |

<!-- HITL-LOCK START:COMMENT-BLOCK-BOM-SEC11 -->
**HITL Review (An Dao) — Reviewed: 2026-02-27:** The Rigol DS1054Z is a well-known 50 MHz 4-channel oscilloscope that is standard for hobbyist and educational use. At $400 it represents 42% of the total BOM with test equipment included. The 4-channel capability is important for simultaneously monitoring CAN-H, CAN-L, chip select, and data lines during SPI/CAN debugging. Being Phase 3 (optional) is correct -- firmware development and basic CAN bus testing can proceed with the CANable USB adapters. However, for timing verification (WCET measurement, interrupt latency, PWM duty cycle accuracy) and analog signal integrity (ADC input noise, sensor waveforms), an oscilloscope is strongly recommended before hardware validation sign-off. Consider also whether a logic analyzer (e.g., Saleae Logic 8 at ~$200) might be a cost-effective complement or alternative for digital protocol debugging.
<!-- HITL-LOCK END:COMMENT-BLOCK-BOM-SEC11 -->

---

## 4. Cost Summary

| Category | Items | Subtotal |
|----------|-------|----------|
| MCU Development Boards | 5 items | $160.00 |
| CAN Bus Components | 7 items | $89.60 |
| Sensors | 6 items | $86.00 |
| Actuators | 3 items | $43.00 |
| Safety Hardware | 6 items | $18.70 |
| User Interface | 6 items | $8.00 |
| Power Supply | 9 items | $45.70 |
| Passive Components | 12 items | $5.55 |
| Protection Components | 4 items | $4.15 |
| Infrastructure & Mounting | 15 items | $76.40 |
| **Total (without test equipment)** | **73 items** | **$537.10** |
| Test Equipment (oscilloscope) | 1 item | $400.00 |
| **Total (with test equipment)** | **74 items** | **$937.10** |

### 4.1 Budget Tracking

| Metric | Value |
|--------|-------|
| Total Budget | $2,000.00 |
| BOM Total (without scope) | $537.10 |
| BOM Total (with scope) | $937.10 |
| Remaining (without scope) | $1,462.90 |
| Remaining (with scope) | $1,062.90 |
| Budget Utilization (without scope) | 26.9% |
| Budget Utilization (with scope) | 46.9% |

---

## 5. Procurement Priority

### Phase 1 -- Core Platform (needed to build the bus and basic framework)

| Priority | Items | Estimated Cost | Why First |
|----------|-------|---------------|-----------|
| 1 | STM32 Nucleo x3, TMS570 LaunchPad x1, 12V PSU | $113.00 | Cannot start firmware dev without boards |
| 2 | CAN transceivers, CANable x2, terminators, wire | $89.60 | CAN bus is the backbone |
| 3 | Safety HW (TPS3823, relay, MOSFET, E-stop) | $18.70 | Safety architecture validation |
| 4 | Passives (resistors, capacitors) | $5.55 | Needed for all circuits |
| 5 | Protection (TVS, Zener, fuses) | $4.15 | Protect hardware from first power-up |
| 6 | Infrastructure (base plate, wires, connectors) | $76.40 | Physical mounting |
| 7 | LEDs | $1.20 | Status/fault indication |
| **Phase 1 Total** | | **$308.60** | |

### Phase 2 -- Sensors, Actuators, and Peripherals

| Priority | Items | Estimated Cost | Why Second |
|----------|-------|---------------|------------|
| 8 | AS5048A sensors x3, magnets x3 | $42.00 | Pedal and steering sensing |
| 9 | Motor, BTS7960, encoder | $32.00 | Motor control validation |
| 10 | Servos x2, 6V regulators, servo fuses | $37.00 | Steering and braking |
| 11 | TFMini-S lidar | $30.00 | Obstacle detection |
| 12 | ACS723, NTC thermistors | $9.00 | Current and temp monitoring |
| 13 | OLED, buzzer, buzzer MOSFET | $6.80 | UI components |
| 14 | RPi 4, SD card, USB-C PSU | $72.00 | Edge gateway |
| **Phase 2 Total** | | **$228.80** | |

### Phase 3 -- Test Equipment (Optional)

| Priority | Items | Estimated Cost | Why Third |
|----------|-------|---------------|-----------|
| 15 | Rigol DS1054Z oscilloscope | $400.00 | Signal validation, timing verification |
| **Phase 3 Total** | | **$400.00** | |

---

## 6. Alternative Components

For critical components, backup alternatives are identified:

| Primary Component | Alternative 1 | Alternative 2 | Notes |
|-------------------|---------------|---------------|-------|
| STM32G474RE Nucleo-64 | STM32G431RB Nucleo-64 ($13) | STM32F446RE Nucleo-64 ($14) | G431 has fewer ADC/timers. F446 lacks FDCAN (need MCP2515 SPI-CAN). |
| TMS570LC43x LaunchPad | TMS570LS12x LaunchPad ($20) | Hercules RM48 LaunchPad ($30) | LS12x has smaller flash (1.25MB). RM48 similar to TMS570. |
| TJA1051T/3 module | MCP2551 + level shifter | SN65HVD230 (same as SC) | MCP2551 needs 5V. SN65HVD230 is 3.3V. |
| CANable 2.0 | PCAN-USB ($250) | USBtin ($30) | PCAN is professional-grade. USBtin is budget option. |
| AS5048A | AS5047P (incremental + SPI) | AS5600 (I2C, 12-bit) | AS5047P lacks absolute position. AS5600 is I2C only (fewer pins free). |
| TFMini-S | TFLuna (I2C/UART, 0.2-8m, $25) | VL53L1X (I2C, 0.04-4m, $15) | TFLuna shorter range. VL53L1X very short range. |
| ACS723 | INA219 + shunt (I2C, $5) | ACS712 (5V, $3) | INA219 needs I2C. ACS712 needs 5V, not galvanically isolated. |
| BTS7960 | L298N ($5, 2A max) | VNH5019 ($25, 30A) | L298N too low current. VNH5019 is premium choice. |
| MG996R servo | DS3218 ($12, 20kg-cm) | SG90 ($2, 1.8kg-cm) | DS3218 higher torque. SG90 too weak. |

---

## 7. Procurement Status Tracking

| # | Component | Ordered | Received | Verified | Notes |
|---|-----------|---------|----------|----------|-------|
| 1 | STM32G474RE Nucleo-64 (x3) | -- | -- | -- | |
| 2 | TMS570LC43x LaunchPad | -- | -- | -- | |
| 3 | Raspberry Pi 4 4GB | -- | -- | -- | |
| 4 | 32GB MicroSD | -- | -- | -- | |
| 5 | RPi USB-C PSU | -- | -- | -- | |
| 6 | TJA1051T/3 module (x3) | -- | -- | -- | |
| 7 | SN65HVD230 module | -- | -- | -- | |
| 8 | CANable 2.0 (x2) | -- | -- | -- | |
| 9 | 120R resistors | -- | -- | -- | |
| 10 | Common-mode chokes (x4) | -- | -- | -- | |
| 11 | CAN TVS diodes (x4) | -- | -- | -- | |
| 12 | CAN twisted pair wire | -- | -- | -- | |
| 13 | AS5048A modules (x3) | -- | -- | -- | |
| 14 | Diametric magnets (x3) | -- | -- | -- | |
| 15 | TFMini-S lidar | -- | -- | -- | |
| 16 | ACS723 module | -- | -- | -- | |
| 17 | NTC 10k thermistors (x3) | -- | -- | -- | |
| 18 | Quadrature encoder | -- | -- | -- | |
| 19 | 12V DC motor | -- | -- | -- | |
| 20 | BTS7960 module | -- | -- | -- | |
| 21 | MG996R servos (x2) | -- | -- | -- | |
| 22 | TPS3823-33DBVT (x5) | 2026-02-27 | -- | -- | Reichelt.de, 5 pcs (4 needed + 1 spare), €8.00 total |
| 23 | SOT-23-5 breakout (x4) | -- | -- | -- | |
| 24 | 30A automotive relay | -- | -- | -- | |
| 25 | IRLZ44N MOSFET | -- | -- | -- | |
| 26 | 1N4007 diodes (x2) | -- | -- | -- | |
| 27 | E-stop button | -- | -- | -- | |
| 28 | SSD1306 OLED | -- | -- | -- | |
| 29-31 | LEDs (red, green, amber) | -- | -- | -- | |
| 32 | Piezo buzzer | -- | -- | -- | |
| 33 | 2N7002 MOSFET | -- | -- | -- | |
| 34 | 12V 10A PSU | -- | -- | -- | |
| 35 | LM2596 buck modules (x2) | -- | -- | -- | |
| 36 | SB560 Schottky | -- | -- | -- | |
| 37-41 | Fuses and holders | -- | -- | -- | |
| 42 | 6V regulator modules (x2) | -- | -- | -- | |
| 43-54 | Passive components | -- | -- | -- | |
| 55-58 | Protection components | -- | -- | -- | |
| 59-73 | Infrastructure | -- | -- | -- | |
| 74 | Rigol DS1054Z (optional) | -- | -- | -- | |

---

## 8. Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 0.1 | 2026-02-21 | System | Initial stub with summary table |
| 1.0 | 2026-02-21 | System | Complete BOM: 74 line items across 11 categories, per-item part numbers, budget tracking ($537 without scope), procurement priority phasing, alternative components for all critical parts |
| 1.1 | 2026-02-27 | An Dao | TPS3823-33DBVT ordered from Reichelt.de: 5 pcs × €1.60 = €8.00. Updated part number (DBVR→DBVT), qty (4→5, 1 spare), supplier (Mouser→Reichelt). First component ordered. |

