---
document_id: ICD
title: "Interface Control Document"
version: "1.0"
status: draft
aspice_process: SYS.3
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


# Interface Control Document

## 1. Purpose and Scope

This document defines all inter-element interfaces for the Taktflow Zonal Vehicle Platform per Automotive SPICE 4.0 SYS.3 (System Architectural Design). It specifies the physical, electrical, protocol, and timing characteristics of every interface between system elements, including CAN bus, SPI, UART, ADC, PWM, GPIO, I2C, MQTT, and the SOME/IP bridge.

This is the authoritative reference for hardware/software interface integration. All firmware driver configurations and hardware interconnections shall comply with the specifications in this document.

## 2. Referenced Documents

| Document ID | Title | Version |
|-------------|-------|---------|
| SYSARCH | System Architecture | 1.0 |
| CAN-MATRIX | CAN Message Matrix | 1.0 |
| HSI | Hardware-Software Interface Specification | 1.0 |
| HSR | Hardware Safety Requirements | 1.0 |
| SWR-CVC | Software Requirements -- CVC | 1.0 |
| SWR-FZC | Software Requirements -- FZC | 1.0 |
| SWR-RZC | Software Requirements -- RZC | 1.0 |
| SWR-SC | Software Requirements -- SC | 1.0 |
| PIN-MAP | Pin Mapping | 0.1 |

## 3. Interface Summary Table

| IF-ID | Interface | Type | Protocol | Elements | Data Rate | ASIL | Direction |
|-------|-----------|------|----------|----------|-----------|------|-----------|
| IF-001 | CAN Bus | Communication | CAN 2.0B, 500 kbps | CVC, FZC, RZC, SC, BCM, ICU, TCU, Pi | 500 kbps | D | Bidirectional (SC listen-only) |
| IF-002 | CVC SPI1 | Sensor | SPI Mode 1, 1 MHz | CVC <-> AS5048A x2 | 1 MHz | D | Bidirectional |
| IF-003 | FZC SPI2 | Sensor | SPI Mode 1, 1 MHz | FZC <-> AS5048A | 1 MHz | D | Bidirectional |
| IF-004 | FZC USART2 | Sensor | UART 115200 8N1 | FZC <-> TFMini-S | 115.2 kbps | C | RX only (MCU receives) |
| IF-005 | RZC ADC1 CH1 | Sensor | Analog 0-3.3V | RZC <-> ACS723 | 1 kHz sample | A | Unidirectional (sensor to MCU) |
| IF-006 | RZC ADC1 CH2 | Sensor | Analog 0-3.3V | RZC <-> NTC winding 1 | 10 Hz sample | A | Unidirectional |
| IF-007 | RZC ADC1 CH3 | Sensor | Analog 0-3.3V | RZC <-> NTC winding 2 | 10 Hz sample | A | Unidirectional |
| IF-008 | RZC ADC1 CH4 | Sensor | Analog 0-3.3V | RZC <-> Battery divider | 10 Hz sample | QM | Unidirectional |
| IF-009 | FZC TIM2 CH1 | Actuator | PWM 50 Hz | FZC -> Steering servo | 50 Hz | D | Unidirectional (MCU to servo) |
| IF-010 | FZC TIM2 CH2 | Actuator | PWM 50 Hz | FZC -> Brake servo | 50 Hz | D | Unidirectional |
| IF-011 | RZC TIM1 CH1/CH2 | Actuator | PWM 20 kHz | RZC -> BTS7960 RPWM/LPWM | 20 kHz | D | Unidirectional |
| IF-012 | CVC GPIO PC13 | Sensor | Digital EXTI | CVC <-> E-stop button | Event | B | Input to MCU |
| IF-013 | SC GIO_A[0] | Actuator | Digital GPIO | SC -> Kill relay | On-demand | D | Output from MCU |
| IF-014 | SC GIO_A[1-3] | Indicator | Digital GPIO | SC -> Fault LEDs | On-demand | C | Output from MCU |
| IF-015 | FZC GPIO PB4 | Actuator | Digital GPIO | FZC -> Buzzer | On-demand | QM | Output from MCU |
| IF-016 | CVC I2C1 | Display | I2C 400 kHz | CVC <-> SSD1306 OLED | 400 kHz | QM | Bidirectional |
| IF-017 | MQTT/TLS | Communication | MQTT v3.1.1/TLS 1.2 | Pi <-> AWS IoT Core | 1 msg/5s | QM | Bidirectional |
| IF-018 | SOME/IP Bridge | Communication | SOME/IP/UDP | Docker ECUs <-> host CAN | Variable | QM | Bidirectional |
| IF-019 | RZC TIM4 CH1/CH2 | Sensor | Quadrature encoder | RZC <-> Motor encoder | Variable | C | Input to MCU |
| IF-020 | RZC GPIO PB0/PB1 | Actuator | Digital GPIO | RZC -> BTS7960 R_EN/L_EN | On-demand | D | Output from MCU |
| IF-021 | All ECUs WDT | Safety | Digital GPIO toggle | ECU <-> TPS3823 | 0.5-1 Hz | D | Output from MCU |
| IF-022 | Pi USB-CAN | Communication | gs_usb / SocketCAN | Pi <-> CANable 2.0 | 500 kbps | QM | Bidirectional |

## 4. CAN Bus Interface (IF-001)

### 4.1 Physical Layer

| Parameter | Value |
|-----------|-------|
| Standard | ISO 11898-1 (data link), ISO 11898-2 (physical) |
| Topology | Linear bus (daisy-chain) |
| Wire type | 22 AWG twisted pair |
| Signal names | CAN_H (dominant high), CAN_L (dominant low) |
| Bus length | < 2 m (bench), max 40 m at 500 kbps |
| Termination | 120 ohm at each physical end |
| Node count | 8 (4 physical + 3 bridged + 1 gateway) |

### 4.2 Electrical Characteristics

| Parameter | Recessive | Dominant |
|-----------|-----------|----------|
| CAN_H voltage | 2.5 V | 3.5 V |
| CAN_L voltage | 2.5 V | 1.5 V |
| Differential voltage | 0 V | 2.0 V |
| Bus impedance | -- | 60 ohm (two 120 ohm in parallel) |
| Common-mode voltage range | -2 V to +7 V | -2 V to +7 V |

### 4.3 Transceiver Configuration

| ECU | Transceiver | VCC | TXD Pin | RXD Pin | STB Pin | Termination |
|-----|-------------|-----|---------|---------|---------|-------------|
| CVC | TJA1051T/3 | 3.3V | PA12 (FDCAN1_TX) | PA11 (FDCAN1_RX) | GPIO (active LOW) | Yes (bus end 1) |
| FZC | TJA1051T/3 | 3.3V | PA12 (FDCAN1_TX) | PA11 (FDCAN1_RX) | GPIO (active LOW) | No (mid-bus) |
| RZC | TJA1051T/3 | 3.3V | PA12 (FDCAN1_TX) | PA11 (FDCAN1_RX) | GPIO (active LOW) | No (mid-bus) |
| SC | SN65HVD230 | 3.3V | DCAN1_TX (edge conn.) | DCAN1_RX (edge conn.) | NC (always active) | Yes (bus end 2) |
| Pi | CANable 2.0 | USB powered | Internal | Internal | N/A | Built-in (switchable) |

### 4.4 STM32 FDCAN1 Controller Configuration

| Parameter | Value |
|-----------|-------|
| Mode | Classic CAN (not CAN FD) |
| Nominal bit rate | 500 kbps |
| Prescaler | 34 (170 MHz / 34 = 5 MHz bit rate clock) |
| Sync segment | 1 Tq |
| Time segment 1 (TSEG1) | 7 Tq (prop + phase1) |
| Time segment 2 (TSEG2) | 2 Tq |
| SJW | 2 Tq |
| Sample point | 80% (8/10 Tq) |
| TX FIFO depth | 3 messages |
| RX FIFO 0 | 3 messages (safety messages) |
| RX FIFO 1 | 3 messages (QM messages) |
| Interrupt | FIFO 0 new message, TX complete, bus-off |
| Error recovery | Automatic retransmission, 3 attempts max on bus-off |

### 4.5 TMS570 DCAN1 Controller Configuration

| Parameter | Value |
|-----------|-------|
| Module | DCAN1 (NOT DCAN4 -- HALCoGen mailbox bug) |
| Mode | Silent (listen-only) via TEST register bit 3 |
| Bit rate | 500 kbps |
| Prescaler | 15 (from 75 MHz VCLK1) |
| TSEG1 | 7 |
| TSEG2 | 2 |
| SJW | 1 |
| Mailboxes | 7 configured (exact-match acceptance masks) |
| Interrupt | New message on mailbox 1-7 |

### 4.6 Bus Protection

| Protection | Implementation |
|------------|---------------|
| Common-mode choke | 100 uH minimum on each node CANH/CANL |
| ESD protection | TVS diodes on CANH and CANL at each node (ISO 7637) |
| Short-circuit tolerance | Transceivers tolerate CANH/CANL short to GND or VBAT |
| Babbling node protection | CAN error counters; bus-off after TEC > 255; 3 recovery attempts |
| Listen-only (SC) | SC cannot transmit; prevents safety monitor from corrupting bus |

### 4.7 CAN Bus Wiring Diagram

```
    120 ohm                                                         120 ohm
   [CVC]---+---[TJA1051T/3]--CAN_H--+--+--+--+--[SN65HVD230]---[SC]
            |                 CAN_L--+--+--+--+                    |
            |                        |  |  |  |                    |
            |                      [FZC][RZC][Pi]                  |
            |                   TJA1051  TJA1051  CANable          |
            |                   (no term)(no term)(built-in        |
            |                                      term off)       |
            +---------- Bus End 1                    Bus End 2 ----+
```

## 5. SPI Interfaces (IF-002, IF-003)

### 5.1 CVC SPI1 -- Dual AS5048A Pedal Sensors (IF-002)

| Parameter | Value |
|-----------|-------|
| Peripheral | SPI1 |
| Mode | Master |
| Clock | 1 MHz (AS5048A max = 10 MHz; derated for signal integrity) |
| CPOL | 0 (clock idle LOW) |
| CPHA | 1 (data sampled on trailing edge) |
| SPI Mode | Mode 1 |
| Data frame | 16-bit |
| Bit order | MSB first |
| NSS management | Software (GPIO chip-selects) |

**Pin Assignment**:

| Signal | Pin | AF | Direction | Pull | Notes |
|--------|-----|-----|-----------|------|-------|
| SPI1_SCK | PA5 | AF5 | Output | -- | Clock to both sensors |
| SPI1_MISO | PA6 | AF5 | Input | Pull-up | Data from sensor |
| SPI1_MOSI | PA7 | AF5 | Output | -- | Command to sensor |
| CS1 (Pedal 1) | PA4 | GPIO | Output | 10k ext. pull-up to 3.3V | Active LOW |
| CS2 (Pedal 2) | PA15 | GPIO | Output | 10k ext. pull-up to 3.3V | Active LOW |

**AS5048A SPI Protocol**:

```
Command Word (16-bit, MCU -> sensor):
  Bit 15:    Parity (even parity over bits 14:0)
  Bit 14:    R/W (1 = read, 0 = write)
  Bits 13:0: Register address

Response Word (16-bit, sensor -> MCU):
  Bit 15:    Parity (even parity over bits 14:0)
  Bit 14:    Error flag (1 = error in previous command)
  Bits 13:0: Data

Angle Register: Address 0x3FFF
  Bits 13:0: 14-bit angle (0-16383 = 0-360 degrees)
  Resolution: 0.022 degrees per count

NOP Register: Address 0x0000
  Used for read-without-command (dummy read)

Diagnostic Register: Address 0x3FFD
  Bits: AGC value, magnitude, CORDIC overflow, parity error
```

**Transaction Sequence (per sensor)**:

```
1. Assert CS_n LOW
2. Wait t_CSn_to_SCLK = 350 ns minimum
3. Transmit 16-bit command (0xFFFF for angle read = parity=1, R=1, addr=0x3FFF)
4. Receive 16-bit response simultaneously (full-duplex)
5. Wait t_SCLK_to_CSn = 50 ns minimum
6. De-assert CS_n HIGH
7. Wait t_CSn_HIGH = 350 ns minimum before next transaction
8. Validate parity: count 1-bits in bits 14:0, check bit 15 matches even parity
9. Check error flag (bit 14): if set, re-read diagnostic register
10. Extract angle: bits 13:0
```

**Timing**:

| Parameter | Value |
|-----------|-------|
| Transaction time per sensor | ~20 us at 1 MHz (16 clocks + setup/hold) |
| Both sensors read time | < 50 us |
| Read cycle | Every 10 ms |
| Timeout per transaction | 500 us |
| Retry on failure | 1 retry per cycle |

### 5.2 FZC SPI2 -- Steering Angle Sensor (IF-003)

| Parameter | Value |
|-----------|-------|
| Peripheral | SPI2 |
| Mode | Master |
| Clock | 1 MHz |
| CPOL / CPHA | 0 / 1 (Mode 1, same as CVC) |
| Data frame | 16-bit, MSB first |

**Pin Assignment**:

| Signal | Pin | AF | Direction | Pull | Notes |
|--------|-----|-----|-----------|------|-------|
| SPI2_SCK | PB13 | AF5 | Output | -- | Clock |
| SPI2_MISO | PB14 | AF5 | Input | Pull-up | Data from sensor |
| SPI2_MOSI | PB15 | AF5 | Output | -- | Command to sensor |
| CS (Steering) | PB12 | GPIO | Output | 10k ext. pull-up to 3.3V | Active LOW |

Protocol, transaction sequence, and timing are identical to CVC SPI1 (Section 5.1), except only one sensor is read per cycle.

### 5.3 SPI Electrical Characteristics

| Parameter | Value |
|-----------|-------|
| Logic levels | 3.3V LVCMOS |
| VOH | >= 2.4 V |
| VOL | <= 0.4 V |
| VIH | >= 2.0 V |
| VIL | <= 0.8 V |
| SCK rise/fall time | < 10 ns |
| Cable length | < 30 cm (SPI not designed for long cables) |
| Decoupling | 100 nF on sensor VDD, within 10 mm of pin |
| ESD protection | TVS on MISO/MOSI/SCK if cable > 15 cm |

## 6. UART Interface (IF-004)

### 6.1 FZC USART2 -- TFMini-S Lidar

| Parameter | Value |
|-----------|-------|
| Peripheral | USART2 |
| Baud rate | 115200 bps |
| Data bits | 8 |
| Parity | None |
| Stop bits | 1 |
| Flow control | None |
| DMA | RX via DMA1 Channel 5 (circular buffer, 18 bytes = 2 frames) |

**Pin Assignment**:

| Signal | Pin | AF | Direction | Pull | Notes |
|--------|-----|-----|-----------|------|-------|
| USART2_TX | PA2 | AF7 | Output | -- | MCU to lidar (config commands) |
| USART2_RX | PA3 | AF7 | Input | Pull-up | Lidar to MCU (data frames) |

**TFMini-S Frame Format (9 bytes)**:

```
Byte 0: 0x59 (header byte 1)
Byte 1: 0x59 (header byte 2)
Byte 2: Distance LSB (cm)
Byte 3: Distance MSB (cm)
Byte 4: Signal strength LSB
Byte 5: Signal strength MSB
Byte 6: Temperature (raw, offset by 10 degrees C relative to actual)
Byte 7: Reserved (0x00)
Byte 8: Checksum (low byte of sum of bytes 0-7)
```

**Data Extraction**:

```
distance_cm = (byte[3] << 8) | byte[2]    // Range: 0-1200 cm
strength    = (byte[5] << 8) | byte[4]    // Higher = stronger signal
temp_C      = byte[6] / 8.0 - 25.6        // Approximate conversion
checksum    = (sum of bytes 0..7) & 0xFF   // Must equal byte[8]
```

**Sensor Specifications**:

| Parameter | Value |
|-----------|-------|
| Range | 0.1 m to 12 m (10 cm to 1200 cm) |
| Accuracy | +/- 1% of distance (at distance > 0.5 m) |
| Update rate | 100 Hz (one frame every 10 ms) |
| Power supply | 5V DC, 100 mA typical |
| Signal logic | 3.3V LVTTL (compatible with STM32 direct connect) |
| FOV | 2.3 degrees |
| Wavelength | 850 nm (infrared) |

**Timing**:

| Parameter | Value |
|-----------|-------|
| Frame period | 10 ms (100 Hz) |
| Frame duration | 9 bytes * 10 bits / 115200 = 0.78 ms |
| DMA transfer complete interrupt | Every 9 bytes |
| Frame validation timeout | 100 ms (if no valid frame, declare sensor fault) |
| Startup handshake | 3 valid frames within 500 ms of UART init |

**Electrical Interface**:

| Parameter | Value |
|-----------|-------|
| Power | 5V from dedicated buck converter (not 3.3V MCU rail) |
| TX/RX logic | 3.3V LVTTL (no level shifter needed) |
| Connector | 4-pin keyed (VCC, GND, TX, RX) to prevent reverse polarity |
| Decoupling | 470 uF bulk + 100 nF on VCC at sensor end |
| Cable length | < 1 m; shielded if > 0.5 m |
| ESD protection | TVS diodes on RX and TX lines |

## 7. ADC Interfaces (IF-005, IF-006, IF-007, IF-008)

### 7.1 RZC ADC1 Configuration

| Parameter | Value |
|-----------|-------|
| ADC peripheral | ADC1 |
| Resolution | 12-bit (0-4095 counts) |
| Reference voltage (VREF+) | 3.3V (internal or stable external) |
| Conversion mode | Timer-triggered scan mode |
| Scan sequence | CH1 -> CH2 -> CH3 -> CH4 |
| DMA | DMA1 transfers results to RAM buffer (circular) |
| Sample time | 47.5 ADC clock cycles (adequate for high-impedance NTC) |
| ADC clock | 42.5 MHz (PCLK2 / 4) |

### 7.2 ACS723 Motor Current Sensor (IF-005)

| Parameter | Value |
|-----------|-------|
| ADC channel | ADC1_IN1 |
| Pin | PA0 |
| Sensor | ACS723LLCTR-20AB-T |
| Sensing range | +/- 20 A (bidirectional) |
| Sensitivity | 100 mV/A (ACS723LLCTR-20AB-T, 20A variant) |
| Zero-current output | VCC/2 = 1.65 V (at 3.3V supply) |
| Zero-current ADC count | ~2048 (at 12-bit, 3.3V VREF) |
| Full-scale positive (20A) | 1.65 + (20 * 0.1) = 3.65 V (clamped to 3.3V by ADC) |
| Usable range at 3.3V | +/- 16.5 A linear; clipped above 16.5 A |
| Note | At 100 mV/A, output spans 0V (-16.5A) to 3.3V (+16.5A) at 3.3V VCC |
| Bandwidth | 80 kHz (-3 dB) with 1 nF output capacitor |
| Galvanic isolation | 2.1 kV RMS (Hall-effect, no copper path to ADC) |
| Sample rate | 1 kHz (timer-triggered) |
| Filter | 4-sample moving average in software |
| Decoupling | 100 nF on VCC, 1 nF on output |

**Conversion Formula**:

```
current_mA = ((adc_value - adc_zero_offset) * Vref_mV) / (4096 * sensitivity_mV_per_A) * 1000

Where:
  adc_zero_offset = calibrated at startup (nominal: 2048)
  Vref_mV = 3300
  sensitivity_mV_per_A = 100

Simplified: current_mA = (adc_value - 2048) * 3300 / (4096 * 0.1)
           current_mA = (adc_value - 2048) * 8.0566
```

**Overcurrent Detection**:

| Threshold | ADC Count | Action |
|-----------|-----------|--------|
| 25 A (software, calibratable) | > 2048 + 2480 = 4528 (clipped to 4095) | Motor disable after 10 ms debounce |
| 43 A (BTS7960 hardware limit) | N/A (BTS7960 internal) | Hardware cutoff, immediate |

### 7.3 NTC Motor Temperature Sensor (IF-006)

| Parameter | Value |
|-----------|-------|
| ADC channel | ADC1_IN2 |
| Pin | PA1 |
| NTC type | 10 kohm at 25 degC, B=3950 |
| Pull-up resistor | 10 kohm (1% precision) to 3.3V |
| Circuit | Voltage divider: VCC -- [R_pullup] -- ADC_pin -- [R_NTC] -- GND |
| Sample rate | 10 Hz (100 ms period) |
| Filter | 100 nF capacitor on ADC input |

**Conversion Formula**:

```
R_ntc = R_pullup * adc_value / (4095 - adc_value)

T_kelvin = 1.0 / ( (1.0/T0) + (1.0/B) * ln(R_ntc/R0) )
T_celsius = T_kelvin - 273.15

Where:
  T0 = 298.15 K (25 degC)
  R0 = 10000 ohm
  B  = 3950 (beta parameter)
  R_pullup = 10000 ohm
```

**Temperature vs ADC Lookup (for verification)**:

| Temperature (degC) | R_NTC (ohm) | ADC Value | Voltage (V) |
|--------------------|-------------|-----------|-------------|
| -30 | 167,700 | 3863 | 3.11 |
| 0 | 32,650 | 3179 | 2.56 |
| 25 | 10,000 | 2048 | 1.65 |
| 60 | 2,488 | 816 | 0.66 |
| 80 | 1,256 | 457 | 0.37 |
| 100 | 680 | 261 | 0.21 |
| 150 | 156 | 63 | 0.05 |

**Fault Detection**:

| Fault | ADC Condition | Temperature Equivalent | Action |
|-------|--------------|----------------------|--------|
| Open circuit (wire break) | ADC >= 4000 | Below -30 degC | Sensor fault, motor disable |
| Short circuit | ADC <= 50 | Above 150 degC | Sensor fault, motor disable |
| Normal range | 63 to 3863 | -30 to 150 degC | Normal operation |

### 7.4 NTC Board Temperature Sensor (IF-007)

| Parameter | Value |
|-----------|-------|
| ADC channel | ADC1_IN3 |
| Pin | PA2 |
| Circuit | Identical to IF-006 (10k NTC, 10k pull-up, 100 nF filter) |
| Sample rate | 10 Hz |
| Purpose | Secondary temperature measurement (board/ambient) |

Conversion and fault detection are identical to IF-006. This sensor provides a secondary temperature data point for derating logic.

### 7.5 Battery Voltage Divider (IF-008)

| Parameter | Value |
|-----------|-------|
| ADC channel | ADC1_IN4 |
| Pin | PA3 |
| Divider ratio | R_high = 47 kohm, R_low = 10 kohm |
| Input range | 0-18.8 V (battery) |
| ADC range | 0-3.3 V (clamped to 3.3V by Zener) |
| Zener protection | 3.3V Zener diode across ADC input |
| Filter | 100 nF capacitor on ADC input |
| Sample rate | 10 Hz |

**Conversion Formula**:

```
V_battery_mV = adc_value * Vref_mV / 4096 * (R_high + R_low) / R_low
V_battery_mV = adc_value * 3300 / 4096 * (47000 + 10000) / 10000
V_battery_mV = adc_value * 3300 / 4096 * 5.7
V_battery_mV = adc_value * 4.592
```

**Voltage Thresholds**:

| Condition | Battery V | ADC Value | Action |
|-----------|-----------|-----------|--------|
| Critical undervoltage | < 8.0 V | < 1741 | Motor disable, DTC |
| Undervoltage warning | < 10.5 V | < 2286 | DTC, heartbeat warning |
| Normal | 10.5-15.0 V | 2286-3267 | Normal operation |
| Overvoltage warning | > 15.0 V | > 3267 | DTC, heartbeat warning |
| Critical overvoltage | > 17.0 V | > 3702 | Motor disable, DTC |

Note: With the 47k/10k divider, the full ADC range (0-4095) corresponds to 0-18.8V. Overvoltage up to 18.8V is detectable before the Zener clamp activates. The Zener diode provides hardware protection for the MCU above 18.8V.

## 8. PWM Interfaces (IF-009, IF-010, IF-011)

### 8.1 FZC Steering Servo PWM (IF-009)

| Parameter | Value |
|-----------|-------|
| Timer | TIM2 |
| Channel | CH1 |
| Pin | PA0 (AF1) |
| Frequency | 50 Hz (20 ms period) |
| Prescaler | (SystemCoreClock / 1,000,000) - 1 (for 1 us resolution) |
| ARR | 19999 (20,000 us period) |
| Pulse range | 1000-2000 us (5%-10% duty) |
| Center position | 1500 us (7.5% duty) |
| Resolution | 1 us (0.09 degrees per count) |

**Angle-to-Pulse Mapping**:

```
pulse_us = 1500 + (angle_deg * 500 / 45)

Where:
  angle_deg: -45.0 to +45.0 degrees
  pulse_us:  1000 to 2000 us
  Center (0 deg): 1500 us

TIM2_CCR1 = pulse_us  (with 1 us timer resolution)
```

**Safety Limits**:

| Parameter | Value |
|-----------|-------|
| Software angle limit | +/- 43 degrees (2-degree margin from mechanical stop) |
| Software rate limit | 0.3 deg per 10 ms cycle (30 deg/s) |
| Three-level disable | 1. CCR1 = 0, 2. CCER OC1E = 0, 3. GPIO forced LOW |
| Return-to-center rate | 30 deg/s toward 0 degrees |

### 8.2 FZC Brake Servo PWM (IF-010)

| Parameter | Value |
|-----------|-------|
| Timer | TIM2 |
| Channel | CH2 |
| Pin | PA1 (AF1) |
| Frequency | 50 Hz (20 ms period) |
| Prescaler | Same as CH1 (shared timer base) |
| ARR | 19999 |
| Pulse range | 1000-2000 us |
| Release position | 1000 us (0% braking) |
| Full brake | 2000 us (100% braking) |

**Brake-to-Pulse Mapping**:

```
pulse_us = 1000 + (brake_percent * 10)

Where:
  brake_percent: 0 to 100
  pulse_us: 1000 to 2000 us

TIM2_CCR2 = pulse_us
```

**Auto-Brake on CAN Timeout**: If no valid brake command is received for 100 ms, the FZC commands 100% braking (CCR2 = 2000).

### 8.3 RZC Motor Driver PWM (IF-011)

| Parameter | Value |
|-----------|-------|
| Timer | TIM1 |
| Channels | CH1 (PA8, RPWM forward) and CH2 (PA9, LPWM reverse) |
| Frequency | 20 kHz (50 us period) |
| Prescaler | 0 (no prescaler; full timer clock) |
| ARR | (SystemCoreClock / 20000) - 1 = 8499 (at 170 MHz) |
| Duty range | 0-95% (max duty limited to avoid BTS7960 bootstrap issues) |
| Resolution | 1/8500 = 0.012% duty cycle per count |
| Dead-time | 10 us minimum (software-enforced direction change dead-time) |

**Direction Control**:

```
Forward:  TIM1_CCR1 = duty (RPWM active), TIM1_CCR2 = 0 (LPWM off)
Reverse:  TIM1_CCR1 = 0 (RPWM off), TIM1_CCR2 = duty (LPWM active)
Stop:     TIM1_CCR1 = 0, TIM1_CCR2 = 0

Direction change sequence:
  1. Set both CCR1 and CCR2 to 0
  2. Wait 10 us (timer-verified dead-time)
  3. Set new direction channel to commanded duty
```

**BTS7960 Enable Lines** (IF-020):

| Signal | Pin | State | Meaning |
|--------|-----|-------|---------|
| R_EN | PB0 | LOW | Right half-bridge disabled (safe state) |
| R_EN | PB0 | HIGH | Right half-bridge enabled |
| L_EN | PB1 | LOW | Left half-bridge disabled (safe state) |
| L_EN | PB1 | HIGH | Left half-bridge enabled |

Note: R_EN and L_EN are driven by separate GPIO pins (PB0, PB1) per the pin mapping. 10k pull-down resistors on both PB0 and PB1 ensure the motor driver defaults to disabled on MCU reset.

**Motor Disable Sequence**:

```
1. TIM1_CCR1 = 0  (RPWM off)
2. TIM1_CCR2 = 0  (LPWM off)
3. PB0 = LOW      (R_EN disabled)
4. PB1 = LOW      (L_EN disabled)
```

## 9. GPIO Interfaces

### 9.1 E-Stop Input (IF-012)

| Parameter | Value |
|-----------|-------|
| Pin | PC13 |
| Mode | Input, EXTI (external interrupt) |
| Trigger | Rising edge (NC button opens = pull-up drives HIGH) |
| Pull | Internal pull-up enabled |
| Debounce | Hardware: 10k series R + 100 nF to GND (RC = 1 ms) |
| | Software: 1 ms timer re-read confirmation |
| ESD | TVS diode (3.3V bidirectional) on PC13 |
| NVIC priority | 0 (highest, non-preemptible) |

**E-Stop Button Wiring (Normally-Closed)**:

```
3.3V --[10k internal pull-up]-- PC13 --[10k series R]--+--[NC Button]-- GND
                                                        |
                                                      [100nF]
                                                        |
                                                       GND

Normal state (button NOT pressed): NC contact closed, PC13 = LOW
Button pressed: NC contact opens, pull-up drives PC13 = HIGH
Wire break / disconnection: PC13 floats HIGH = same as button pressed (fail-safe)
```

**Note on polarity**: The system architecture originally described E-stop as "active low" which assumed a normally-open button. With the NC fail-safe wiring from HSR-CVC-003, the interrupt should be configured for the rising edge (PC13 going HIGH when button pressed or wire broken). The ISR shall confirm the HIGH state after debounce. This fail-safe design means any wire break triggers E-stop.

### 9.2 Kill Relay Output (IF-013)

| Parameter | Value |
|-----------|-------|
| Pin | SC GIO_A[0] |
| Mode | Output, push-pull |
| Initial state | LOW (relay de-energized, safe state) |
| Drive circuit | GIO_A[0] -> 100 ohm -> IRLZ44N gate |
| | IRLZ44N drain -> relay coil low side |
| | IRLZ44N source -> GND |
| | 10k pull-down on gate (default OFF) |
| | 1N4007 flyback diode across relay coil |
| Relay | 30A automotive, SPST-NO, 12V coil, energize-to-run |
| Energize delay | < 10 ms (relay contact close time) |
| De-energize delay | < 10 ms (relay contact open time) |
| GPIO readback | GIO_A[0] data-in register, verified every 10 ms |

**Kill Relay Drive Circuit**:

```
+12V (always live, NOT through kill relay)
  |
  +--[Relay Coil]--+
  |                |
  | [1N4007 diode] | (cathode to +12V)
  |                |
  +--[IRLZ44N drain]
     |
     [IRLZ44N source] -- GND
     |
     [IRLZ44N gate] --[100 ohm]-- GIO_A[0]
                       |
                     [10k] -- GND  (default OFF pull-down)

Relay Contact:
  +12V (from supply) --[Relay Contact NO]-- 12V Actuator Rail
                                               |
                                         BTS7960, Servos
```

### 9.3 Fault LED Outputs (IF-014)

| Pin | LED Color | Function | Series R | Current |
|-----|-----------|----------|----------|---------|
| GIO_A[1] | Red | CVC fault | 330 ohm | ~10 mA at 3.3V |
| GIO_A[2] | Red | FZC fault | 330 ohm | ~10 mA |
| GIO_A[3] | Red | RZC fault | 330 ohm | ~10 mA |
| GIO_A[4] (or GIO_B[1]) | Amber | System fault | 330 ohm | ~10 mA |

All LED outputs are push-pull, initially LOW (LEDs off). TMS570 GIO pins can source 10 mA directly (no MOSFET driver needed).

### 9.4 Buzzer Output (IF-015)

| Parameter | Value |
|-----------|-------|
| Pin | PB4 |
| Mode | Output, push-pull |
| Drive circuit | PB4 -> 2N7002 gate, drain to buzzer, buzzer to 3.3V |
| | 10k pull-down on gate (buzzer OFF on reset) |
| | Schottky flyback diode across buzzer |
| Buzzer type | Active piezo, self-oscillating, > 85 dB at 10 cm |
| Initial state | LOW (buzzer off) |

### 9.5 External Watchdog Interfaces (IF-021)

Each physical ECU has an identical TPS3823 external watchdog connection:

| ECU | WDI Pin | RESET Connection | Timeout |
|-----|---------|-----------------|---------|
| CVC | PB0 | STM32 NRST via 100 nF RC | 1.6 s (+/- 20%) |
| FZC | PB0 | STM32 NRST via 100 nF RC | 1.6 s (+/- 20%) |
| RZC | PB4 | STM32 NRST via 100 nF RC | 1.6 s (+/- 20%) |
| SC | GIO_A[4] | TMS570 nRST via 100 nF RC | 1.6 s (+/- 20%) |

**Feed Protocol**: Software toggles WDI pin (alternating HIGH/LOW) once per main loop iteration. If the main loop hangs and no toggle occurs within 1.6 seconds, the TPS3823 asserts RESET (active low, open-drain) and resets the MCU.

### 9.6 BTS7960 Enable Lines (IF-020)

| Parameter | Value |
|-----------|-------|
| R_EN Pin | PB0 (drives BTS7960 R_EN) |
| L_EN Pin | PB1 (drives BTS7960 L_EN) |
| Mode | Output, push-pull |
| External pull-down | 10k ohm to GND on each pin (ensures disabled on MCU reset) |
| HIGH | Half-bridge enabled |
| LOW | Half-bridge disabled (safe state) |

## 10. I2C Interface (IF-016)

### 10.1 CVC I2C1 -- SSD1306 OLED Display

| Parameter | Value |
|-----------|-------|
| Peripheral | I2C1 |
| Mode | Master |
| Speed | 400 kHz (Fast Mode) |
| Addressing | 7-bit, address 0x3C |
| Pull-ups | 4.7k ohm on SCL and SDA to 3.3V |

**Pin Assignment**:

| Signal | Pin | AF | Direction |
|--------|-----|-----|-----------|
| I2C1_SCL | PB8 | AF4 | Bidirectional (open-drain) |
| I2C1_SDA | PB9 | AF4 | Bidirectional (open-drain) |

**SSD1306 Display Specifications**:

| Parameter | Value |
|-----------|-------|
| Resolution | 128 x 64 pixels |
| Controller | SSD1306 |
| Interface | I2C (selected by hardware pin) |
| Address | 0x3C (SA0 pin = LOW) |
| VCC | 3.3V (with internal charge pump for OLED VCC) |
| Initialization sequence | Display OFF, Multiplex 63, Offset 0, Start line 0, Charge pump ON, Address mode horizontal, Segment remap, COM scan direction reversed, COM pins 0x12, Contrast 0x7F, Pre-charge 0x22, VCOMH 0x20, Entire display ON (resume), Normal display, Display ON |
| Update rate | 5 Hz (200 ms period, QM priority) |
| Frame buffer | 128 * 64 / 8 = 1024 bytes |

**I2C Write Transaction**:

```
START -> 0x3C (write) -> ACK -> Control byte (0x00 = command, 0x40 = data) -> ACK -> data bytes... -> STOP
```

**Failure Handling**: If I2C NACK is received (display disconnected), the software logs a QM DTC and continues without display. Display failure does not affect safety-critical functions.

## 11. MQTT Interface (IF-017)

### 11.1 MQTT Connection

| Parameter | Value |
|-----------|-------|
| Broker | AWS IoT Core (endpoint from device certificate) |
| Protocol | MQTT v3.1.1 |
| Transport | TLS 1.2, port 8883 |
| Authentication | X.509 client certificate (device-specific) |
| Client ID | `taktflow-gateway-001` |
| Keep-alive | 300 seconds |
| Clean session | True |
| Library | paho-mqtt (Python) on Raspberry Pi |

### 11.2 MQTT Topics

| Topic | Direction | QoS | Payload Format | Rate |
|-------|-----------|-----|---------------|------|
| `vehicle/telemetry` | Pi -> Cloud | 1 | JSON | 1 msg / 5 sec (batched) |
| `vehicle/dtc/new` | Pi -> Cloud | 1 | JSON | Event-driven |
| `vehicle/dtc/soft` | Pi -> Cloud | 1 | JSON | Event-driven |
| `vehicle/alerts` | Pi -> Cloud | 1 | JSON | Event-driven |
| `vehicle/config` | Cloud -> Pi | 1 | JSON | On-demand |

### 11.3 Telemetry Payload Schema

```json
{
  "timestamp": "2026-02-21T14:30:00Z",
  "device_id": "taktflow-gateway-001",
  "vehicle_state": 1,
  "motor": {
    "speed_rpm": 1200,
    "current_mA": 5400,
    "temperature_C": 45,
    "derating_pct": 100,
    "direction": "forward",
    "enabled": true
  },
  "steering": {
    "angle_deg": 12.5,
    "commanded_deg": 12.5,
    "mode": "normal"
  },
  "brake": {
    "position_pct": 0,
    "mode": "release"
  },
  "lidar": {
    "distance_cm": 350,
    "strength": 2500,
    "zone": "clear"
  },
  "battery": {
    "voltage_mV": 12400,
    "soc_pct": 95,
    "status": "normal"
  },
  "heartbeats": {
    "cvc": true,
    "fzc": true,
    "rzc": true
  },
  "ml": {
    "motor_health_score": 0.92,
    "anomaly_score": 0.03
  }
}
```

### 11.4 DTC Payload Schema

```json
{
  "timestamp": "2026-02-21T14:31:05Z",
  "device_id": "taktflow-gateway-001",
  "dtc_number": "P0A40",
  "dtc_status": 25,
  "ecu_source": "RZC",
  "occurrence_count": 1,
  "freeze_frame": {
    "vehicle_state": 1,
    "motor_current_mA": 26000,
    "motor_temp_C": 85,
    "battery_mV": 12100,
    "torque_request_pct": 80
  }
}
```

### 11.5 AWS IoT Core Integration

| Parameter | Value |
|-----------|-------|
| Region | us-east-1 (configurable) |
| Thing name | `taktflow-vehicle-001` |
| Certificate type | X.509 RSA 2048-bit |
| Certificate files | `device.cert.pem`, `device.private.key`, `AmazonRootCA1.pem` |
| IoT Rule | Route `vehicle/telemetry` to Timestream, `vehicle/dtc/*` to Timestream + Lambda |
| Timestream database | `taktflow_vehicle` |
| Timestream table | `telemetry`, `dtc_events` |
| Grafana | Timestream data source, real-time dashboards |

### 11.6 Batching Strategy

| Parameter | Value |
|-----------|-------|
| Batch window | 5 seconds |
| Messages per window | 1 (aggregated from all CAN data) |
| Max payload size | < 4 KB |
| AWS free tier limit | 500,000 messages/month |
| Budget at 1/5s | ~518,400 msg/month (slight overage; reduce to 1/6s if needed) |

## 12. SOME/IP Bridge (IF-018)

### 12.1 Architecture

The 3 simulated ECUs (BCM, ICU, TCU) run as Docker containers on the development PC. They communicate via the AUTOSAR-like BSW stack with a POSIX SocketCAN backend (`Can_Posix.c`). Two bridge modes are supported:

**Mode A: vCAN (CI/CD testing)**:

```
Docker ECU -> SocketCAN (vcan0) -> Linux vCAN kernel module -> Docker ECU
```

No physical CAN hardware required. All simulated ECUs share the same virtual CAN bus.

**Mode B: CAN Bridge (HIL integration)**:

```
Physical ECUs -> CAN bus -> CANable 2.0 (USB-CAN) -> PC SocketCAN (can0) -> Docker ECU
```

The CANable 2.0 bridges physical CAN frames to the SocketCAN interface, making them available to Docker containers.

### 12.2 vsomeip Configuration (Supplementary)

| Parameter | Value |
|-----------|-------|
| Implementation | vsomeip (BMW open-source) |
| Transport | UDP on Docker network (172.20.0.0/24) |
| Service Discovery | SOME/IP-SD, multicast 224.244.224.245:30490 |
| Services | BCM LightService (0x1001), ICU DisplayService (0x1002) |

The SOME/IP layer is supplementary and does not carry safety-critical data. All safety communication uses CAN.

## 13. USB-CAN Interface (IF-022)

### 13.1 Pi CANable 2.0 Interface

| Parameter | Value |
|-----------|-------|
| Device | CANable 2.0 |
| Firmware | candleLight (gs_usb driver) |
| Connection | USB to Raspberry Pi 4 |
| Linux interface | can0 (SocketCAN) |
| Bit rate | 500 kbps |
| Mode | Listen-only (Pi does not transmit on CAN; fault injection via separate interface) |

**Setup Commands**:

```bash
sudo ip link set can0 type can bitrate 500000
sudo ip link set can0 up
# Verify: candump can0
```

### 13.2 CAN Listener (Python)

| Parameter | Value |
|-----------|-------|
| Library | python-can |
| Interface | socketcan |
| Channel | can0 |
| Filters | None (receive all messages) |
| Decode | Signal extraction per CAN-MATRIX document |
| Thread | Dedicated receive thread, signals main thread via queue |

## 14. Quadrature Encoder Interface (IF-019)

### 14.1 RZC TIM4 Encoder Mode

| Parameter | Value |
|-----------|-------|
| Timer | TIM4 |
| Mode | Encoder mode (both edges, TI1 and TI2) |
| Channels | CH1 (PB6, encoder A) and CH2 (PB7, encoder B) |
| Input filter | ICF = 0x0F (maximum hardware noise rejection) |
| Counter direction | Up = forward, Down = reverse |
| Pull-ups | 10k ohm on PB6 and PB7 (for open-collector encoder outputs) |
| ESD protection | TVS diodes on both inputs |

**Speed Calculation**:

```
delta_counts = TIM4->CNT - previous_count    // 16-bit counter, handle overflow
RPM = (delta_counts * 60) / (pulses_per_rev * sample_period_s)

Where:
  pulses_per_rev = 360 (calibratable, 4x counting mode)
  sample_period_s = 0.01 (10 ms)
  RPM = delta_counts * 60 / (360 * 0.01) = delta_counts * 16.667
```

**Direction**: Determined by TIM4 CR1 DIR bit (0 = counting up = forward, 1 = counting down = reverse).

## 15. Interface Timing Summary

| Interface | Latency Budget | Measured/Expected | Margin |
|-----------|---------------|-------------------|--------|
| E-stop GPIO to ISR | < 1 ms | ~10 us (EXTI latency) | > 99% |
| SPI pedal read (both sensors) | < 500 us | ~50 us | > 90% |
| SPI steering read | < 500 us | ~25 us | > 95% |
| UART lidar frame | < 10 ms | 0.78 ms (frame) + DMA | > 90% |
| ADC scan (4 channels) | < 1 ms | ~20 us (12-bit, DMA) | > 95% |
| CAN TX (single frame) | < 1 ms | 0.27 ms (worst-case arbitration) | > 70% |
| I2C OLED full update | < 50 ms | ~30 ms (1024 bytes at 400 kHz) | 40% |
| Kill relay response | < 16 ms | < 10 ms (GIO + MOSFET + relay) | > 35% |
| MQTT publish | < 5 s | ~100 ms (network + TLS) | > 95% |

## 16. Traceability

| SYS Requirement | Interface IDs |
|----------------|--------------|
| SYS-001 (Dual pedal) | IF-002 |
| SYS-005 (Motor current) | IF-005 |
| SYS-006 (Motor temp) | IF-006, IF-007 |
| SYS-008 (Battery voltage) | IF-008 |
| SYS-009 (Encoder) | IF-019 |
| SYS-010 (Steering command) | IF-009, IF-001 |
| SYS-011 (Steering feedback) | IF-003 |
| SYS-014 (Brake command) | IF-010, IF-001 |
| SYS-018 (Lidar) | IF-004 |
| SYS-024 (Kill relay) | IF-013 |
| SYS-025 (SC listen-only) | IF-001 |
| SYS-027 (Ext. watchdog) | IF-021 |
| SYS-028 (E-stop) | IF-012 |
| SYS-031 (CAN bus) | IF-001 |
| SYS-042 (MQTT telemetry) | IF-017 |
| SYS-044 (OLED display) | IF-016 |
| SYS-045 (Buzzer) | IF-015 |
| SYS-046 (Fault LEDs) | IF-014 |
| SYS-047 (SPI sensors) | IF-002, IF-003 |
| SYS-048 (UART lidar) | IF-004 |
| SYS-049 (ADC interfaces) | IF-005, IF-006, IF-007, IF-008 |
| SYS-050 (PWM interfaces) | IF-009, IF-010, IF-011 |

## 17. Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 0.1 | 2026-02-21 | System | Initial stub (planned status) |
| 1.0 | 2026-02-21 | System | Complete ICD: 22 interfaces defined (CAN, SPI x2, UART, ADC x4, PWM x3, GPIO x5, I2C, MQTT, SOME/IP, USB-CAN, encoder), full electrical characteristics, protocol details, timing analysis, traceability |

