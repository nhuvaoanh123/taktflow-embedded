---
document_id: ITEM-DEF
title: "Item Definition"
version: "1.0"
status: draft
iso_26262_part: 3
iso_26262_clause: "5"
aspice_process: SYS.1
date: 2026-02-21
---

# Item Definition

<!-- DECISION: ADR-001 — Zonal architecture over domain-based -->

## 1. Item Description

### 1.1 System Name

**Taktflow Zonal Vehicle Platform**

### 1.2 Purpose

The item is a zonal vehicle platform implementing drive-by-wire controls for a small electric vehicle. It processes driver inputs (pedal, steering, braking, emergency stop) and controls actuators (motor, servos) with real-time safety monitoring, diagnostics, and cloud telemetry.

### 1.3 Intended Use

The platform serves as a functional safety portfolio demonstrating ISO 26262 ASIL D engineering across the full safety lifecycle. Although designed as an indoor demonstration platform, all safety analysis is performed assuming the system controls a real vehicle to demonstrate competence with the full standard. The platform implements all safety mechanisms that would be required in a production vehicle.

### 1.4 Operational Modes

| Mode | Description | Trigger |
|------|-------------|---------|
| OFF | All ECUs powered down, kill relay open | Ignition off |
| INIT | BSW initialization, self-test, CAN bus startup | Ignition on |
| RUN | Normal operation, all functions active | Self-test passed |
| DEGRADED | Reduced performance, non-critical fault present | Sensor fault, minor CAN error |
| LIMP | Minimal function, speed/torque limited | Dual sensor fault, repeated errors |
| SAFE_STOP | Controlled shutdown, motor off, brakes applied | Critical fault, safety goal violation |
| SHUTDOWN | Orderly power-down, DTC persistence, relay open | Operator command, E-stop release |

## 2. Functional Description

The item provides the following vehicle-level functions:

### 2.1 Drive-by-Wire (F-DBW)

The driver commands vehicle speed via dual pedal sensors. The Central Vehicle Computer (CVC) reads both sensors, performs plausibility checks, and transmits torque requests to the Rear Zone Controller (RZC) over CAN. The RZC controls the motor via an H-bridge driver, with current and temperature monitoring for protection.

- **Input**: Dual AS5048A magnetic angle sensors (SPI) — redundant pedal position
- **Processing**: Plausibility check (|S1 - S2| < threshold), torque mapping, ramp limiting
- **Output**: PWM to BTS7960 H-bridge → 12V brushed DC motor
- **Safety**: Dual sensor disagreement triggers limp mode or safe stop

### 2.2 Steering Control (F-STR)

The CVC transmits steering angle commands to the Front Zone Controller (FZC), which drives a steering servo to the target angle. A steering angle sensor provides position feedback for closed-loop control.

- **Input**: CAN steering command from CVC, AS5048A angle feedback (SPI)
- **Processing**: Angle limiting, rate limiting, closed-loop position control
- **Output**: PWM to metal gear servo
- **Safety**: Return-to-center on sensor fault, servo authority limits

### 2.3 Braking Control (F-BRK)

The CVC transmits brake commands to the FZC, which drives a brake servo to apply braking force. Emergency braking is triggered autonomously by the lidar distance sensing system.

- **Input**: CAN brake command from CVC, lidar emergency brake trigger
- **Processing**: Brake force mapping, auto-brake on CAN timeout
- **Output**: PWM to metal gear servo (brake actuator)
- **Safety**: Auto-brake on communication loss, lidar emergency brake override

### 2.4 Distance Sensing (F-DIST)

A forward-facing TFMini-S lidar sensor measures distance to obstacles. The FZC processes distance data and triggers graduated responses: warning, controlled braking, emergency stop.

- **Input**: TFMini-S lidar via UART (100 Hz, 0.1–12 m range)
- **Processing**: Distance filtering, threshold comparison (warning/brake/emergency)
- **Output**: CAN distance report, emergency brake request, buzzer warning
- **Safety**: Stuck sensor detection, timeout monitoring

### 2.5 Safety Monitoring (F-SAF)

An independent Safety Controller (TMS570, different vendor from zone controllers) monitors the entire system. It listens to the CAN bus in silent mode (does not transmit, cannot corrupt bus), checks heartbeats from all zone ECUs, performs cross-plausibility checks, and controls a kill relay to force the system into a safe state.

- **Input**: CAN bus (listen-only mode), heartbeat messages from CVC/FZC/RZC
- **Processing**: Heartbeat timeout detection, cross-plausibility (torque vs. current)
- **Output**: Kill relay control (GPIO → MOSFET → relay), fault LED panel
- **Safety**: Lockstep CPU cores (hardware redundancy), external watchdog, energize-to-run relay pattern

### 2.6 Body Control (F-BODY)

The Body Control Module (simulated, Docker) manages comfort and convenience functions: automatic headlights, turn indicators, hazard lights, and door locks.

- **Input**: CAN messages (vehicle state, speed, brake status, steering angle, emergency flags)
- **Processing**: Auto headlight logic, indicator control, hazard light trigger, auto-lock
- **Output**: CAN light/indicator/lock status messages
- **Safety**: QM rated — no direct safety function

### 2.7 Diagnostics and Telemetry (F-DIAG)

The Instrument Cluster Unit (simulated) displays vehicle status and warnings. The Telematics Control Unit (simulated) implements UDS (ISO 14229) diagnostic services and OBD-II PIDs. A Raspberry Pi gateway provides cloud telemetry and edge ML inference.

- **Input**: All CAN messages, UDS requests (0x7DF/0x7E0–0x7E6)
- **Processing**: DTC storage/retrieval, UDS service dispatch, ML inference
- **Output**: UDS responses, cloud MQTT telemetry, Grafana dashboards
- **Safety**: QM rated — diagnostic function, no direct safety impact

## 3. System Boundary

### 3.1 What Is In Scope (The Item)

```
┌─────────────────────────────────────────────────────────────────────┐
│                     ITEM BOUNDARY                                   │
│                                                                     │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌──────────┐             │
│  │   CVC   │  │   FZC   │  │   RZC   │  │    SC    │  Physical   │
│  │ STM32G4 │  │ STM32G4 │  │ STM32G4 │  │ TMS570   │  ECUs      │
│  └────┬────┘  └────┬────┘  └────┬────┘  └────┬─────┘             │
│       │            │            │            │                     │
│  ═════╪════════════╪════════════╪════════════╪═══ CAN Bus 500kbps │
│       │            │            │            │                     │
│  ┌────┴────┐  ┌────┴────┐  ┌────┴────┐                           │
│  │   BCM   │  │   ICU   │  │   TCU   │  Simulated ECUs           │
│  │ Docker  │  │ Docker  │  │ Docker  │  (vECU / CAN bridge)      │
│  └─────────┘  └─────────┘  └─────────┘                           │
│                                                                     │
│  Sensors: 2× AS5048A, TFMini-S, ACS723, 3× NTC, encoder          │
│  Actuators: DC motor + BTS7960, 2× servo, buzzer, OLED, LEDs     │
│  Safety HW: kill relay, E-stop button, 4× ext. watchdog (TPS3823)│
│  Power: 12V supply, buck converters (5V, 3.3V)                    │
│  CAN transceivers: 4× TJA1051T/3, 1× SN65HVD230                 │
└─────────────────────────────────────────────────────────────────────┘
```

### 3.2 What Is Out of Scope

| Element | Reason |
|---------|--------|
| Raspberry Pi gateway | Edge computing, not safety-critical. No safety function allocated. |
| AWS cloud infrastructure | Telemetry and dashboards only. No vehicle control authority. |
| SAP QM mock | Quality management demonstration. No safety function. |
| Vehicle chassis/frame | Mechanical platform is out of scope for E/E safety analysis. |
| 12V bench power supply | External power source, treated as boundary condition. |
| Development PC | Host for simulated ECUs and debugging tools. |
| Charging system | Not applicable (bench power supply). |

### 3.3 System Boundary Interactions

| Boundary | Direction | Interface | Description |
|----------|-----------|-----------|-------------|
| Driver → CVC | Input | SPI (AS5048A) | Pedal position (dual redundant) |
| Driver → CVC | Input | GPIO (EXTI) | Emergency stop button |
| CVC → Driver | Output | I2C (SSD1306) | OLED status display |
| FZC → Driver | Output | GPIO (piezo) | Audible warning buzzer |
| SC → Driver | Output | GPIO (LEDs) | Fault indicator LED panel |
| Environment → FZC | Input | UART (TFMini-S) | Forward obstacle distance |
| RZC → Vehicle | Output | PWM (BTS7960) | Motor torque |
| FZC → Vehicle | Output | PWM (servo) | Steering angle |
| FZC → Vehicle | Output | PWM (servo) | Brake force |
| SC → Vehicle | Output | GPIO (relay) | Power kill relay (safety) |
| Pi → Cloud | Output | MQTT/TLS | Telemetry data |
| Cloud → Pi | Input | MQTT/TLS | Configuration, commands |

## 4. Complete Interface List

### 4.1 CAN Bus (Primary Inter-ECU Communication)

| Parameter | Value |
|-----------|-------|
| Standard | CAN 2.0B (29-bit extended IDs not used — 11-bit standard) |
| Bit rate | 500 kbps |
| Nodes | 7 (CVC, FZC, RZC, SC, BCM, ICU, TCU) + Pi gateway |
| Topology | Linear bus with 120 ohm termination at each end |
| Transceivers | TJA1051T/3 (STM32 ECUs), SN65HVD230 (TMS570) |
| E2E protection | CRC-8 + alive counter + data ID on safety-critical messages |
| Controller | FDCAN in classic mode (STM32), DCAN (TMS570) |

### 4.2 SPI Interfaces

| ECU | Device | SPI Bus | CS Pin | Speed | Data |
|-----|--------|---------|--------|-------|------|
| CVC | AS5048A pedal sensor 1 | SPI1 | PA4 | 10 MHz | 14-bit angle |
| CVC | AS5048A pedal sensor 2 | SPI1 | PA15 | 10 MHz | 14-bit angle |
| FZC | AS5048A steering sensor | SPI2 | PB12 | 10 MHz | 14-bit angle |

### 4.3 UART Interfaces

| ECU | Device | UART | Baud | Data |
|-----|--------|------|------|------|
| FZC | TFMini-S lidar | USART1 | 115200 | Distance (cm), signal strength |

### 4.4 ADC Interfaces

| ECU | Channel | Signal | Range | Resolution |
|-----|---------|--------|-------|------------|
| RZC | ADC1_CH1 | Motor current (ACS723) | 0–3.3V (0–30A) | 12-bit |
| RZC | ADC1_CH2 | Motor temperature (NTC) | 0–3.3V (−40°C to +125°C) | 12-bit |
| RZC | ADC1_CH3 | Winding temperature (NTC) | 0–3.3V | 12-bit |
| RZC | ADC1_CH4 | Battery voltage (divider) | 0–3.3V (0–16V) | 12-bit |

### 4.5 PWM Interfaces

| ECU | Timer | Channel | Signal | Frequency |
|-----|-------|---------|--------|-----------|
| RZC | TIM3 | CH1, CH2 | BTS7960 RPWM/LPWM | 20 kHz |
| FZC | TIM2 | CH1 | Steering servo | 50 Hz (1–2 ms pulse) |
| FZC | TIM2 | CH2 | Brake servo | 50 Hz (1–2 ms pulse) |

### 4.6 GPIO Interfaces

| ECU | Pin | Direction | Signal | Type |
|-----|-----|-----------|--------|------|
| CVC | PC13 | Input | E-stop button (active low, hardware pullup) | EXTI, falling edge |
| CVC | PB0 | Output | Status LED green | Push-pull |
| CVC | PB1 | Output | Status LED red | Push-pull |
| RZC | PA5 | Output | BTS7960 R_EN | Push-pull |
| RZC | PA6 | Output | BTS7960 L_EN | Push-pull |
| SC | GIO_A0 | Output | Kill relay MOSFET gate | Push-pull |
| SC | GIO_A1–A4 | Output | Fault LEDs (CVC, FZC, RZC, system) | Push-pull |
| SC | GIO_B0 | Output | External watchdog toggle (TPS3823) | Push-pull |
| All | varies | Output | External watchdog feed (TPS3823 WDI) | Toggle |

### 4.7 I2C Interfaces

| ECU | Device | I2C Bus | Address | Speed | Data |
|-----|--------|---------|---------|-------|------|
| CVC | SSD1306 OLED 128x64 | I2C1 | 0x3C | 400 kHz | Display buffer |

### 4.8 Cloud Interface (Out of Item Boundary)

| Parameter | Value |
|-----------|-------|
| Protocol | MQTT v3.1.1 over TLS 1.2 (port 8883) |
| Broker | AWS IoT Core |
| Authentication | X.509 client certificates |
| Publish rate | 1 message / 5 seconds (batched) |
| Topics | `vehicle/telemetry`, `vehicle/dtc/new`, `vehicle/dtc/soft`, `vehicle/alerts` |

## 5. Operating Environment

### 5.1 Environmental Conditions

| Parameter | Value | Notes |
|-----------|-------|-------|
| Operating temperature | +15°C to +35°C | Indoor laboratory/demo |
| Storage temperature | −10°C to +50°C | Standard indoor |
| Humidity | 20–80% RH non-condensing | Indoor |
| Altitude | 0–2000 m | Standard |
| Vibration | None (bench-mounted) | Not a vehicular environment |
| EMC | Laboratory levels | No automotive EMC testing required |
| IP rating | Not applicable | Open bench platform |

### 5.2 Power Supply

| Rail | Voltage | Source | Consumers |
|------|---------|--------|-----------|
| 12V | 12V ± 0.5V | Bench power supply (5A) | Motor, relay, servos |
| 5V | 5V ± 0.25V | Buck converter from 12V | TFMini-S lidar, Raspberry Pi |
| 3.3V | 3.3V ± 0.1V | Nucleo onboard / buck | STM32 MCUs, sensors, CAN transceivers |
| 3.3V | 3.3V ± 0.1V | LaunchPad onboard | TMS570 |

### 5.3 Operational Scenarios

The platform is designed to demonstrate 16 scenarios (12 safety + 3 simulated ECU + 1 SAP QM) as defined in the master plan. All scenarios are performed on a stationary bench with a motor driving a flywheel (or free-spinning), servos moving without load, and lidar pointing at a movable target.

## 6. Legal and Regulatory Requirements

| Standard | Requirement | Application |
|----------|-------------|-------------|
| ISO 26262:2018 | ASIL D functional safety | All safety-critical functions (F-DBW, F-STR, F-BRK, F-DIST, F-SAF) |
| MISRA C:2012/2023 | Coding standard | All firmware (mandatory, formal deviation process) |
| Automotive SPICE 4.0 | Process maturity | Level 2 minimum, Level 3 target for ASIL D processes |
| ISO/SAE 21434 | Cybersecurity engineering | CAN bus security, cloud interface security |
| ISO 14229 | Unified Diagnostic Services | TCU diagnostic implementation |
| ISO 15031 | OBD-II | TCU on-board diagnostics |

**Note**: As a portfolio demonstration project, formal certification by a notified body (TUV, SGS) is not pursued. However, all work products are structured to be assessor-ready, following the same format and completeness expected in a production ASIL D project.

## 7. Known Limitations and Assumptions

### 7.1 Assumptions

| ID | Assumption | Impact |
|----|-----------|--------|
| A-001 | S/E/C ratings in HARA assume the platform controls a real vehicle, not a bench demo | Higher ASIL than actual risk profile — intentional for portfolio value |
| A-002 | STM32G474RE is treated as an ASIL D capable MCU for software safety | STM32G474 is not ASIL-certified; safety is achieved through architecture (diverse SC) |
| A-003 | The TMS570LC43x Safety Controller provides ASIL D hardware integrity | TMS570 has TUV-certified safety manual and lockstep cores |
| A-004 | CAN bus is the sole inter-ECU communication path for safety functions | No secondary communication bus (no redundant Ethernet) |
| A-005 | The 12V bench power supply is stable and reliable | No automotive-grade voltage transients expected |
| A-006 | The E-stop button is hardwired with hardware debouncing | Not dependent on software for activation |
| A-007 | Generic Cortex-M4 failure rates are acceptable for STM32G474 FMEDA | ST does not publish G474-specific safety manual |

### 7.2 Limitations

| ID | Limitation | Mitigation |
|----|-----------|------------|
| L-001 | Demo platform, not a road-going vehicle | All safety analysis performed as-if real vehicle |
| L-002 | No redundant CAN bus | DFA documents shared bus as single point of failure; SC provides independent monitoring |
| L-003 | No automotive-grade power supply (load dump, cranking) | Not required for bench demo; assumption A-005 |
| L-004 | Simulated ECUs (BCM, ICU, TCU) run on Docker, not real hardware | QM-rated functions only; same C source code as physical would use |
| L-005 | No physical vehicle dynamics (no chassis, no wheels) | MIL plant models simulate dynamics; motor/servos demonstrate actuator response |
| L-006 | Single CAN bus for all ASIL levels (no ASIL/QM bus separation) | E2E protection on safety messages; SC monitors bus independently |
| L-007 | No secure boot or hardware security module (HSM) | Out of scope for portfolio demo; documented as gap |

## 8. Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 0.1 | 2026-02-21 | System | Initial stub |
| 1.0 | 2026-02-21 | System | Complete item definition with all sections |
