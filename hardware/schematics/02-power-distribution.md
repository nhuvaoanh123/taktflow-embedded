# 02 — Power Distribution

**Block**: 12V rail, buck converters, fuses, kill relay gating
**Source**: HWDES Section 7

## Power Distribution Diagram

```
  [12V Bench PSU 10A]
         |
    [SB560 Schottky] -- reverse polarity protection
         |
    [10A ATC Fuse] -- main rail fuse
         |
    ===== 12V MAIN RAIL (16 AWG) =====
         |         |         |         |         |         |
     [LM2596]  [LM2596]  [Nucleo]  [Nucleo]  [Nucleo]  [LaunchPad]
     12->5V    12->3.3V   Vin(CVC)  Vin(FZC)  Vin(RZC)  Vin(SC)
      |          |
    5V Rail    3.3V Rail
    (3A PTC)   (1A PTC)
      |          |
    TFMini     Sensors
    RPi4       Transceivers
               Watchdogs
               LEDs
         |
    [Kill Relay Coil Circuit] (SC controlled)
         |
    [30A ATC Fuse]
         |
    ===== 12V ACTUATOR RAIL (16 AWG) =====
         |              |              |
    [BTS7960]       [6V Reg #1]    [6V Reg #2]
    Motor Driver    Steering Servo  Brake Servo
    (25A peak)      (2A peak)       (2A peak)
         |              |              |
    [DC Motor]     [MG996R]        [MG996R]
                   [3A Fuse]       [3A Fuse]
```

## Power Budget

| Domain | Voltage | Current (typical) | Current (peak) | Source |
|--------|---------|-------------------|----------------|--------|
| CVC logic | 3.3V | 230 mA | 250 mA | Nucleo LDO |
| FZC logic | 3.3V | 194 mA | 220 mA | Nucleo LDO |
| RZC logic | 3.3V | 162 mA | 180 mA | Nucleo LDO |
| SC logic | 3.3V | 150 mA | 200 mA | LaunchPad LDO |
| External 3.3V rail | 3.3V | 74 mA | 100 mA | LM2596 buck |
| External 5V rail | 5V | 700 mA | 1340 mA | LM2596 buck |
| Motor (12V actuator rail) | 12V | 5 A | 25 A | Kill relay |
| Servos (6V from 12V act.) | 6V | 1 A | 5 A | 6V regulators |
| Kill relay coil | 12V | 100 mA | 170 mA | Main rail |
| **Total from 12V supply** | **12V** | **~3.5 A** | **~8.5 A** | |

Peak current (8.5A) is within the 10A PSU rating. Motor stall current (25A) is transient (<100 ms) before overcurrent protection triggers.

## Key Notes

- **Reverse polarity**: SB560 Schottky at PSU entry (~0.5V drop at full load)
- **PTC fuses**: Resettable PTC on 5V rail (3A) and 3.3V rail (1A) for overcurrent protection
- **Actuator isolation**: Kill relay gates ALL actuator power — SC-controlled fail-safe
- **Servo fuses**: 3A fast-blow glass fuses on each 6V servo feed
- **Wire gauge**: 16 AWG for 12V main rail and actuator rail, 22 AWG for signal/logic

## BOM References

| Component | BOM # |
|-----------|-------|
| 12V 10A bench PSU | #34 |
| LM2596 buck modules (x2) | #35 |
| SB560 Schottky | #36 |
| ATC fuse holders (x2) | #37 |
| ATC fuse 10A | #38 |
| ATC fuse 30A | #39 |
| Glass tube fuse 3A (x4) | #40 |
| Glass tube fuse holders (x2) | #41 |
| 6V regulator modules (x2) | #42 |
| Resettable PTC 3A (x2) | #58 |
| 16 AWG wire | #68 |
| 18 AWG wire | #67 |
