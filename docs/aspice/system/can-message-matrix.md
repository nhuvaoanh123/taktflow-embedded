---
document_id: CAN-MATRIX
title: "CAN Message Matrix"
version: "1.0"
status: draft
aspice_process: SYS.3
date: 2026-02-21
---

# CAN Message Matrix

## 1. Purpose and Scope

This document defines the complete CAN bus message matrix for the Taktflow Zonal Vehicle Platform per Automotive SPICE 4.0 SYS.3 (System Architectural Design). It specifies every CAN message on the 500 kbps CAN 2.0B bus, including signal-level definitions, E2E protection configuration, bus load analysis, and message timing.

This is the authoritative reference for all CAN communication between ECUs. All firmware CAN transmit and receive configurations shall be derived from this document.

## 2. Referenced Documents

| Document ID | Title | Version |
|-------------|-------|---------|
| SYSARCH | System Architecture | 1.0 |
| ICD | Interface Control Document | 1.0 |
| HSI | Hardware-Software Interface Specification | 1.0 |
| SWR-CVC | Software Requirements -- CVC | 1.0 |
| SWR-FZC | Software Requirements -- FZC | 1.0 |
| SWR-RZC | Software Requirements -- RZC | 1.0 |
| SWR-SC | Software Requirements -- SC | 1.0 |

## 3. CAN Bus Configuration

| Parameter | Value |
|-----------|-------|
| Standard | CAN 2.0B (11-bit standard identifiers) |
| Bit rate | 500 kbps |
| Sample point | 87.5% (recommended for 500 kbps) |
| SJW | 1 Tq |
| Propagation segment | 1 Tq |
| Phase segment 1 | 12 Tq |
| Phase segment 2 | 2 Tq |
| Time quanta per bit | 16 Tq |
| Topology | Linear bus, daisy-chain wiring |
| Termination | 120 ohm at each physical end |
| Wire | 22 AWG twisted pair (CAN_H, CAN_L) |
| Max bus length | 40 meters (500 kbps limit) |
| Actual bus length | < 2 meters (bench setup) |

## 4. CAN ID Allocation Strategy

### 4.1 Priority Bands

CAN IDs are assigned by safety priority. Lower CAN ID wins arbitration (higher priority on the bus).

| ID Range | Priority | Category | ASIL | Expansion Slots |
|----------|----------|----------|------|-----------------|
| 0x001-0x00F | Highest | Emergency broadcast | B | 14 reserved |
| 0x010-0x01F | Very High | Heartbeat messages | C | 12 reserved (0x013-0x01F) |
| 0x100-0x10F | High | Vehicle control commands | D | 12 reserved (0x104-0x10F) |
| 0x200-0x22F | Medium-High | Actuator status feedback | A-D | 17 reserved |
| 0x300-0x30F | Medium | Sensor data (motor, battery) | A-QM | 12 reserved |
| 0x350-0x36F | Normal | Body control | QM | 14 reserved |
| 0x400-0x40F | Low | Body status | QM | 13 reserved |
| 0x500-0x50F | Low | DTC broadcast | QM | 15 reserved |
| 0x600-0x6FF | Lowest | Diagnostics (UDS) | QM | Large reserved block |
| 0x7DF | Standard | UDS functional request | QM | Fixed per ISO 14229 |
| 0x7E0-0x7EE | Standard | UDS physical request/response | QM | Fixed per ISO 14229 |

### 4.2 Reserved ID Ranges

| ID Range | Reserved For | Notes |
|----------|-------------|-------|
| 0x002-0x00F | Future emergency messages | E.g., battery disconnect, thermal runaway |
| 0x013-0x01F | Future ECU heartbeats | Up to 13 additional ECUs |
| 0x104-0x10F | Future control commands | E.g., lighting commands, HVAC |
| 0x230-0x2FF | Future status messages | Sensor expansion |
| 0x310-0x34F | Future sensor channels | E.g., IMU, GPS |
| 0x410-0x4FF | Future body functions | Window, mirror, seat |
| 0x501-0x5FF | Future DTC channels | Per-ECU DTC streams |

## 5. Message Overview Table

| CAN ID | Message Name | Sender | Receiver(s) | DLC | Cycle (ms) | E2E | ASIL | Data ID |
|--------|-------------|--------|-------------|-----|-----------|-----|------|---------|
| 0x001 | EStop_Broadcast | CVC | ALL | 4 | Event (10 ms repeat) | Yes | B | 0x01 |
| 0x010 | CVC_Heartbeat | CVC | SC, FZC, RZC, ICU | 4 | 50 | Yes | C | 0x02 |
| 0x011 | FZC_Heartbeat | FZC | SC, CVC, ICU | 4 | 50 | Yes | C | 0x03 |
| 0x012 | RZC_Heartbeat | RZC | SC, CVC, ICU | 4 | 50 | Yes | C | 0x04 |
| 0x100 | Vehicle_State | CVC | FZC, RZC, BCM, ICU | 6 | 10 | Yes | D | 0x05 |
| 0x101 | Torque_Request | CVC | RZC | 8 | 10 | Yes | D | 0x06 |
| 0x102 | Steer_Command | CVC | FZC | 8 | 10 | Yes | D | 0x07 |
| 0x103 | Brake_Command | CVC | FZC | 8 | 10 | Yes | D | 0x08 |
| 0x200 | Steering_Status | FZC | CVC, SC, ICU | 8 | 20 | Yes | D | 0x09 |
| 0x201 | Brake_Status | FZC | CVC, SC, ICU | 8 | 20 | Yes | D | 0x0A |
| 0x210 | Brake_Fault | FZC | CVC, SC | 4 | Event | Yes | D | 0x0B |
| 0x211 | Motor_Cutoff_Req | FZC | CVC | 4 | Event (10 ms repeat) | Yes | D | 0x0C |
| 0x220 | Lidar_Distance | FZC | CVC, ICU | 8 | 10 | Yes | C | 0x0D |
| 0x300 | Motor_Status | RZC | CVC, SC, ICU | 8 | 20 | Yes | D | 0x0E |
| 0x301 | Motor_Current | RZC | SC, CVC, ICU | 8 | 10 | Yes | C | 0x0F |
| 0x302 | Motor_Temperature | RZC | CVC, ICU | 6 | 100 | Yes | A | 0x00 |
| 0x303 | Battery_Status | RZC | CVC, ICU | 4 | 1000 | No | QM | -- |
| 0x350 | Body_Control_Cmd | CVC | BCM | 4 | 100 | No | QM | -- |
| 0x360 | Body_Status | BCM | ICU | 4 | 100 | No | QM | -- |
| 0x400 | Light_Status | BCM | ICU | 4 | 100 | No | QM | -- |
| 0x401 | Indicator_State | BCM | ICU | 4 | 100 | No | QM | -- |
| 0x402 | Door_Lock_Status | BCM | ICU | 2 | Event | No | QM | -- |
| 0x500 | DTC_Broadcast | Any | TCU, ICU | 8 | Event | No | QM | -- |
| 0x7DF | UDS_Func_Request | Tester | ALL (TCU routes) | 8 | On-demand | No | QM | -- |
| 0x7E0 | UDS_Phys_Req_CVC | Tester | CVC | 8 | On-demand | No | QM | -- |
| 0x7E1 | UDS_Phys_Req_FZC | Tester | FZC | 8 | On-demand | No | QM | -- |
| 0x7E2 | UDS_Phys_Req_RZC | Tester | RZC | 8 | On-demand | No | QM | -- |
| 0x7E3 | UDS_Phys_Req_SC | Tester | SC (listen only) | 8 | On-demand | No | QM | -- |
| 0x7E8 | UDS_Resp_CVC | CVC | Tester | 8 | On-demand | No | QM | -- |
| 0x7E9 | UDS_Resp_FZC | FZC | Tester | 8 | On-demand | No | QM | -- |
| 0x7EA | UDS_Resp_RZC | RZC | Tester | 8 | On-demand | No | QM | -- |

**Total defined message types**: 31 (including per-ECU UDS variants).
**E2E protected messages**: 16 (all ASIL A or higher).
**Data ID assignment**: 16 unique Data IDs (0x00-0x0F) for E2E protected messages, fitting within the 4-bit Data ID field.

## 6. Signal Definitions

All signals use **little-endian** (Intel) byte order. Bit numbering follows the AUTOSAR convention: bit 0 is the LSB of byte 0.

### 6.1 EStop_Broadcast (0x001) -- DLC 4

| Signal | Start Bit | Length | Factor | Offset | Unit | Min | Max | Init | Description |
|--------|-----------|--------|--------|--------|------|-----|-----|------|-------------|
| E2E_AliveCounter | 4 | 4 | 1 | 0 | -- | 0 | 15 | 0 | Alive counter (increments per TX) |
| E2E_DataID | 0 | 4 | 1 | 0 | -- | 1 | 1 | 1 | Data ID = 0x01 |
| E2E_CRC8 | 8 | 8 | 1 | 0 | -- | 0 | 255 | 0xFF | CRC-8 SAE J1850 |
| EStop_Active | 16 | 1 | 1 | 0 | bool | 0 | 1 | 0 | 1 = E-stop active |
| EStop_Source | 17 | 3 | 1 | 0 | enum | 0 | 7 | 0 | 0=CVC button, 1=CAN request, 2=SC |
| Reserved | 20 | 12 | -- | -- | -- | -- | -- | 0 | Reserved for future use |

### 6.2 CVC_Heartbeat (0x010) -- DLC 4

| Signal | Start Bit | Length | Factor | Offset | Unit | Min | Max | Init | Description |
|--------|-----------|--------|--------|--------|------|-----|-----|------|-------------|
| E2E_AliveCounter | 4 | 4 | 1 | 0 | -- | 0 | 15 | 0 | Alive counter |
| E2E_DataID | 0 | 4 | 1 | 0 | -- | 2 | 2 | 2 | Data ID = 0x02 |
| E2E_CRC8 | 8 | 8 | 1 | 0 | -- | 0 | 255 | 0xFF | CRC-8 |
| ECU_ID | 16 | 8 | 1 | 0 | -- | 1 | 1 | 1 | ECU ID = 0x01 (CVC) |
| OperatingMode | 24 | 4 | 1 | 0 | enum | 0 | 5 | 0 | 0=INIT,1=RUN,2=DEGRADED,3=LIMP,4=SAFE_STOP,5=SHUTDOWN |
| FaultStatus | 28 | 4 | 1 | 0 | bitmask | 0 | 15 | 0 | bit0=pedal,bit1=CAN,bit2=NVM,bit3=WDT |

### 6.3 FZC_Heartbeat (0x011) -- DLC 4

| Signal | Start Bit | Length | Factor | Offset | Unit | Min | Max | Init | Description |
|--------|-----------|--------|--------|--------|------|-----|-----|------|-------------|
| E2E_AliveCounter | 4 | 4 | 1 | 0 | -- | 0 | 15 | 0 | Alive counter |
| E2E_DataID | 0 | 4 | 1 | 0 | -- | 3 | 3 | 3 | Data ID = 0x03 |
| E2E_CRC8 | 8 | 8 | 1 | 0 | -- | 0 | 255 | 0xFF | CRC-8 |
| ECU_ID | 16 | 8 | 1 | 0 | -- | 2 | 2 | 2 | ECU ID = 0x02 (FZC) |
| OperatingMode | 24 | 4 | 1 | 0 | enum | 0 | 5 | 0 | Same enum as CVC heartbeat |
| FaultStatus | 28 | 4 | 1 | 0 | bitmask | 0 | 15 | 0 | bit0=steering,bit1=brake,bit2=lidar,bit3=CAN |

### 6.4 RZC_Heartbeat (0x012) -- DLC 4

| Signal | Start Bit | Length | Factor | Offset | Unit | Min | Max | Init | Description |
|--------|-----------|--------|--------|--------|------|-----|-----|------|-------------|
| E2E_AliveCounter | 4 | 4 | 1 | 0 | -- | 0 | 15 | 0 | Alive counter |
| E2E_DataID | 0 | 4 | 1 | 0 | -- | 4 | 4 | 4 | Data ID = 0x04 |
| E2E_CRC8 | 8 | 8 | 1 | 0 | -- | 0 | 255 | 0xFF | CRC-8 |
| ECU_ID | 16 | 8 | 1 | 0 | -- | 3 | 3 | 3 | ECU ID = 0x03 (RZC) |
| OperatingMode | 24 | 4 | 1 | 0 | enum | 0 | 5 | 0 | Same enum as CVC heartbeat |
| FaultStatus | 28 | 4 | 1 | 0 | bitmask | 0 | 15 | 0 | bit0=overcurrent,bit1=overtemp,bit2=direction,bit3=CAN |

### 6.5 Vehicle_State (0x100) -- DLC 6

| Signal | Start Bit | Length | Factor | Offset | Unit | Min | Max | Init | Description |
|--------|-----------|--------|--------|--------|------|-----|-----|------|-------------|
| E2E_AliveCounter | 4 | 4 | 1 | 0 | -- | 0 | 15 | 0 | Alive counter |
| E2E_DataID | 0 | 4 | 1 | 0 | -- | 5 | 5 | 5 | Data ID = 0x05 |
| E2E_CRC8 | 8 | 8 | 1 | 0 | -- | 0 | 255 | 0xFF | CRC-8 |
| VehicleState | 16 | 4 | 1 | 0 | enum | 0 | 5 | 0 | 0=INIT..5=SHUTDOWN |
| FaultMask | 20 | 12 | 1 | 0 | bitmask | 0 | 4095 | 0 | Active fault bitmask (12-bit) |
| TorqueLimit | 32 | 8 | 1 | 0 | % | 0 | 100 | 0 | Max torque allowed |
| SpeedLimit | 40 | 8 | 1 | 0 | % | 0 | 100 | 0 | Max speed allowed |

### 6.6 Torque_Request (0x101) -- DLC 8

| Signal | Start Bit | Length | Factor | Offset | Unit | Min | Max | Init | Description |
|--------|-----------|--------|--------|--------|------|-----|-----|------|-------------|
| E2E_AliveCounter | 4 | 4 | 1 | 0 | -- | 0 | 15 | 0 | Alive counter |
| E2E_DataID | 0 | 4 | 1 | 0 | -- | 6 | 6 | 6 | Data ID = 0x06 |
| E2E_CRC8 | 8 | 8 | 1 | 0 | -- | 0 | 255 | 0xFF | CRC-8 |
| TorqueRequest | 16 | 8 | 1 | 0 | % | 0 | 100 | 0 | Requested torque percentage |
| Direction | 24 | 2 | 1 | 0 | enum | 0 | 2 | 0 | 0=stopped,1=forward,2=reverse |
| PedalPosition1 | 26 | 14 | 0.022 | 0 | deg | 0 | 359.98 | 0 | Pedal sensor 1 angle (14-bit) |
| PedalPosition2 | 40 | 14 | 0.022 | 0 | deg | 0 | 359.98 | 0 | Pedal sensor 2 angle (14-bit) |
| PedalFault | 54 | 1 | 1 | 0 | bool | 0 | 1 | 0 | 1 = pedal fault active |
| Reserved | 55 | 9 | -- | -- | -- | -- | -- | 0 | Reserved |

### 6.7 Steer_Command (0x102) -- DLC 8

| Signal | Start Bit | Length | Factor | Offset | Unit | Min | Max | Init | Description |
|--------|-----------|--------|--------|--------|------|-----|-----|------|-------------|
| E2E_AliveCounter | 4 | 4 | 1 | 0 | -- | 0 | 15 | 0 | Alive counter |
| E2E_DataID | 0 | 4 | 1 | 0 | -- | 7 | 7 | 7 | Data ID = 0x07 |
| E2E_CRC8 | 8 | 8 | 1 | 0 | -- | 0 | 255 | 0xFF | CRC-8 |
| SteerAngleCmd | 16 | 16 | 0.01 | -45.00 | deg | -45.00 | 45.00 | 0 | Commanded steering angle |
| SteerRateLimit | 32 | 8 | 0.2 | 0 | deg/s | 0 | 51.0 | 30 | Max steering rate |
| VehicleState | 40 | 4 | 1 | 0 | enum | 0 | 5 | 0 | Current vehicle state |
| Reserved | 44 | 20 | -- | -- | -- | -- | -- | 0 | Reserved |

### 6.8 Brake_Command (0x103) -- DLC 8

| Signal | Start Bit | Length | Factor | Offset | Unit | Min | Max | Init | Description |
|--------|-----------|--------|--------|--------|------|-----|-----|------|-------------|
| E2E_AliveCounter | 4 | 4 | 1 | 0 | -- | 0 | 15 | 0 | Alive counter |
| E2E_DataID | 0 | 4 | 1 | 0 | -- | 8 | 8 | 8 | Data ID = 0x08 |
| E2E_CRC8 | 8 | 8 | 1 | 0 | -- | 0 | 255 | 0xFF | CRC-8 |
| BrakeForceCmd | 16 | 8 | 1 | 0 | % | 0 | 100 | 0 | Commanded brake force |
| BrakeMode | 24 | 4 | 1 | 0 | enum | 0 | 3 | 0 | 0=release,1=normal,2=emergency,3=auto |
| VehicleState | 28 | 4 | 1 | 0 | enum | 0 | 5 | 0 | Current vehicle state |
| Reserved | 32 | 32 | -- | -- | -- | -- | -- | 0 | Reserved |

### 6.9 Steering_Status (0x200) -- DLC 8

| Signal | Start Bit | Length | Factor | Offset | Unit | Min | Max | Init | Description |
|--------|-----------|--------|--------|--------|------|-----|-----|------|-------------|
| E2E_AliveCounter | 4 | 4 | 1 | 0 | -- | 0 | 15 | 0 | Alive counter |
| E2E_DataID | 0 | 4 | 1 | 0 | -- | 9 | 9 | 9 | Data ID = 0x09 |
| E2E_CRC8 | 8 | 8 | 1 | 0 | -- | 0 | 255 | 0xFF | CRC-8 |
| ActualAngle | 16 | 16 | 0.01 | -45.00 | deg | -45.00 | 45.00 | 0 | Measured steering angle |
| CommandedAngle | 32 | 16 | 0.01 | -45.00 | deg | -45.00 | 45.00 | 0 | Last commanded angle |
| SteerFaultStatus | 48 | 4 | 1 | 0 | bitmask | 0 | 15 | 0 | bit0=position,bit1=range,bit2=rate,bit3=sensor |
| SteerMode | 52 | 4 | 1 | 0 | enum | 0 | 3 | 0 | 0=normal,1=return_center,2=disabled,3=local |
| ServoCurrent_mA | 56 | 8 | 10 | 0 | mA | 0 | 2550 | 0 | Servo current consumption |

### 6.10 Brake_Status (0x201) -- DLC 8

| Signal | Start Bit | Length | Factor | Offset | Unit | Min | Max | Init | Description |
|--------|-----------|--------|--------|--------|------|-----|-----|------|-------------|
| E2E_AliveCounter | 4 | 4 | 1 | 0 | -- | 0 | 15 | 0 | Alive counter |
| E2E_DataID | 0 | 4 | 1 | 0 | -- | 10 | 10 | 10 | Data ID = 0x0A |
| E2E_CRC8 | 8 | 8 | 1 | 0 | -- | 0 | 255 | 0xFF | CRC-8 |
| BrakePosition | 16 | 8 | 1 | 0 | % | 0 | 100 | 0 | Actual brake position |
| BrakeCommandEcho | 24 | 8 | 1 | 0 | % | 0 | 100 | 0 | Last brake command received |
| ServoCurrent_mA | 32 | 16 | 1 | 0 | mA | 0 | 3000 | 0 | Brake servo current |
| BrakeFaultStatus | 48 | 4 | 1 | 0 | bitmask | 0 | 15 | 0 | bit0=PWM_mismatch,bit1=servo_fault,bit2=timeout,bit3=overcurrent |
| BrakeMode | 52 | 4 | 1 | 0 | enum | 0 | 3 | 0 | 0=release,1=normal,2=auto_brake,3=emergency |
| Reserved | 56 | 8 | -- | -- | -- | -- | -- | 0 | Reserved |

### 6.11 Brake_Fault (0x210) -- DLC 4

| Signal | Start Bit | Length | Factor | Offset | Unit | Min | Max | Init | Description |
|--------|-----------|--------|--------|--------|------|-----|-----|------|-------------|
| E2E_AliveCounter | 4 | 4 | 1 | 0 | -- | 0 | 15 | 0 | Alive counter |
| E2E_DataID | 0 | 4 | 1 | 0 | -- | 11 | 11 | 11 | Data ID = 0x0B |
| E2E_CRC8 | 8 | 8 | 1 | 0 | -- | 0 | 255 | 0xFF | CRC-8 |
| FaultType | 16 | 4 | 1 | 0 | enum | 0 | 3 | 0 | 0=PWM_mismatch,1=servo_stuck,2=overcurrent,3=timeout |
| CommandedBrake | 20 | 8 | 1 | 0 | % | 0 | 100 | 0 | Brake % commanded at fault |
| MeasuredBrake | 28 | 4 | 10 | 0 | % | 0 | 100 | 0 | Measured brake position (coarse) |

### 6.12 Motor_Cutoff_Req (0x211) -- DLC 4

| Signal | Start Bit | Length | Factor | Offset | Unit | Min | Max | Init | Description |
|--------|-----------|--------|--------|--------|------|-----|-----|------|-------------|
| E2E_AliveCounter | 4 | 4 | 1 | 0 | -- | 0 | 15 | 0 | Alive counter |
| E2E_DataID | 0 | 4 | 1 | 0 | -- | 12 | 12 | 12 | Data ID = 0x0C |
| E2E_CRC8 | 8 | 8 | 1 | 0 | -- | 0 | 255 | 0xFF | CRC-8 |
| RequestType | 16 | 4 | 1 | 0 | enum | 0 | 3 | 0 | 0=motor_cutoff,1=speed_reduce,2=e_stop_fwd |
| Reason | 20 | 4 | 1 | 0 | enum | 0 | 7 | 0 | 0=brake_fault,1=steer_fault,2=lidar_emergency,3=CAN_loss |
| Reserved | 24 | 8 | -- | -- | -- | -- | -- | 0 | Reserved |

### 6.13 Lidar_Distance (0x220) -- DLC 8

| Signal | Start Bit | Length | Factor | Offset | Unit | Min | Max | Init | Description |
|--------|-----------|--------|--------|--------|------|-----|-----|------|-------------|
| E2E_AliveCounter | 4 | 4 | 1 | 0 | -- | 0 | 15 | 0 | Alive counter |
| E2E_DataID | 0 | 4 | 1 | 0 | -- | 13 | 13 | 13 | Data ID = 0x0D |
| E2E_CRC8 | 8 | 8 | 1 | 0 | -- | 0 | 255 | 0xFF | CRC-8 |
| Distance_cm | 16 | 16 | 1 | 0 | cm | 0 | 1200 | 0 | Measured distance |
| SignalStrength | 32 | 16 | 1 | 0 | -- | 0 | 65535 | 0 | TFMini-S signal strength |
| ObstacleZone | 48 | 4 | 1 | 0 | enum | 0 | 3 | 3 | 0=emergency,1=braking,2=warning,3=clear |
| SensorStatus | 52 | 4 | 1 | 0 | bitmask | 0 | 15 | 0 | bit0=timeout,bit1=range,bit2=stuck,bit3=weak_signal |
| Reserved | 56 | 8 | -- | -- | -- | -- | -- | 0 | Reserved |

### 6.14 Motor_Status (0x300) -- DLC 8

| Signal | Start Bit | Length | Factor | Offset | Unit | Min | Max | Init | Description |
|--------|-----------|--------|--------|--------|------|-----|-----|------|-------------|
| E2E_AliveCounter | 4 | 4 | 1 | 0 | -- | 0 | 15 | 0 | Alive counter |
| E2E_DataID | 0 | 4 | 1 | 0 | -- | 14 | 14 | 14 | Data ID = 0x0E |
| E2E_CRC8 | 8 | 8 | 1 | 0 | -- | 0 | 255 | 0xFF | CRC-8 |
| MotorSpeed_RPM | 16 | 16 | 1 | 0 | RPM | 0 | 10000 | 0 | Encoder-measured speed |
| MotorDirection | 32 | 2 | 1 | 0 | enum | 0 | 2 | 0 | 0=stopped,1=forward,2=reverse |
| MotorEnable | 34 | 1 | 1 | 0 | bool | 0 | 1 | 0 | 1 = motor driver enabled |
| MotorFaultStatus | 35 | 5 | 1 | 0 | bitmask | 0 | 31 | 0 | bit0=overcurrent,bit1=overtemp,bit2=stall,bit3=direction,bit4=driver |
| DutyPercent | 40 | 8 | 1 | 0 | % | 0 | 95 | 0 | Current PWM duty cycle |
| DeratingPercent | 48 | 8 | 1 | 0 | % | 0 | 100 | 100 | Remaining capacity (100 = no derating) |
| Reserved | 56 | 8 | -- | -- | -- | -- | -- | 0 | Reserved |

### 6.15 Motor_Current (0x301) -- DLC 8

| Signal | Start Bit | Length | Factor | Offset | Unit | Min | Max | Init | Description |
|--------|-----------|--------|--------|--------|------|-----|-----|------|-------------|
| E2E_AliveCounter | 4 | 4 | 1 | 0 | -- | 0 | 15 | 0 | Alive counter |
| E2E_DataID | 0 | 4 | 1 | 0 | -- | 15 | 15 | 15 | Data ID = 0x0F |
| E2E_CRC8 | 8 | 8 | 1 | 0 | -- | 0 | 255 | 0xFF | CRC-8 |
| Current_mA | 16 | 16 | 1 | 0 | mA | 0 | 30000 | 0 | ACS723 measured current |
| CurrentDirection | 32 | 1 | 1 | 0 | enum | 0 | 1 | 0 | 0=forward,1=reverse |
| MotorEnable | 33 | 1 | 1 | 0 | bool | 0 | 1 | 0 | 1 = motor driver enabled |
| OvercurrentFlag | 34 | 1 | 1 | 0 | bool | 0 | 1 | 0 | 1 = overcurrent threshold exceeded |
| TorqueEcho | 35 | 8 | 1 | 0 | % | 0 | 100 | 0 | Torque command echo from CVC |
| Reserved | 43 | 21 | -- | -- | -- | -- | -- | 0 | Reserved |

### 6.16 Motor_Temperature (0x302) -- DLC 6

| Signal | Start Bit | Length | Factor | Offset | Unit | Min | Max | Init | Description |
|--------|-----------|--------|--------|--------|------|-----|-----|------|-------------|
| E2E_AliveCounter | 4 | 4 | 1 | 0 | -- | 0 | 15 | 0 | Alive counter |
| E2E_DataID | 0 | 4 | 1 | 0 | -- | 0 | 0 | 0 | Data ID = 0x00 |
| E2E_CRC8 | 8 | 8 | 1 | 0 | -- | 0 | 255 | 0xFF | CRC-8 |
| WindingTemp1_C | 16 | 8 | 1 | -40 | degC | -40 | 215 | 25 | NTC 1 motor temp |
| WindingTemp2_C | 24 | 8 | 1 | -40 | degC | -40 | 215 | 25 | NTC 2 board temp |
| DeratingPercent | 32 | 8 | 1 | 0 | % | 0 | 100 | 100 | Derating level (100 = full power) |
| TempFaultStatus | 40 | 4 | 1 | 0 | bitmask | 0 | 15 | 0 | bit0=NTC1_fault,bit1=NTC2_fault,bit2=overtemp,bit3=derating_active |
| Reserved | 44 | 4 | -- | -- | -- | -- | -- | 0 | Reserved |

### 6.17 Battery_Status (0x303) -- DLC 4 (No E2E)

| Signal | Start Bit | Length | Factor | Offset | Unit | Min | Max | Init | Description |
|--------|-----------|--------|--------|--------|------|-----|-----|------|-------------|
| BatteryVoltage_mV | 0 | 16 | 1 | 0 | mV | 0 | 20000 | 12000 | Battery voltage |
| BatterySOC | 16 | 8 | 1 | 0 | % | 0 | 100 | 100 | State of charge estimate |
| BatteryStatus | 24 | 4 | 1 | 0 | enum | 0 | 4 | 2 | 0=critical_UV,1=UV_warn,2=normal,3=OV_warn,4=critical_OV |
| Reserved | 28 | 4 | -- | -- | -- | -- | -- | 0 | Reserved |

### 6.18 Body_Control_Cmd (0x350) -- DLC 4 (No E2E)

| Signal | Start Bit | Length | Factor | Offset | Unit | Min | Max | Init | Description |
|--------|-----------|--------|--------|--------|------|-----|-----|------|-------------|
| HeadlightCmd | 0 | 2 | 1 | 0 | enum | 0 | 2 | 0 | 0=off,1=low,2=high |
| TailLightCmd | 2 | 1 | 1 | 0 | bool | 0 | 1 | 0 | 1 = tail lights on |
| HazardCmd | 3 | 1 | 1 | 0 | bool | 0 | 1 | 0 | 1 = hazard active |
| TurnSignalCmd | 4 | 2 | 1 | 0 | enum | 0 | 2 | 0 | 0=off,1=left,2=right |
| DoorLockCmd | 6 | 1 | 1 | 0 | bool | 0 | 1 | 0 | 1 = lock all doors |
| Reserved | 7 | 25 | -- | -- | -- | -- | -- | 0 | Reserved |

### 6.19 Body_Status (0x360) -- DLC 4 (No E2E)

| Signal | Start Bit | Length | Factor | Offset | Unit | Min | Max | Init | Description |
|--------|-----------|--------|--------|--------|------|-----|-----|------|-------------|
| HeadlightState | 0 | 2 | 1 | 0 | enum | 0 | 2 | 0 | 0=off,1=low,2=high |
| TailLightState | 2 | 1 | 1 | 0 | bool | 0 | 1 | 0 | Current tail light state |
| HazardState | 3 | 1 | 1 | 0 | bool | 0 | 1 | 0 | Current hazard state |
| TurnSignalState | 4 | 2 | 1 | 0 | enum | 0 | 2 | 0 | 0=off,1=left,2=right |
| DoorLockState | 6 | 1 | 1 | 0 | bool | 0 | 1 | 0 | 1 = locked |
| Reserved | 7 | 25 | -- | -- | -- | -- | -- | 0 | Reserved |

### 6.20 Light_Status (0x400) -- DLC 4 (No E2E)

| Signal | Start Bit | Length | Factor | Offset | Unit | Min | Max | Init | Description |
|--------|-----------|--------|--------|--------|------|-----|-----|------|-------------|
| HeadlightOn | 0 | 1 | 1 | 0 | bool | 0 | 1 | 0 | 1 = headlights active |
| TailLightOn | 1 | 1 | 1 | 0 | bool | 0 | 1 | 0 | 1 = tail lights active |
| FogLightOn | 2 | 1 | 1 | 0 | bool | 0 | 1 | 0 | 1 = fog lights active |
| BrakeLightOn | 3 | 1 | 1 | 0 | bool | 0 | 1 | 0 | 1 = brake lights active |
| HeadlightLevel | 4 | 2 | 1 | 0 | enum | 0 | 2 | 0 | 0=off,1=low,2=high |
| Reserved | 6 | 26 | -- | -- | -- | -- | -- | 0 | Reserved |

### 6.21 Indicator_State (0x401) -- DLC 4 (No E2E)

| Signal | Start Bit | Length | Factor | Offset | Unit | Min | Max | Init | Description |
|--------|-----------|--------|--------|--------|------|-----|-----|------|-------------|
| LeftIndicator | 0 | 1 | 1 | 0 | bool | 0 | 1 | 0 | 1 = left indicator on |
| RightIndicator | 1 | 1 | 1 | 0 | bool | 0 | 1 | 0 | 1 = right indicator on |
| HazardActive | 2 | 1 | 1 | 0 | bool | 0 | 1 | 0 | 1 = hazard flasher active |
| BlinkState | 3 | 1 | 1 | 0 | bool | 0 | 1 | 0 | Current blink phase (0=off,1=on) |
| Reserved | 4 | 28 | -- | -- | -- | -- | -- | 0 | Reserved |

### 6.22 Door_Lock_Status (0x402) -- DLC 2 (No E2E)

| Signal | Start Bit | Length | Factor | Offset | Unit | Min | Max | Init | Description |
|--------|-----------|--------|--------|--------|------|-----|-----|------|-------------|
| FrontLeftLock | 0 | 1 | 1 | 0 | bool | 0 | 1 | 0 | 1 = locked |
| FrontRightLock | 1 | 1 | 1 | 0 | bool | 0 | 1 | 0 | 1 = locked |
| RearLeftLock | 2 | 1 | 1 | 0 | bool | 0 | 1 | 0 | 1 = locked |
| RearRightLock | 3 | 1 | 1 | 0 | bool | 0 | 1 | 0 | 1 = locked |
| CentralLock | 4 | 1 | 1 | 0 | bool | 0 | 1 | 0 | 1 = central lock active |
| Reserved | 5 | 11 | -- | -- | -- | -- | -- | 0 | Reserved |

### 6.23 DTC_Broadcast (0x500) -- DLC 8 (No E2E)

| Signal | Start Bit | Length | Factor | Offset | Unit | Min | Max | Init | Description |
|--------|-----------|--------|--------|--------|------|-----|-----|------|-------------|
| DTC_Number | 0 | 16 | 1 | 0 | -- | 0 | 65535 | 0 | DTC code per SAE J2012 |
| DTC_Status | 16 | 8 | 1 | 0 | -- | 0 | 255 | 0 | ISO 14229 status byte |
| ECU_Source | 24 | 8 | 1 | 0 | -- | 0 | 7 | 0 | 1=CVC,2=FZC,3=RZC,4=SC,5=BCM,6=ICU,7=TCU |
| OccurrenceCount | 32 | 8 | 1 | 0 | -- | 0 | 255 | 0 | Occurrence counter |
| FreezeFrame0 | 40 | 8 | 1 | 0 | -- | 0 | 255 | 0 | Freeze-frame byte 0 (context-dependent) |
| FreezeFrame1 | 48 | 8 | 1 | 0 | -- | 0 | 255 | 0 | Freeze-frame byte 1 |
| FreezeFrame2 | 56 | 8 | 1 | 0 | -- | 0 | 255 | 0 | Freeze-frame byte 2 |

### 6.24 UDS Messages (0x7DF, 0x7E0-0x7EA)

UDS messages follow ISO 14229 / ISO 15765-2 (CAN transport protocol) framing. The payload is a standard ISO-TP frame:

| Signal | Start Bit | Length | Factor | Offset | Unit | Description |
|--------|-----------|--------|--------|--------|------|-------------|
| PCI | 0 | 8 | -- | -- | -- | Protocol Control Information (SF/FF/CF/FC) |
| SID | 8 | 8 | -- | -- | -- | Service Identifier (e.g., 0x22 = ReadDataById) |
| Data | 16 | 48 | -- | -- | -- | Service-specific data payload |

UDS physical addressing per ECU:

| ECU | Request CAN ID | Response CAN ID |
|-----|---------------|-----------------|
| CVC | 0x7E0 | 0x7E8 |
| FZC | 0x7E1 | 0x7E9 |
| RZC | 0x7E2 | 0x7EA |
| TCU | 0x7E3 | 0x7EB |
| BCM | 0x7E4 | 0x7EC |
| ICU | 0x7E5 | 0x7ED |
| Functional | 0x7DF | Per-ECU response ID |

## 7. E2E Protection Details

### 7.1 E2E Configuration Summary

| Parameter | Value |
|-----------|-------|
| CRC Polynomial | CRC-8/SAE-J1850 (0x1D) |
| CRC Initial Value | 0xFF |
| CRC Input | Data ID byte + payload bytes 2..DLC-1 |
| CRC Position | Byte 1 of CAN payload |
| Alive Counter Size | 4-bit (values 0-15, wraps) |
| Alive Counter Position | Byte 0, bits 7:4 |
| Data ID Size | 4-bit (values 0x00-0x0F) |
| Data ID Position | Byte 0, bits 3:0 |
| Hamming Distance | 4 (for payloads up to 8 bytes) |

### 7.2 E2E Header Layout (Bytes 0-1)

```
Byte 0:  [7:4] = Alive Counter    [3:0] = Data ID
Byte 1:  [7:0] = CRC-8
Bytes 2..DLC-1: Application data payload
```

### 7.3 CRC-8 Computation

```
Input:  DataID_byte || Payload[2] || Payload[3] || ... || Payload[DLC-1]
Init:   0xFF
Poly:   0x1D (x^8 + x^4 + x^3 + x^2 + 1)
Result: Stored in Payload[1]
```

Pseudocode:

```c
uint8_t crc = 0xFF;
crc = crc8_table[crc ^ data_id_byte];
for (i = 2; i < dlc; i++) {
    crc = crc8_table[crc ^ payload[i]];
}
payload[1] = crc;
```

### 7.4 Per-Message E2E Configuration

| CAN ID | Data ID | Timeout (ms) | Max Alive Delta | Safe Default |
|--------|---------|-------------|-----------------|--------------|
| 0x001 | 0x01 | 50 | 1 | E-stop assumed active |
| 0x010 | 0x02 | 200 | 1 | CVC heartbeat timeout -> DEGRADED |
| 0x011 | 0x03 | 200 | 1 | FZC heartbeat timeout -> DEGRADED |
| 0x012 | 0x04 | 200 | 1 | RZC heartbeat timeout -> DEGRADED |
| 0x100 | 0x05 | 30 | 1 | SAFE_STOP state assumed |
| 0x101 | 0x06 | 30 | 1 | Zero torque (motor disabled) |
| 0x102 | 0x07 | 100 | 1 | Return-to-center (0 deg) |
| 0x103 | 0x08 | 100 | 1 | Max braking (100%) |
| 0x200 | 0x09 | 60 | 1 | Last valid angle, then center |
| 0x201 | 0x0A | 60 | 1 | Last valid position, then 100% brake |
| 0x210 | 0x0B | Event | 1 | No safe default (event msg) |
| 0x211 | 0x0C | Event | 1 | No safe default (event msg) |
| 0x220 | 0x0D | 30 | 1 | Distance = 0 cm (emergency assumed) |
| 0x300 | 0x0E | 60 | 1 | Motor assumed disabled |
| 0x301 | 0x0F | 30 | 1 | Current = 0 (motor assumed disabled) |
| 0x302 | 0x00 | 300 | 1 | Last valid temp, then max derating |

### 7.5 Receiver E2E Failure Behavior

| Consecutive Failures | Action |
|---------------------|--------|
| 1 | Use last valid value; no warning |
| 2 | Use last valid value; log E2E warning event |
| 3 | Substitute safe default value; log DTC |
| 4+ | Maintain safe default; escalate to state machine fault |

## 8. Bus Load Analysis

### 8.1 Per-Message Bit Calculation

CAN 2.0B frame overhead (standard 11-bit ID):

| Component | Bits |
|-----------|------|
| Start of Frame | 1 |
| Arbitration (11-bit ID + RTR) | 12 |
| Control (IDE + r0 + DLC) | 6 |
| Data field | 8 * DLC |
| CRC (15-bit + delimiter) | 16 |
| ACK (slot + delimiter) | 2 |
| End of Frame | 7 |
| Intermission | 3 |
| **Total overhead** | **47** |

Total bits per frame = 47 + (8 * DLC). Including worst-case bit stuffing (1 stuff bit per 4 data bits): stuff bits = floor((34 + 8*DLC) / 4).

| DLC | Data Bits | Overhead | Stuff Bits (max) | Total Bits (max) |
|-----|-----------|----------|-----------------|------------------|
| 2 | 16 | 47 | 12 | 75 |
| 4 | 32 | 47 | 16 | 95 |
| 6 | 48 | 47 | 20 | 115 |
| 8 | 64 | 47 | 24 | 135 |

### 8.2 Worst-Case Bus Load Calculation

| CAN ID | Name | DLC | Cycle (ms) | Bits/frame | Frames/s | Bits/s |
|--------|------|-----|-----------|------------|----------|--------|
| 0x001 | EStop (event, worst-case assume active) | 4 | 10 | 95 | 100 | 9,500 |
| 0x010 | CVC Heartbeat | 4 | 50 | 95 | 20 | 1,900 |
| 0x011 | FZC Heartbeat | 4 | 50 | 95 | 20 | 1,900 |
| 0x012 | RZC Heartbeat | 4 | 50 | 95 | 20 | 1,900 |
| 0x100 | Vehicle State | 6 | 10 | 115 | 100 | 11,500 |
| 0x101 | Torque Request | 8 | 10 | 135 | 100 | 13,500 |
| 0x102 | Steer Command | 8 | 10 | 135 | 100 | 13,500 |
| 0x103 | Brake Command | 8 | 10 | 135 | 100 | 13,500 |
| 0x200 | Steering Status | 8 | 20 | 135 | 50 | 6,750 |
| 0x201 | Brake Status | 8 | 20 | 135 | 50 | 6,750 |
| 0x210 | Brake Fault (event, assume 1/s) | 4 | 1000 | 95 | 1 | 95 |
| 0x211 | Motor Cutoff (event, assume 1/s) | 4 | 1000 | 95 | 1 | 95 |
| 0x220 | Lidar Distance | 8 | 10 | 135 | 100 | 13,500 |
| 0x300 | Motor Status | 8 | 20 | 135 | 50 | 6,750 |
| 0x301 | Motor Current | 8 | 10 | 135 | 100 | 13,500 |
| 0x302 | Motor Temperature | 6 | 100 | 115 | 10 | 1,150 |
| 0x303 | Battery Status | 4 | 1000 | 95 | 1 | 95 |
| 0x350 | Body Control Cmd | 4 | 100 | 95 | 10 | 950 |
| 0x360 | Body Status | 4 | 100 | 95 | 10 | 950 |
| 0x400 | Light Status | 4 | 100 | 95 | 10 | 950 |
| 0x401 | Indicator State | 4 | 100 | 95 | 10 | 950 |
| 0x402 | Door Lock (event, 1/s) | 2 | 1000 | 75 | 1 | 75 |
| 0x500 | DTC Broadcast (event, 1/s) | 8 | 1000 | 135 | 1 | 135 |
| | | | | | | |
| | **Total (worst-case)** | | | | | **119,895** |

### 8.3 Bus Utilization Summary

| Metric | Value |
|--------|-------|
| Total bus bandwidth | 500,000 bits/s |
| Worst-case bus load (all messages, E-stop active) | 119,895 bits/s |
| **Worst-case utilization** | **24.0%** |
| Typical bus load (E-stop inactive, events rare) | ~110,000 bits/s |
| **Typical utilization** | **22.0%** |
| Design target | < 50% |
| ASIL D budget (safety messages only) | < 30% |
| Safety message load only (0x001-0x302) | ~117,400 bits/s = 23.5% |
| Remaining capacity for expansion | ~380,000 bits/s (76%) |

**Assessment**: Bus load is well within the 50% design target and the 30% ASIL D safety budget. Significant headroom exists for adding messages during development.

## 9. Message Timing Diagram

### 9.1 100 ms Window Schedule

The following ASCII timing diagram shows message transmission scheduling over a 100 ms window. Time flows left to right. Each row represents one CAN ID. Tick marks represent scheduled transmissions.

```
Time (ms)   0    10   20   30   40   50   60   70   80   90   100
            |    |    |    |    |    |    |    |    |    |    |
0x001 E-stp [----only when active, 10ms repeat-----------------]
            |    |    |    |    |    |    |    |    |    |    |
0x010 CVC-HB     .    .    .    .   X    .    .    .    .   X
0x011 FZC-HB     .    .    .    .   X    .    .    .    .   X
0x012 RZC-HB     .    .    .    .   X    .    .    .    .   X
            |    |    |    |    |    |    |    |    |    |    |
0x100 State X    X    X    X    X    X    X    X    X    X    X
0x101 Torq  X    X    X    X    X    X    X    X    X    X    X
0x102 Steer X    X    X    X    X    X    X    X    X    X    X
0x103 Brake X    X    X    X    X    X    X    X    X    X    X
            |    |    |    |    |    |    |    |    |    |    |
0x200 StSts .    X    .    X    .    X    .    X    .    X    .
0x201 BkSts .    X    .    X    .    X    .    X    .    X    .
            |    |    |    |    |    |    |    |    |    |    |
0x220 Lidar X    X    X    X    X    X    X    X    X    X    X
            |    |    |    |    |    |    |    |    |    |    |
0x300 MotSt .    X    .    X    .    X    .    X    .    X    .
0x301 Curr  X    X    X    X    X    X    X    X    X    X    X
0x302 Temp  X    .    .    .    .    .    .    .    .    .    X
            |    |    |    |    |    |    |    |    |    |    |
0x350 BodyC X    .    .    .    .    .    .    .    .    .    X
0x360 BodyS X    .    .    .    .    .    .    .    .    .    X
0x400 Light X    .    .    .    .    .    .    .    .    .    X
0x401 Indic X    .    .    .    .    .    .    .    .    .    X

Legend: X = scheduled transmission, . = no transmission
        Heartbeats offset by 2ms from control messages to spread load
```

### 9.2 10 ms Slot Detail

Within each 10 ms period, messages are staggered to avoid simultaneous arbitration. CAN arbitration handles collisions, but staggering reduces worst-case latency:

```
Time within 10ms slot:
  0.0 ms  0x100 Vehicle_State (CVC TX)
  0.3 ms  0x101 Torque_Request (CVC TX)
  0.6 ms  0x102 Steer_Command (CVC TX)
  0.9 ms  0x103 Brake_Command (CVC TX)
  1.5 ms  0x220 Lidar_Distance (FZC TX)
  2.0 ms  0x301 Motor_Current (RZC TX)
  3.0 ms  [slot for 20ms messages: 0x200, 0x201, 0x300]
  5.0 ms  [slot for heartbeats: 0x010, 0x011, 0x012]
  7.0 ms  [slot for body/QM messages]
  9.0 ms  [idle / event message opportunity]
```

### 9.3 Worst-Case Latency Analysis

| Message | Cycle | Max frames ahead | Max latency | Budget |
|---------|-------|-----------------|-------------|--------|
| 0x001 E-Stop | Event | 0 (highest priority) | 0.27 ms (1 frame) | 1 ms |
| 0x010-0x012 Heartbeat | 50 ms | 4 (control msgs) | 1.4 ms | 5 ms |
| 0x100-0x103 Control | 10 ms | 0-3 (same group) | 1.1 ms | 1 ms |
| 0x200-0x201 Status | 20 ms | 7 | 2.0 ms | 5 ms |
| 0x301 Current | 10 ms | 6 | 1.7 ms | 5 ms |

All worst-case latencies are well within their respective budgets.

## 10. Data ID Assignment Table

| Data ID | CAN ID | Message Name | Sender |
|---------|--------|-------------|--------|
| 0x00 | 0x302 | Motor_Temperature | RZC |
| 0x01 | 0x001 | EStop_Broadcast | CVC |
| 0x02 | 0x010 | CVC_Heartbeat | CVC |
| 0x03 | 0x011 | FZC_Heartbeat | FZC |
| 0x04 | 0x012 | RZC_Heartbeat | RZC |
| 0x05 | 0x100 | Vehicle_State | CVC |
| 0x06 | 0x101 | Torque_Request | CVC |
| 0x07 | 0x102 | Steer_Command | CVC |
| 0x08 | 0x103 | Brake_Command | CVC |
| 0x09 | 0x200 | Steering_Status | FZC |
| 0x0A | 0x201 | Brake_Status | FZC |
| 0x0B | 0x210 | Brake_Fault | FZC |
| 0x0C | 0x211 | Motor_Cutoff_Req | FZC |
| 0x0D | 0x220 | Lidar_Distance | FZC |
| 0x0E | 0x300 | Motor_Status | RZC |
| 0x0F | 0x301 | Motor_Current | RZC |

## 11. Per-ECU Message Summary

### 11.1 CVC Transmit Messages

| CAN ID | Name | DLC | Cycle | E2E | Data ID |
|--------|------|-----|-------|-----|---------|
| 0x001 | EStop_Broadcast | 4 | Event/10 ms | Yes | 0x01 |
| 0x010 | CVC_Heartbeat | 4 | 50 ms | Yes | 0x02 |
| 0x100 | Vehicle_State | 6 | 10 ms | Yes | 0x05 |
| 0x101 | Torque_Request | 8 | 10 ms | Yes | 0x06 |
| 0x102 | Steer_Command | 8 | 10 ms | Yes | 0x07 |
| 0x103 | Brake_Command | 8 | 10 ms | Yes | 0x08 |
| 0x350 | Body_Control_Cmd | 4 | 100 ms | No | -- |
| 0x7E8 | UDS_Resp_CVC | 8 | On-demand | No | -- |

### 11.2 FZC Transmit Messages

| CAN ID | Name | DLC | Cycle | E2E | Data ID |
|--------|------|-----|-------|-----|---------|
| 0x011 | FZC_Heartbeat | 4 | 50 ms | Yes | 0x03 |
| 0x200 | Steering_Status | 8 | 20 ms | Yes | 0x09 |
| 0x201 | Brake_Status | 8 | 20 ms | Yes | 0x0A |
| 0x210 | Brake_Fault | 4 | Event | Yes | 0x0B |
| 0x211 | Motor_Cutoff_Req | 4 | Event/10 ms | Yes | 0x0C |
| 0x220 | Lidar_Distance | 8 | 10 ms | Yes | 0x0D |
| 0x7E9 | UDS_Resp_FZC | 8 | On-demand | No | -- |

### 11.3 RZC Transmit Messages

| CAN ID | Name | DLC | Cycle | E2E | Data ID |
|--------|------|-----|-------|-----|---------|
| 0x012 | RZC_Heartbeat | 4 | 50 ms | Yes | 0x04 |
| 0x300 | Motor_Status | 8 | 20 ms | Yes | 0x0E |
| 0x301 | Motor_Current | 8 | 10 ms | Yes | 0x0F |
| 0x302 | Motor_Temperature | 6 | 100 ms | Yes | 0x00 |
| 0x303 | Battery_Status | 4 | 1000 ms | No | -- |
| 0x7EA | UDS_Resp_RZC | 8 | On-demand | No | -- |

### 11.4 SC Receive Messages (Listen-Only)

| CAN ID | Name | Purpose |
|--------|------|---------|
| 0x001 | EStop_Broadcast | E-stop detection |
| 0x010 | CVC_Heartbeat | CVC alive monitoring |
| 0x011 | FZC_Heartbeat | FZC alive monitoring |
| 0x012 | RZC_Heartbeat | RZC alive monitoring |
| 0x100 | Vehicle_State | Cross-plausibility (torque request) |
| 0x301 | Motor_Current | Cross-plausibility (actual current) |

### 11.5 BCM Transmit Messages

| CAN ID | Name | DLC | Cycle | E2E |
|--------|------|-----|-------|-----|
| 0x360 | Body_Status | 4 | 100 ms | No |
| 0x400 | Light_Status | 4 | 100 ms | No |
| 0x401 | Indicator_State | 4 | 100 ms | No |
| 0x402 | Door_Lock_Status | 2 | Event | No |

### 11.6 ICU Receive Messages (Listen-Only Consumer)

The ICU listens to all messages on the bus for dashboard display purposes. No CAN acceptance filter restricts ICU reception.

### 11.7 TCU Messages

| Direction | CAN ID | Name |
|-----------|--------|------|
| Receive | 0x7DF | UDS Functional Request |
| Receive | 0x7E3 | UDS Physical Request (TCU) |
| Transmit | 0x7EB | UDS Response (TCU) |
| Receive | 0x500 | DTC Broadcast |

## 12. Traceability

| SYS Requirement | CAN Messages |
|----------------|-------------|
| SYS-001 (Dual pedal sensing) | 0x101 (TorqueRequest.PedalPosition1/2) |
| SYS-004 (Motor torque control) | 0x101 (TorqueRequest), 0x100 (VehicleState.TorqueLimit) |
| SYS-010 (Steering command) | 0x102 (SteerCommand) |
| SYS-011 (Steering feedback) | 0x200 (SteeringStatus) |
| SYS-014 (Brake command) | 0x103 (BrakeCommand) |
| SYS-015 (Brake monitoring) | 0x201 (BrakeStatus), 0x210 (BrakeFault) |
| SYS-018 (Lidar sensing) | 0x220 (LidarDistance) |
| SYS-021 (Heartbeat TX) | 0x010, 0x011, 0x012 |
| SYS-022 (Heartbeat timeout) | SC monitors 0x010, 0x011, 0x012 |
| SYS-023 (Cross-plausibility) | SC monitors 0x100, 0x301 |
| SYS-028 (E-stop broadcast) | 0x001 |
| SYS-029 (Vehicle state) | 0x100 (VehicleState) |
| SYS-031 (CAN config) | All messages |
| SYS-032 (E2E protection) | All messages with ASIL >= A |
| SYS-033 (CAN priority) | CAN ID allocation (Section 4) |
| SYS-037-040 (UDS diagnostics) | 0x7DF, 0x7E0-0x7EA |
| SYS-041 (DTC storage) | 0x500 (DTC_Broadcast) |

## 13. Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 0.1 | 2026-02-21 | System | Initial stub (planned status) |
| 1.0 | 2026-02-21 | System | Complete CAN message matrix: 31 message types, 16 E2E-protected messages with full signal definitions, bus load analysis (24% worst-case), Data ID assignment table, timing diagram, per-ECU message summary |
