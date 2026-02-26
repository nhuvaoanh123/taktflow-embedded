# 03 — CAN Bus

**Block**: CAN topology, transceivers, termination
**Source**: HWDES Section 6

## Bus Topology

```
  [CVC]----100mm----[FZC]----100mm----[RZC]----100mm----[SC]
   |                                                      |
  [120R]                                                [120R]
  term.                                                 term.

  Bus stubs: < 100 mm from main trunk to each ECU transceiver
  Total bus length: ~300 mm main trunk + stubs = ~700 mm total
  (well within 2 m maximum specified in HWR-014)

  Additional taps (T-connection, < 100 mm stub):
  - CANable 2.0 #1: connected to Raspberry Pi 4 (USB)
  - CANable 2.0 #2: connected to development PC (USB) for CAN analyzer
```

## STM32 CAN Transceiver Circuit (CVC/FZC/RZC)

```
  STM32G474RE                TJA1051T/3 Module
  +-----------+              +------------------+
  |           |              |                  |
  | FDCAN1_TX +--- PA12 --->| TXD         CANH |--+--[CMC]--+-- CAN_H (yellow)
  | (AF9)     |              |                  |  |         |
  | FDCAN1_RX +--- PA11 ---<| RXD         CANL |--+--[CMC]--+-- CAN_L (green)
  | (AF9)     |              |                  |  |         |
  +-----------+  3.3V --+-->| VCC          GND |--+-- GND   +--[120R]--+
                        |   |                  |                       |
                    [100nF] | STB          N/C |                    CAN_L
                        |   +------------------+
                       GND        |
                                 GND (STB tied LOW = always active)

  CMC = Common-mode choke (100 uH minimum)
  TVS diodes (PESD1CAN) placed between CMC and bus connectors
  120R termination resistor at CVC end of bus (FZC, RZC: no termination)
```

## TMS570 CAN Transceiver Circuit (SC)

```
  TMS570LC43x                   SN65HVD230
  +-----------+                  +------------------+
  |           |                  |                  |
  | DCAN1_TX  +--- (edge conn)->| TXD         CANH |--[CMC]--+-- CAN_H
  |           |                  |                  |         |
  | DCAN1_RX  +--- (edge conn)<-| RXD         CANL |--[CMC]--+-- CAN_L
  |           |                  |                  |         |
  +-----------+  3.3V --[100nF]->| VCC          GND |  [120R termination]
                                 |                  |         |
                                 | Rs           N/C |       CAN_L
                                 +--+---------------+
                                    |
                                   GND (Rs = GND for full speed)

  DCAN1 TEST register bit 3 = 1 (Silent/Listen-only mode).
  120R termination at SC (SC is at one end of the bus).
  Using DCAN1 via edge connector (NOT DCAN4 due to HALCoGen bug).
```

## CAN Connector Plan

Each ECU connects via a 3-position screw terminal block:

| Terminal | Signal | Wire Color |
|----------|--------|------------|
| 1 | CAN_H | Yellow |
| 2 | CAN_L | Green |
| 3 | GND (CAN reference) | Black |

Daisy-chain wiring: main trunk wire runs along base plate edge, continuous through each terminal block.

## Bit Timing Configuration

| Parameter | STM32G474RE (FDCAN) | TMS570LC43x (DCAN) |
|-----------|--------------------|--------------------|
| Input clock | 170 MHz (PCLK1) | 75 MHz (VCLK1) |
| Prescaler | 34 | 15 |
| Nominal bit rate clock | 5 MHz | 5 MHz |
| Time quanta per bit | 10 | 10 |
| TSEG1 | 7 tq | 7 tq |
| TSEG2 | 2 tq | 2 tq |
| SJW | 2 tq | 2 tq |
| Sample point | 80% | 80% |
| Actual bit rate | 500.000 kbps | 500.000 kbps |

## Key Notes

- **Common-mode chokes** (100 uH) on every transceiver for EMC compliance
- **PESD1CAN TVS diode arrays** between chokes and bus connectors for ESD protection
- **Bus termination**: 120 ohm at each physical end (CVC and SC)
- **CAN GND**: Reference ground wire runs alongside CAN_H/CAN_L twisted pair

## Pin References

| ECU | TX Pin | RX Pin | AF | Transceiver |
|-----|--------|--------|-----|-------------|
| CVC | PA12 | PA11 | AF9 | TJA1051T/3 |
| FZC | PA12 | PA11 | AF9 | TJA1051T/3 |
| RZC | PA12 | PA11 | AF9 | TJA1051T/3 |
| SC | DCAN1TX (J5) | DCAN1RX (J5) | — | SN65HVD230 |

## BOM References

| Component | BOM # |
|-----------|-------|
| TJA1051T/3 modules (x3) | #6 |
| SN65HVD230 module | #7 |
| CANable 2.0 (x2) | #8 |
| 120 ohm terminators (x4) | #9 |
| Common-mode chokes (x4) | #10 |
| PESD1CAN TVS diodes (x4) | #11 |
| 22 AWG twisted pair wire | #12 |
| 3-position screw terminals | #63 |
