# 08 — SC Circuits

**Block**: Safety Controller — DCAN1, kill relay, fault LEDs, WDT, power
**Source**: HWDES Section 5.4

## DCAN1 CAN Interface (Listen-Only)

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
  TMS570 TX drives recessive (no effect on bus).
  TMS570 RX receives all frames without acknowledging.
  SN65HVD230 is TI part (same vendor as TMS570).
  120R termination at SC (SC is at one end of the bus).
  Using DCAN1 via edge connector (NOT DCAN4 due to HALCoGen bug).
```

## Kill Relay Circuit

See `04-safety-chain.md` for full schematic. SC GIO_A0 drives IRLZ44N MOSFET gate to control the energize-to-run relay.

## Fault LED Circuit

```
  SC GIO Pins                    LEDs

  GIO_A1 ---[330R]--- LED_CVC (Red, 3mm)    --- GND
  GIO_A2 ---[330R]--- LED_FZC (Red, 3mm)    --- GND
  GIO_A3 ---[330R]--- LED_RZC (Red, 3mm)    --- GND
  GIO_A4 ---[330R]--- LED_SYS (Amber, 3mm)  --- GND

  GIO_B1 ---[330R]--- LED_HB (Green, 3mm)   --- GND  (heartbeat/status)

  LED current: I = (3.3V - 1.8V) / 330R = 4.5 mA.
  Lamp test during startup: all LEDs ON for 500 ms.
```

## External Watchdog Circuit

```
                    TPS3823DBVR
                    +-----------+
  3.3V --[100nF]--->| VDD    MR |--- 3.3V
                    |           |
  GIO_A5 (GPIO) -->| WDI  RESET|---[100nF]--- TMS570 nRST
                    |           |
                    | CT    GND |
                    +---+-------+
                        |
                     [100nF]
                        |
                       GND

  Same configuration as STM32 ECU watchdogs.
  TPS3823 provides independent reset mechanism for faults not caught by lockstep.
  Lockstep detects computation errors; TPS3823 detects hang/timing faults.
```

## Power Supply Design

```
  12V Main Rail (NOT through kill relay)
       |
  [Schottky diode - reverse polarity protection]
       |
  [LM1117-3.3 Linear Regulator]
       |
  3.3V (SC dedicated rail)
       |
       +--- TMS570LC43x (via LaunchPad Vin or direct 3.3V header)
       +--- SN65HVD230 CAN transceiver
       +--- TPS3823 watchdog
       +--- Fault LEDs (5x, through current-limiting resistors)
       +--- Kill relay MOSFET gate circuit

  ALTERNATIVELY: SC powered via LaunchPad USB (5V USB -> onboard 3.3V LDO).

  CRITICAL: SC power is independent of the kill relay.
  When kill relay opens, SC remains powered to:
    - Log DTCs
    - Maintain fault LED indication
    - Continue watchdog feed (if firmware is healthy)
```

## Pin Summary

| # | Function | Pin | Connector | Direction | Net Name | ASIL |
|---|----------|-----|-----------|-----------|----------|------|
| 1 | CAN TX | DCAN1TX | J5 (edge) | OUT | SC_CAN_TX | D |
| 2 | CAN RX | DCAN1RX | J5 (edge) | IN | SC_CAN_RX | D |
| 3 | Kill relay control | GIO_A0 | J3-1 (HDR1) | OUT | SC_KILL_RELAY | D |
| 4 | CVC fault LED | GIO_A1 | J3-2 (HDR1) | OUT | SC_LED_CVC | B |
| 5 | FZC fault LED | GIO_A2 | J3-3 (HDR1) | OUT | SC_LED_FZC | B |
| 6 | RZC fault LED | GIO_A3 | J3-4 (HDR1) | OUT | SC_LED_RZC | B |
| 7 | System fault LED | GIO_A4 | J3-5 (HDR1) | OUT | SC_LED_SYS | B |
| 8 | WDT feed | GIO_A5 | J3-6 (HDR1) | OUT | SC_WDT_WDI | D |
| 9 | Heartbeat LED | GIO_B1 | J12 (onboard) | OUT | SC_LED_HB | QM |

### DCAN1 Configuration Notes

- DCAN1 module enabled via HALCoGen
- Silent mode: DCAN1 TEST register bit 3 = 1
- Bit timing: 500 kbps with 80% sample point
- Mailboxes: objects 1-15 for heartbeat (0x010-0x012), vehicle state (0x100), torque request (0x101), motor current (0x301)
- Edge connector wiring: DCAN1TX/RX on J5 -> SN65HVD230 breakout

## BOM References

| Component | BOM # |
|-----------|-------|
| TMS570LC43x LaunchPad (LAUNCHXL2-570LC43) | #2 |
| SN65HVD230 module | #7 |
| TPS3823DBVR | #22 |
| SOT-23-5 breakout | #23 |
| 30A automotive relay | #24 |
| IRLZ44N MOSFET | #25 |
| 1N4007 diodes (x2) | #26 |
| LED 3mm red (x3) | #29 |
| LED 3mm green (x1) | #30 |
| LED 3mm amber (x1) | #31 |
| 330 ohm resistors | #51 |
| 100 ohm resistors | #52 |
| 10k resistors | #49 |
| 100nF capacitors | #43 |
| 120 ohm terminator | #9 |
| Common-mode choke | #10 |
| Relay socket | #72 |
