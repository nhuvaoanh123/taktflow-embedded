# 01 — System Block Diagram

**Block**: Top-level system overview
**Source**: HWDES Section 4

## System Block Diagram

```
  +========================================================================+
  |                         12V BENCH SUPPLY (10A)                         |
  +====+==============+============+============+============+=============+
       |              |            |            |            |
   [SB560 Schottky - Reverse Polarity Protection]
       |
       +-- [10A Fuse] -- 12V MAIN RAIL
       |
       +------+------+------+------+------+------+
       |      |      |      |      |      |      |
    LM2596  LM2596  Nucleo Nucleo Nucleo  LP     |
    12->5V  12->3.3V LDO   LDO    LDO   LDO    |
     |       |      (CVC)  (FZC)  (RZC)  (SC)   |
     |       |       |      |      |      |      |
    5V      3.3V    3.3V   3.3V   3.3V   3.3V   |
    Rail    Rail     |      |      |      |      |
     |       |       |      |      |      |      |
   TFMini  AS5048A  CVC    FZC    RZC    SC     |
   RPi4    ACS723   MCU    MCU    MCU    MCU    |
           TJA1051                SN65HVD         |
           TPS3823                                |
           NTC divs                               |
                                                  |
       +--[Kill Relay (30A SPST-NO)]--[30A Fuse]--+
       |
       +-- 12V ACTUATOR RAIL (gated)
       |
       +------+------+------+
       |      |      |      |
    BTS7960  Steer  Brake  (Relay coil
    Motor    Servo  Servo   is on 12V
    Driver   6V reg 6V reg  main rail)
     |
    DC Motor (12V)

  ==========+===========+===========+===========+=====  CAN BUS (500 kbps)
            |           |           |           |
       +----+----+ +----+----+ +----+----+ +----+----+
       |  CVC    | |  FZC    | |  RZC    | |   SC    |
       | TJA1051 | | TJA1051 | | TJA1051 | | SN65HVD |
       | [120R]  | |         | |         | | [120R]  |
       +---------+ +---------+ +---------+ +---------+
                                                |
                                           CANable 2.0
                                           (RPi USB)
                                                |
                                         Raspberry Pi 4
                                                |
                                           MQTT/TLS
                                                |
                                            AWS Cloud
                                         (IoT Core ->
                                          Timestream ->
                                           Grafana)
```

## Key Points

- **Two power domains**: 12V main rail (always on) and 12V actuator rail (gated by kill relay)
- **SC powered independently** of kill relay — maintains fault monitoring even when actuators are off
- **CAN bus**: 500 kbps, 120 ohm termination at CVC (start) and SC (end)
- **Reverse polarity protection**: SB560 Schottky at PSU entry

## Pin References

| ECU | CAN TX | CAN RX | Transceiver |
|-----|--------|--------|-------------|
| CVC | PA12 (AF9) | PA11 (AF9) | TJA1051T/3 |
| FZC | PA12 (AF9) | PA11 (AF9) | TJA1051T/3 |
| RZC | PA12 (AF9) | PA11 (AF9) | TJA1051T/3 |
| SC | DCAN1TX (J5) | DCAN1RX (J5) | SN65HVD230 |

## BOM References

| Component | BOM # |
|-----------|-------|
| STM32G474RE Nucleo-64 (x3) | #1 |
| TMS570LC43x LaunchPad | #2 |
| Raspberry Pi 4 | #3 |
| TJA1051T/3 (x3) | #6 |
| SN65HVD230 | #7 |
| CANable 2.0 (x2) | #8 |
| 120 ohm terminators | #9 |
| SB560 Schottky | #36 |
| 10A ATC fuse | #38 |
| 30A ATC fuse | #39 |
| Kill relay (30A SPST-NO) | #24 |
| 12V bench PSU | #34 |
| LM2596 buck converters (x2) | #35 |
