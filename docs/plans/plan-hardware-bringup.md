# Hardware Bring-Up Plan — Taktflow Zonal Vehicle Platform

> **Status**: Phase 0 — Gap Closure & Inventory Check (parts ordered 2026-02-23)
> **Created**: 2026-02-23
> **Last Updated**: 2026-02-23
> **Builder Profile**: Solo, SW-strong / HW-beginner
> **Estimated Completion**: ~4 weekends (40-60 hours hands-on)

## Context

All BOM items have been ordered/received. Firmware phases 0-6 are DONE (BSW layer + CVC SWCs). This plan is a structured, step-by-step guide to physically assemble and bring up the 4-ECU hardware platform, then integrate it with firmware. It bridges the gap between "parts on the desk" and "working HIL platform."

This plan is written for someone who is strong on the SW side but less experienced with HW integration. Every step includes what to check and what "good" looks like.

## Reference Files

- **`docs/plans/plan-hardware-bringup-workbook.md`** — **Detailed step-by-step assembly workbook** (companion to this plan)
- `hardware/bom.md` — Full 74-item BOM
- `hardware/bom-list.md` — Core 29-item quick reference
- `hardware/procurement-validation.md` — What was actually bought
- `hardware/pin-mapping.md` — All pin assignments (53 pins, 4 ECUs)
- `docs/aspice/hardware-eng/hw-design.md` — Circuit schematics (ASCII), assembly order, power-up checklist
- `docs/aspice/hardware-eng/hw-requirements.md` — 33 HWR specifications
- `docs/safety/requirements/hw-safety-reqs.md` — 25 HSR requirements
- `docs/plans/master-plan.md` — Phase 13 (HIL) definition

---

## Phase 0: Gap Closure & Inventory Check (Before You Touch a Soldering Iron)

### 0.1 Physical Inventory

Lay out every component you received. Check against BOM #1-29 using the `procurement-validation.md` snapshot. Physically verify quantity and condition.

### 0.2 Critical Gaps to Resolve

| Priority | Item | Why Critical | Action |
|----------|------|-------------|--------|
| **P0** | **6V regulator modules x2** | Servos rated 4.8-7.2V. Feeding 12V = instant smoke. | Order 2x LM7806 or adjustable buck set to 6V. **Do NOT power servos without this.** |
| **P0** | **Fuses: 10A blade + 30A blade + holders** | First short circuit = damaged boards/wiring with no fuse protection | Order inline fuse holders + 10A/30A blade fuses |
| **P0** | **100nF ceramic capacitors (bag of 30+)** | Every IC power pin needs one. Missing = noise, random resets, CAN errors | Verify you have these — they may have come in a kit |
| **P1** | **SB560 Schottky diode (or equivalent 5A/60V)** | Reverse polarity protection at 12V entry | Order 1. Or skip if you are 100% confident you won't reverse the PSU leads (risky) |
| **P1** | **Electrolytic capacitors: 10uF, 100uF, 220uF, 470uF** | Bulk decoupling for buck converters and servo power | Verify from kit. These are cheap — order a small assortment if unsure |
| **P1** | **Resistors: 10k, 47k, 330R, 100R, 4.7k** | Pull-ups, pull-downs, voltage dividers, LED current limiting | Verify from LED+resistor kit. Need: 10k (20x), 47k (5x), 330R (10x), 100R (5x), 4.7k (5x) |
| **P1** | **BZX84C3V3 Zener diodes x5** | ADC overvoltage clamp on RZC | Order if not in kit |
| **P2** | **16-18 AWG wire (red+black, 2m each)** | Power distribution paths need heavier gauge than signal wire | Check if included in wiring order |
| **P2** | **SOT-23-5 breakout boards x4** | For soldering TPS3823 watchdog ICs | Check if your TPS3823 came pre-mounted on breakout. If bare IC, you need these |
| **P2** | **Common-mode chokes 100uH x4** | CAN bus noise immunity per HWR-018 | Can skip for initial bring-up, add later |
| **P2** | **Quadrature encoder** | Motor speed feedback | Can skip for initial bring-up (open-loop motor test first) |

### 0.3 Gap Items Ordered (Amazon.de, 2026-02-23)

| # | Item | Qty | Product | Status |
|---|------|-----|---------|--------|
| 1 | LM7806 6V regulator module | 2 | Three Terminal Voltage Regulator Module (LM7806 variant) | Ordered |
| 2 | Fuse box + blade fuses (3A-40A) | 1 | EEFUN 6-way 32V fuse holder box + 10 fuses | Ordered |
| 3 | 100nF ceramic capacitors | 50 | BOJACK 0.1uF 50V ceramic disc capacitors | Ordered |
| 4 | SB560 Schottky diode (5A/60V) | 10 | HUABAN SR560 (SB560) DO-201AD | Ordered |
| 5 | Electrolytic capacitor kit | 130 (13 values) | POPESQ 1uF-1000uF (includes 10uF, 100uF, 220uF, 470uF) | Ordered |
| 6 | Resistor kit | 1350 (50 values) | BOJACK 0Ω-5.6MΩ 1% 1/4W metal film | Ordered |
| 7 | BZX84C3V3 Zener diodes (SOT-23) | 25 | BZX84C3V3-DIO64 0.3W 3.3V | Ordered |
| 8 | 16AWG silicone wire (red+black) | 2×(1m+1m) = 4m | 16AWG flexible tinned copper stranded | Ordered |
| 9 | SOT-23 breakout boards | 10 (pack) | Colcolo SOT23 to DIP adapter (6-pin) | Ordered |

**Estimated total: ~€104 (Amazon.de)**

### 0.4 Solder Bridge Modifications (Do Before Mounting)

Do these on the Nucleo boards **BEFORE** mounting to the base plate:

| Board | Bridge | Action | Tool | Why |
|-------|--------|--------|------|-----|
| **FZC Nucleo** | SB63 | Remove | Soldering iron + solder wick | Free PA2 for lidar UART TX |
| **FZC Nucleo** | SB65 | Remove | Soldering iron + solder wick | Free PA3 for lidar UART RX |
| **RZC Nucleo** | SB63 | Remove | Soldering iron + solder wick | Free PA2 for ADC board temp |
| **RZC Nucleo** | SB65 | Remove | Soldering iron + solder wick | Free PA3 for ADC battery voltage |
| CVC Nucleo | SB21 | Remove (optional) | Soldering iron + solder wick | Disconnect PA5 from onboard LED to avoid SPI SCK interference |

**How to remove a solder bridge**: Put flux on the bridge. Touch it with a clean soldering iron tip. The solder should wick onto the iron. Verify with multimeter in continuity mode — no beep = bridge removed.

**After SB63/SB65 removal**: You lose the ST-LINK virtual COM port (USART2). Use SWO (PB3) or LPUART1 (PG7/PG8 morpho pins) for debug printf.

### 0.5 Label Your Boards

Use masking tape + marker. Write clearly on each Nucleo:
- **CVC** (Central Vehicle Computer)
- **FZC** (Front Zone Controller)
- **RZC** (Rear Zone Controller)

And on the LaunchPad:
- **SC** (Safety Controller)

This prevents wiring to the wrong board.

### 0.6 Tools Needed

- Soldering iron + solder + flux + solder wick
- **Helping hands / PCB holder** (~€10) — essential for soldering, you can't hold board + wire + iron with 2 hands
- Multimeter (continuity, DC voltage, resistance modes)
- Oscilloscope (critical for CAN and timing verification)
- Wire strippers for 16-22 AWG
- Small screwdrivers (for screw terminals)
- USB cables: 3x micro-USB (Nucleo), 1x micro-USB (LaunchPad XDS110)
- Masking tape + marker (for labeling boards and wires)

---

## Phase 1: Base Plate & Power Distribution

> **Goal**: 12V in, 5V/3.3V out, all rails verified. No ECUs connected yet.

### 1.1 Prepare the Base Plate

- Cut plywood/acrylic to ~400x300mm (or use what you have)
- Plan layout (left-to-right): CVC — FZC — RZC — SC, with CAN bus along the top edge
- Motor/BTS7960 at the right end near RZC
- E-stop + relay + fault LEDs at the front edge (operator side)
- PSU entry point at the left rear corner

### 1.2 Mount Buck Converters

Mount the two LM2596 modules on the base plate:

1. **12V → 5V module**: Adjust the potentiometer BEFORE connecting load. Connect 12V input, measure output with multimeter, turn pot until you read 5.0V.
2. **12V → 3.3V module** (or TSR 2-2433N if using that): Same process, set to 3.3V.

If using the TSR 2-2433N (24V→3.3V, 2A): these are fixed output, no adjustment needed. Just verify input voltage range is compatible (TSR 2-2433N accepts 4.6V-36V input → fine for 12V).

### 1.3 Wire Power Distribution Bus

```
PSU + (red 16AWG) → [SB560 Schottky anode→cathode] → [10A fuse] → 12V MAIN RAIL screw terminal
PSU - (black 16AWG) → STAR GROUND POINT (single screw terminal block)
```

From 12V MAIN RAIL, branch out (18AWG each):
- To 5V buck converter input
- To 3.3V buck converter input
- Leave 3 drops for Nucleo Vin (don't connect yet)
- Leave 1 drop for LaunchPad (don't connect yet)
- To kill relay coil circuit area (don't wire relay yet)

### 1.4 Verify — Power Rails (No Load)

| Test | Expected | Tool |
|------|----------|------|
| 12V rail at screw terminal (PSU set to 12.0V) | 11.5-12.0V (after Schottky drop) | Multimeter DC voltage |
| 5V rail at buck output | 4.9-5.1V | Multimeter |
| 3.3V rail at buck output | 3.2-3.4V | Multimeter |
| Reverse polarity: swap PSU leads briefly | 0V on all rails, nothing smokes | Visual (if no Schottky, DO NOT do this) |
| 5V rail ripple (optional) | < 50mV p-p | Oscilloscope, AC coupled, 20MHz BW limit |
| 3.3V rail ripple (optional) | < 30mV p-p | Oscilloscope |

**STOP if any voltage is wrong. Debug before proceeding.**

---

## Phase 2: Mount ECU Boards & Individual Power-Up

> **Goal**: Each board boots independently via USB. No inter-board wiring yet.

### 2.1 Mount Boards on Base Plate

Use M3 nylon standoffs (10mm) under each board. Screw down firmly. Layout:

```
[PSU entry]
                [CVC]    [FZC]    [RZC]    [SC]
                  |        |        |        |
    ============ CAN BUS ROUTE (top edge) ============

[E-stop] [Relay] [LEDs]   [Motor area]   [BTS7960]
```

### 2.2 Individual USB Power-Up Test

For each board, one at a time:
1. Connect only USB cable (no 12V yet)
2. Verify it powers up (green LED on Nucleo, XDS110 LED on LaunchPad)
3. Verify your PC sees the ST-LINK (for Nucleo) or XDS110 (for LaunchPad)
4. Flash a simple "blink" or "hello" firmware to confirm programming works
5. Disconnect USB

| Board | USB Port | Expected |
|-------|----------|----------|
| CVC Nucleo | Micro-USB (CN1) | LD2 blinks (default factory firmware) |
| FZC Nucleo | Micro-USB (CN1) | LD2 blinks |
| RZC Nucleo | Micro-USB (CN1) | LD2 blinks |
| SC LaunchPad | Micro-USB (XDS110) | LED on LaunchPad blinks (factory demo) |

### 2.3 Connect 12V to ECU Boards

Now wire 12V main rail (18AWG) to each Nucleo Vin pin:
- **CVC**: Red wire to CN7 pin 24 (VIN), Black to CN7 pin 20 (GND)
- **FZC**: Same morpho pins
- **RZC**: Same morpho pins
- **SC**: Via LaunchPad 3.3V/5V input headers OR keep on USB power (simplest for bench)

**Verify each board still boots with 12V** (unplug USB first to test standalone):
- Nucleo green LED should light up
- Measure 3.3V on Nucleo CN7 pin 16 (3V3 output) → should read 3.2-3.4V

---

## Phase 3: CAN Bus Wiring

> **Goal**: All 4 ECUs + Pi can see each other on a 500 kbps CAN bus.

### 3.1 Solder CAN Transceivers onto Perfboard/Breadboard

**For each STM32 ECU (CVC, FZC, RZC):**
1. Mount TJA1051 module on breadboard/perfboard near the Nucleo
2. Wire: VCC → external 3.3V rail, GND → ground, 100nF cap on VCC
3. Wire: TXD → PA12 (FDCAN1_TX, CN10-12 on Nucleo)
4. Wire: RXD → PA11 (FDCAN1_RX, CN10-14 on Nucleo)
5. Wire: STB → GND (always active)

**For SC:**
1. Mount SN65HVD230 module near the LaunchPad
2. Wire: VCC → 3.3V, GND → ground, 100nF cap on VCC
3. Wire: TXD → DCAN1TX (J5 edge connector on LaunchPad)
4. Wire: RXD → DCAN1RX (J5 edge connector)
5. Wire: Rs → GND (full speed, no slope control)

### 3.2 Run the CAN Bus Trunk

Using 22AWG twisted pair (yellow = CAN_H, green = CAN_L):
1. Run a continuous trunk wire along the top edge of the base plate
2. At each ECU location, T-tap the trunk to a 3-position screw terminal (CAN_H, CAN_L, CAN_GND)
3. Keep stub wires from trunk to each transceiver < 100mm
4. Install 120R termination resistor at CVC end (across CAN_H and CAN_L)
5. Install 120R termination resistor at SC end

### 3.3 Connect Pi + CANable

1. Plug CANable #1 into Raspberry Pi USB
2. T-tap from CAN bus trunk to CANable (< 100mm stub)
3. Plug CANable #2 into your development PC (for analysis/simulated ECUs)
4. T-tap from trunk

### 3.4 Verify — CAN Bus Electrical

| Test | Expected | Tool |
|------|----------|------|
| Termination resistance (CAN_H to CAN_L, all power OFF) | 60 ohm (two 120R in parallel) | Multimeter resistance mode |
| CAN_H and CAN_L at rest (power ON, no traffic) | Both at ~2.5V | Multimeter DC |
| CAN_H dominant level (during TX) | ~3.5V | Oscilloscope |
| CAN_L dominant level (during TX) | ~1.5V | Oscilloscope |
| Differential (CAN_H - CAN_L) during dominant | ~2.0V | Oscilloscope (math channel) |

### 3.5 Verify — CAN Bus Communication

Flash a minimal CAN TX firmware on CVC that sends a heartbeat frame (ID 0x010) every 100ms.

1. On your PC with CANable #2, run: `candump can0` (Linux) or use a CAN analyzer tool (Windows)
2. You should see 0x010 frames arriving at 10 Hz
3. Flash CAN RX firmware on another Nucleo — verify it receives the frames
4. Repeat for each ECU until all 4 can TX and RX on the bus
5. Test Pi: `candump can0` on the Pi should also show all traffic

**Common CAN problems and fixes:**
- No traffic at all → check termination (60 ohm?), check TX/RX not swapped
- Intermittent errors → check decoupling caps on transceivers, check wire connections
- Bus-off → check bit timing matches between STM32 and TMS570 (both 500 kbps, 80% sample point)

---

## Phase 4: Safety Chain

> **Goal**: Kill relay works, E-stop works, watchdogs reset MCUs. This must work before connecting any actuator.

### 4.1 Wire the Kill Relay Circuit

Follow the schematic in `hw-design.md` Section 5.4.2 exactly:

```
12V Main Rail → Relay Coil (+) → Relay Coil (-) → IRLZ44N Drain
                  ↕ 1N4007 flyback (cathode to +)
IRLZ44N Source → GND
IRLZ44N Gate ← 100R resistor ← SC GIO_A0 (J3-1)
               ↓
             10k resistor → GND (pull-down)
```

Relay contact (SPST-NO):
- Common → 12V main rail
- NO → 12V actuator rail (via 30A fuse)

### 4.2 Verify Kill Relay

| Test | How | Expected |
|------|-----|----------|
| Default state (SC not powered) | Measure voltage on actuator rail | 0V (relay open) |
| SC powered, GIO_A0 = LOW (default) | Measure actuator rail | 0V (relay open) |
| SC drives GIO_A0 = HIGH | Measure actuator rail | ~12V (relay closed, click sound) |
| SC drives GIO_A0 = LOW | Measure actuator rail + listen | 0V within 10ms (relay opens, click) |
| Pull USB from SC (power loss) | Measure actuator rail | 0V (fail-safe: 10k pulls gate low) |
| Relay dropout time | Oscilloscope on actuator rail, trigger on GIO_A0 falling | < 10ms from GIO_A0 LOW to rail = 0V |

### 4.3 Wire E-Stop Button

```
PC13 (CVC CN7-23) ← 10k series resistor ← junction
                                            ↓
                                        100nF cap → GND
                                            ↓
                                     NC E-Stop button → GND
```

Internal pull-up on PC13 (enabled in firmware).

### 4.4 Verify E-Stop

| Test | Expected |
|------|----------|
| Button NOT pressed (resting) | PC13 = LOW (0V, button shorts to GND) |
| Button PRESSED (locked) | PC13 = HIGH (~3.3V, button opens, pull-up takes over) |
| Wire disconnected | PC13 = HIGH (fail-safe: same as pressed) |

### 4.5 Wire & Verify Watchdogs (One at a Time)

For each ECU, solder TPS3823 onto SOT-23-5 breakout board:

```
Pin 1 (VDD) → 3.3V + 100nF cap
Pin 2 (GND) → GND
Pin 3 (WDI) → WDT GPIO (see table below)
Pin 4 (RESET) → MCU NRST pin + 100nF cap
Pin 5 (CT) → 100nF cap → GND (sets 1.6s timeout)
Pin 6 (MR) → 3.3V (no manual reset)
```

| ECU | WDI GPIO | MCU NRST Location |
|-----|----------|-------------------|
| CVC | PB0 (CN10-31) | CN7 pin 14 (NRST) |
| FZC | PB0 (CN10-31) | CN7 pin 14 (NRST) |
| RZC | **PB4** (CN10-27) — NOT PB0! | CN7 pin 14 (NRST) |
| SC | GIO_A5 (J3-6) | nRST on LaunchPad |

**Verify each watchdog:**
1. Power up ECU with no firmware toggling WDI → MCU should reset every ~1.6s (oscilloscope on NRST shows periodic low pulses)
2. Flash firmware that toggles WDI every 500ms → MCU stays running, no resets
3. Flash firmware that hangs after 5 seconds → watchdog resets MCU at ~1.6s after hang

---

## Phase 5: Sensors

> **Goal**: Each sensor reads valid data through the MCU.

### 5.1 AS5048A Angle Sensors (CVC: 2 pedal sensors, FZC: 1 steering)

**CVC pedal sensors on SPI1:**
- SCK → PA5 (CN10-11)
- MISO → PA6 (CN10-13)
- MOSI → PA7 (CN10-15)
- CS1 → PA4 (CN7-32) + 10k pull-up to 3.3V
- CS2 → PA15 (CN7-17) + 10k pull-up to 3.3V
- Sensor VDD → external 3.3V + 100nF cap each
- Mount diametric magnet on pedal shaft, align over sensor IC

**FZC steering sensor on SPI2:**
- SCK → PB13 (CN10-30)
- MISO → PB14 (CN10-28)
- MOSI → PB15 (CN10-26)
- CS → PB12 (CN10-16) + 10k pull-up to 3.3V
- Sensor VDD → external 3.3V + 100nF cap

**Verify**: Flash SPI read firmware. Read AS5048A angle register (address 0x3FFF). Rotate magnet — value should change 0-16383 (14-bit) for a full revolution. If stuck at 0 or 0x3FFF → check SPI mode (must be Mode 1: CPOL=0, CPHA=1), check CS wiring, check MISO/MOSI not swapped.

### 5.2 TFMini-S Lidar (FZC)

- VCC (red) → external 5V rail (NOT Nucleo 5V)
- GND (black) → GND
- TX (white, sensor output) → PA3 (CN10-37, USART2_RX on FZC)
- RX (green, sensor input) → PA2 (CN10-35, USART2_TX on FZC)
- **Remember**: SB63 and SB65 must be removed on FZC!

**Verify**: Configure USART2 at 115200 baud, 8N1. TFMini-S sends 9-byte frames at ~100Hz. Parse distance value from bytes 2-3 (little-endian, in cm). Point at wall → reading should match approximate distance.

### 5.3 ACS723 Current Sensor (RZC)

- IP+ / IP- → in series with motor power (or just leave floating for now, reads 0A)
- VCC → 3.3V + 100nF cap
- VIOUT → PA0 (CN7-28, ADC1_IN1) via 1nF + 100nF filter
- GND → analog GND

**Verify**: Read ADC. At 0A current, output should be ~VCC/2 = 1.65V = ADC value ~2048 (12-bit).

### 5.4 NTC Thermistors (RZC)

Wire voltage divider (per `hw-design.md` Section 5.3.4):
- 10k fixed resistor: 3.3V → resistor → junction
- NTC: junction → NTC → GND
- 100nF cap: junction → GND
- ADC: junction → PA1 (motor temp) or PA2 (board temp)

**Verify**: At room temp (~25C), NTC = 10k, divider output = 1.65V, ADC = ~2048.

### 5.5 Battery Voltage Monitor (RZC)

- 47k resistor: 12V rail → junction
- 10k resistor: junction → GND
- 100nF cap: junction → GND
- BZX84C3V3 Zener: junction → GND (protection)
- ADC: junction → PA3 (CN10-37, ADC1_IN4)

**Verify**: At 12V input, divider output = 12 * 10/(47+10) = 2.11V, ADC = ~2612.

### 5.6 OLED Display (CVC)

- SCL → PB8 (CN10-3) + 4.7k pull-up to 3.3V
- SDA → PB9 (CN10-5) + 4.7k pull-up to 3.3V
- VCC → 3.3V + 10uF bulk cap + 100nF
- GND → GND

**Verify**: Flash I2C scan firmware. Should find device at address 0x3C. Then flash SSD1306 driver — display should show text.

---

## Phase 6: Actuators

> **Goal**: Motor spins, servos move. Only after safety chain (Phase 4) is verified!

### 6.1 Wire 6V Regulator for Servos

```
12V Actuator Rail (after kill relay + 30A fuse) → 6V regulator input
6V regulator output → 470uF bulk cap → servo power bus
```

**Verify**: Close kill relay (SC GIO_A0 HIGH), measure 6V regulator output = 5.8-6.2V.

### 6.2 Steering Servo (FZC)

- Signal (orange) → PA0 (CN7-28, TIM2_CH1)
- VCC (red) → 6V regulator output (via 3A fuse)
- GND (brown) → GND

**Verify**: Flash PWM firmware: 50Hz, sweep 1.0ms-2.0ms pulse width. Servo should sweep 0-180 degrees. Center at 1.5ms = 90 degrees.

### 6.3 Brake Servo (FZC)

- Signal → PA1 (CN7-30, TIM2_CH2)
- VCC → 6V (shared bus, via separate 3A fuse)
- GND → GND

**Verify**: Same as steering servo test.

### 6.4 BTS7960 Motor Driver (RZC)

- RPWM → PA8 (CN10-23, TIM1_CH1)
- LPWM → PA9 (CN10-21, TIM1_CH2)
- R_EN → PB0 (CN10-31) + 10k pull-down to GND
- L_EN → PB1 (CN10-24) + 10k pull-down to GND
- B+ → 12V actuator rail (via kill relay + 30A fuse)
- B- → GND (star ground, 16AWG)
- M+ / M- → DC motor terminals

**Verify:**
1. With enables LOW → motor should NOT spin (pull-downs keep it off)
2. Set R_EN HIGH, apply 10% duty on RPWM → motor spins slowly in one direction
3. Set L_EN HIGH, apply 10% duty on LPWM → motor spins other direction
4. Ramp up to 50% duty → motor speeds up
5. Open kill relay → motor stops immediately (12V removed)
6. Check oscilloscope: PWM at 20kHz, clean edges, no ringing

---

## Phase 7: Full Integration & HW-SW Handoff

> **Goal**: Firmware reads all sensors and controls all actuators over CAN. End-to-end pedal-to-motor working.

### 7.1 Flash Real Firmware

Flash the actual ECU firmware (from completed phases 6-9 when ready):
- **CVC**: Pedal reading → CAN TX torque request + vehicle state + E-stop monitoring
- **FZC**: Steering/brake servo control + lidar + buzzer
- **RZC**: Motor control + current/temp monitoring
- **SC**: Heartbeat monitoring → kill relay control + fault LEDs

### 7.2 Integration Test Sequence

| # | Test | Pass Criteria |
|---|------|--------------|
| 1 | CAN heartbeats | All 4 ECUs send heartbeats at 100ms; visible on candump |
| 2 | Pedal → CAN | Push pedal → CVC SPI reads angle → CAN frame 0x101 (TorqueReq) changes |
| 3 | CAN → Motor | TorqueReq arrives at RZC → BTS7960 PWM changes → motor speed matches request |
| 4 | Lidar → CAN | Object in front of TFMini-S → CAN frame with distance updates |
| 5 | Safety heartbeat timeout | Unplug CVC USB → SC detects missing heartbeat → kill relay opens within FTTI |
| 6 | E-stop | Press E-stop → CVC detects → sends emergency CAN → SC opens relay |
| 7 | Watchdog | Flash hang firmware on CVC → TPS3823 resets CVC → SC detects heartbeat gap → relay opens |
| 8 | Fault LEDs | Unplug FZC → SC lights FZC fault LED (GIO_A2) + system LED (GIO_A4) |
| 9 | OLED | Vehicle state displayed on CVC OLED: speed, torque, system status |
| 10 | Pi gateway | Pi candump shows all traffic; MQTT publish to cloud if configured |

### 7.3 HSR Open Items to Close During Bring-Up

| ID | What to Measure | Tool | Document Result In |
|----|----------------|------|--------------------|
| HSR-O-001 | Kill relay dropout time | Oscilloscope (GIO_A0 vs actuator rail) | hw-safety-reqs.md |
| HSR-O-002 | ACS723 sensitivity at room temp | Multimeter + known load | hw-safety-reqs.md |
| HSR-O-005 | TPS3823 POR pulse width on NRST | Oscilloscope (trigger on power-up) | hw-safety-reqs.md |

### 7.4 Endurance Test

Run the full system for 30 minutes:
- Pedal inputs cycling
- Motor running at varying speeds
- Monitor CAN bus for errors (`candump -e`)
- Monitor MCU temperatures via NTC
- Monitor for watchdog resets (should be zero)
- Monitor for CAN bus-off events (should be zero)

---

## Phase Summary & Dependencies

```
Phase 0: Gap Closure          ← DO FIRST (order missing parts, prep boards)
    ↓
Phase 1: Power Distribution   ← Verify all voltages before anything else
    ↓
Phase 2: Mount & USB Boot     ← Confirm all boards are alive
    ↓
Phase 3: CAN Bus              ← Communication backbone
    ↓
Phase 4: Safety Chain          ← MUST pass before Phase 6 (actuators)
    ↓
Phase 5: Sensors              ← Can run in parallel with Phase 4
    ↓
Phase 6: Actuators            ← Only after Phase 4 passes!
    ↓
Phase 7: Full Integration     ← Depends on firmware phases 7-9 completion
```

## Estimated Time

### Experienced Builder

| Phase | Time | Notes |
|-------|------|-------|
| 0 | 1-3 days | Depends on shipping for gap items |
| 1 | 2 hours | Power wiring and verification |
| 2 | 1 hour | Mount boards, USB test |
| 3 | 3 hours | CAN wiring + transceiver soldering + verification |
| 4 | 3 hours | Kill relay, E-stop, watchdogs |
| 5 | 3 hours | All sensors wired and verified |
| 6 | 2 hours | Servos + motor driver |
| 7 | 4 hours | Full integration testing |
| **Total** | **~2-3 days hands-on** | Plus gap closure wait time |

### Solo Builder — Little/No HW Experience (Realistic Estimates)

| Phase | Time | Difficulty | Where You'll Get Stuck |
|-------|------|-----------|----------------------|
| 0 | 1-3 days + 1 evening | Easy | Parts shipping wait. Solder bridge removal is your first soldering task — practice on scrap first |
| 1 | 4-6 hours | Easy-Medium | First time wiring power distribution, double-checking everything with multimeter, re-doing connections |
| 2 | 1-2 hours | Easy | Straightforward USB plug-and-check. Easiest phase — quick confidence win |
| 3 | 8-12 hours | **Hard** | **Hardest phase.** Soldering 4 CAN transceivers to perfboard, running the bus trunk, debugging when candump shows nothing |
| 4 | 6-10 hours | **Hard** | **Second hardest.** Soldering TPS3823 (tiny SOT-23) onto breakout boards. Kill relay MOSFET circuit. Lots of "why isn't this working" |
| 5 | 4-8 hours | Medium | SPI angle sensors and UART lidar are fiddly (mode/baud mismatch). ADC/NTC/voltage dividers are straightforward |
| 6 | 3-4 hours | Easy-Medium | Easy IF Phase 4 works. Just connecting servos and motor driver to tested infrastructure |
| 7 | 6-10 hours | Medium-Hard | Integration bugs. CAN timing issues. Firmware-hardware mismatch debugging |
| **Total** | **~40-60 hours hands-on** | | **~4 weekends** plus gap closure wait |

### Recommended Weekend Schedule

| Weekend | Phases | Goal | What "Done" Looks Like |
|---------|--------|------|----------------------|
| **Weekend 1** | 0 (finish) + 1 + 2 | Power rails verified, all boards boot | Multimeter shows 12V/5V/3.3V correct. All 4 boards blink via USB. Quick win — builds confidence |
| **Weekend 2** | 3 | CAN bus working | `candump` on PC shows heartbeat frames from at least 1 ECU. Then 2, then all 4 |
| **Weekend 3** | 4 + 5 | Safety chain + sensors | Kill relay clicks on command, E-stop cuts power, watchdog resets MCU. Sensors return plausible data |
| **Weekend 4** | 6 + 7 | Actuators + full integration | Motor spins from pedal input. End-to-end pedal→CAN→motor working. 30-min endurance pass |

### The 3 Hardest Parts (Where Beginners Get Stuck)

#### 1. Soldering TPS3823 onto SOT-23 Breakout (Phase 4)
The IC pins are ~0.95mm apart. Your first attempt will probably bridge pins.
- Watch 2-3 YouTube videos on "drag soldering SOT-23" before attempting
- Use flux generously — flux is the difference between success and bridged pins
- Have solder wick ready for cleanup
- Practice on a spare breakout board first (you have 6 spares)

#### 2. CAN Bus Debugging (Phase 3)
When `candump` shows nothing, the possible causes are: TX/RX swapped, termination wrong, bit timing mismatch, transceiver wired wrong, transceiver not powered, stub too long. You'll be checking 6 things before finding the one wrong one.
- Get ONE node working first (CVC → PC via CANable). Don't wire all 4 at once
- Verify 60 ohm termination with multimeter FIRST (power off)
- Check CAN_H and CAN_L rest voltage (both ~2.5V) before expecting traffic

#### 3. Knowing What "Wrong" Looks Like (All Phases)
Experienced people glance at an oscilloscope trace and say "that's ringing" or "that's ground bounce." You'll need to learn what good signals look like.
- Screenshot/photo every "good" measurement as you go
- When something breaks later, you have a reference to compare against
- The verification tables in each phase tell you what "good" numbers are — trust them

### Risk Assessment (Solo Beginner)

| Scenario | Likelihood | Impact | Recovery |
|----------|-----------|--------|----------|
| Bad solder bridge removal on Nucleo SB63/65 | Medium | Low | Fixable with solder wick + patience + flux |
| Burn out a Nucleo (wrong voltage) | Low (if fuse + Schottky steps followed) | Medium | ~€15 replacement |
| CAN bus won't communicate on first attempt | **High** | Low | Systematic debug: termination → wiring → transceiver → firmware bit timing |
| TPS3823 SOT-23 soldering failure | Medium-High | Low | You have 6 spare breakout boards. Just try again |
| Motor/servo smoke | Very Low | Medium | Plan enforces Phase 4 (safety chain) before Phase 6 (actuators) — by design |
| Complete stuck, can't debug | Low-Medium | High | Post on EEVblog forum or r/AskElectronics with photos — community is helpful |

### Essential Tips for HW Beginners

1. **Get a helping hands / PCB holder** (~€10) — soldering while holding the board + wire + iron with 2 hands doesn't work
2. **One change at a time** — wire one thing, test it, then wire the next. Never wire 5 things and then wonder which one is wrong
3. **Take photos before and after** every wiring step — invaluable for debugging and for documenting HSR open items
4. **Label every wire** with masking tape — "12V MAIN", "GND", "CAN_H", "CVC_WDI". Your future self will thank you
5. **Flux is your best friend** — apply it before every solder joint, especially on SOT-23 and perfboard work
6. **When stuck for 30+ minutes on the same problem**: stop, take a photo, describe the problem in writing. Often the act of writing it down reveals the issue
7. **Keep the plan open on a second screen** — check off each verification step as you go

## Safety Rules (Follow These Always)

1. **Never connect actuators (motor, servos) until the kill relay is tested and working**
2. **Always check 12V polarity before connecting a new board**
3. **Always use fuses** — a short circuit without a fuse = fire risk
4. **Start motor at low PWM duty (10%)** — don't jump to 100%
5. **Keep the E-stop within arm's reach** at all times during actuator testing
6. **Disconnect USB from PC before measuring high-current paths** with oscilloscope — ground loop risk
7. **One change at a time** — wire one thing, test it, then wire the next
