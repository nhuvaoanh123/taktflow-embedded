# 07 — RZC Circuits

**Block**: Rear Zone Controller — CAN, BTS7960, ACS723, NTC, battery, encoder, WDT
**Source**: HWDES Section 5.3

> **Note**: On the RZC, the watchdog WDI pin is PB4 (not PB0). PB0 is used for BTS7960 R_EN on this ECU.

## CAN Transceiver Circuit

Same as CVC (see `05-cvc-circuits.md`) using FDCAN1 on PA12 (TX) and PA11 (RX) with TJA1051T/3. If RZC is at a physical bus end, include 120 ohm termination.

## BTS7960 H-Bridge Motor Driver Interface

```
  STM32G474RE                    BTS7960 Module
  +-----------+                  +------------------+
  |           |                  |                  |
  | TIM1_CH1  +--- PA8 -------->| RPWM             |
  | (AF6)     |   (20 kHz PWM)  |                  |
  | TIM1_CH2  +--- PA9 -------->| LPWM             |
  | (AF6)     |   (20 kHz PWM)  |         M+  M-   |---[Motor]
  |           |                  |                  |
  |   GPIO    +--- PB0 -------->| R_EN             |
  |           |     |            |                  |
  |   GPIO    +--- PB1 -------->| L_EN             |
  |           |     |            |                  |
  |  ADC1_CH3 +--- PA3 ---<-----| R_IS (current)   |
  |           |                  |                  |
  |  ADC1_CH4 +--- PA4 ---<-----| L_IS (current)   |
  |           |                  |                  |
  +-----------+                  | B+  B-           |
                                 +--+-----+---------+
                                    |     |
                                12V Act.  GND
                                Rail

  R_EN, L_EN: 10k pull-down resistors to GND (fail-safe disabled on reset).
  RPWM, LPWM: 20 kHz PWM (above audible), hardware dead-time = 10 us minimum.
  BTS7960 VCC from 12V actuator rail (through kill relay and 30A fuse).
  IS_R, IS_L: current sense outputs for backup monitoring (ACS723 is primary).
```

## ACS723 Current Sensor Circuit

```
                    ACS723LLCTR-20AB-T
                    +------------------+
  Motor supply ---->| IP+          VCC |--- 3.3V ---[100nF]
  (from BTS7960)    |                  |
  Motor return ---<-| IP-         VIOUT|---[1nF]---+--- PA0 (ADC1_CH1)
                    |                  |           |
                    |              GND |     [100nF filter]
                    +------------------+           |
                                                  GND

  Galvanic isolation between high-current path (IP+/IP-) and signal output.
  Sensitivity: 100 mV/A (ACS723LLCTR-20AB-T, 20A variant).
  Zero-current output: VCC/2 = 1.65V (bidirectional sensing).
  Practical range: +/-16.5A before output saturates.
  1nF capacitor: bandwidth filter (80 kHz).
  100nF capacitor: anti-aliasing at ADC input (1.6 kHz cutoff).
  Zener clamp (BZX84C3V3) across ADC input for overvoltage protection.
```

## NTC Temperature Sensing Circuit

```
  3.3V
    |
  [10k, 1% precision fixed resistor]
    |
    +--- PA1 (ADC1_CH2) ---[100nF]--- GND
    |
  [10k NTC (B=3950)]
    |
   GND

  At 25C: R_NTC = 10k, V_out = 1.65V (midscale)
  At 60C: R_NTC = 2.49k, V_out = 0.66V
  At 100C: R_NTC = 0.68k, V_out = 0.21V
  Open NTC: V_out = 3.3V (reads as < -30C -> sensor fault)
  Short NTC: V_out = 0V (reads as > 150C -> sensor fault)

  Second NTC on PA2 (ADC1_CH3) with identical circuit for board temperature.
```

## Battery Voltage Monitoring Circuit

```
  12V Battery Bus
    |
  [47k, 1% resistor]
    |
    +--- PA3 (ADC1_CH4) ---[100nF]--- GND
    |                           |
  [10k, 1% resistor]       [BZX84C3V3 Zener]
    |                           |
   GND                        GND

  Voltage divider: V_out = V_bat * 10k / (47k + 10k) = V_bat * 0.175
  At 12V: V_out = 2.11V
  At 14.4V: V_out = 2.53V
  Max input before Zener clamps: 18.8V
```

## Motor Encoder Interface

```
  Motor Encoder (open-collector outputs)
  +----------+
  |          |
  | Ch A     |---[10k to 3.3V]---[TVS 3.3V]--- PB6 (TIM4_CH1, AF2)
  |          |
  | Ch B     |---[10k to 3.3V]---[TVS 3.3V]--- PB7 (TIM4_CH2, AF2)
  |          |
  | VCC      |--- 5V or 3.3V (per encoder spec)
  |          |
  | GND      |--- GND
  +----------+

  TIM4 configured in encoder mode (both edges, 4x counting).
  Hardware input filter ICF = 0x0F (maximum, 8 samples at fDTS/32).
  10k pull-ups required for open-collector encoder outputs.
  TVS diodes for ESD protection (motor is a noise source).
```

## Watchdog Circuit

Same as CVC (see `05-cvc-circuits.md`) but using **PB4** as WDI GPIO (PB0 used for BTS7960 R_EN).

## Pin Summary

| # | Function | Pin | AF | Direction | Net Name | ASIL |
|---|----------|-----|----|-----------|----------|------|
| 1 | CAN TX | PA12 | AF9 | OUT | RZC_CAN_TX | D |
| 2 | CAN RX | PA11 | AF9 | IN | RZC_CAN_RX | D |
| 3 | Motor RPWM | PA8 | AF6 | OUT | RZC_MOT_RPWM | D |
| 4 | Motor LPWM | PA9 | AF6 | OUT | RZC_MOT_LPWM | D |
| 5 | Motor R_EN | PB0 | — | OUT | RZC_MOT_REN | D |
| 6 | Motor L_EN | PB1 | — | OUT | RZC_MOT_LEN | D |
| 7 | Encoder A | PB6 | AF2 | IN | RZC_ENC_A | C |
| 8 | Encoder B | PB7 | AF2 | IN | RZC_ENC_B | C |
| 9 | Current sense | PA0 | AN | AN | RZC_CURR_SENSE | A |
| 10 | Motor temp | PA1 | AN | AN | RZC_MOT_TEMP | A |
| 11 | Board temp | PA2 | AN | AN | RZC_BRD_TEMP | QM |
| 12 | Battery voltage | PA3 | AN | AN | RZC_VBAT | QM |
| 13 | BTS7960 IS_R | PB15 | AN | AN | RZC_ISR | A |
| 14 | BTS7960 IS_L | PA4 | AN | AN | RZC_ISL | A |
| 15 | WDT feed | PB4 | — | OUT | RZC_WDT_WDI | D |
| 16 | LED Green | PB5 | — | OUT | RZC_LED_GRN | QM |
| 17 | LED Red | PB3 | — | OUT | RZC_LED_RED | QM |

> **Note**: PB3 (LED Red) is shared with SWO debug trace. If SWO debug is needed on RZC, relocate LED Red to another available pin (e.g., PC10). See pin-mapping.md Section 6.

### Solder Bridge Modifications

| Bridge | Action | Reason |
|--------|--------|--------|
| SB63 | REMOVE | Free PA2 from ST-LINK VCP TX (needed for ADC1_IN3) |
| SB65 | REMOVE | Free PA3 from ST-LINK VCP RX (needed for ADC1_IN4) |

## BOM References

| Component | BOM # |
|-----------|-------|
| STM32G474RE Nucleo-64 | #1 |
| TJA1051T/3 module | #6 |
| BTS7960 H-bridge module | #20 |
| 12V DC motor | #19 |
| Quadrature encoder | #18 |
| ACS723 module | #16 |
| NTC 10k thermistors (x3) | #17 |
| TPS3823DBVR | #22 |
| SOT-23-5 breakout | #23 |
| 100nF capacitors | #43 |
| 1nF capacitors | #44 |
| 10k resistors | #49 |
| 47k resistors | #50 |
| BZX84C3V3 Zener diodes | #55 |
| 3.3V TVS diodes | #56 |
| Common-mode choke | #10 |
| PESD1CAN TVS | #11 |
