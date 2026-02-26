# 06 — FZC Circuits

**Block**: Front Zone Controller — CAN, AS5048A, servos, TFMini, buzzer, WDT
**Source**: HWDES Section 5.2

## CAN Transceiver Circuit

Same as CVC (see `05-cvc-circuits.md`) using FDCAN1 on PA12 (TX) and PA11 (RX) with TJA1051T/3. **No 120 ohm termination** on FZC (not at a bus end).

## Steering Angle Sensor SPI Circuit

```
  STM32G474RE                  AS5048A (Steering)
  +-----------+                +----------+
  |           |                |          |
  | SPI2_SCK  +--- PB13 ----->| CLK      |
  | (AF5)     |                |          |
  | SPI2_MISO +--- PB14 --<---| DO   VDD |--[100nF]-- 3.3V
  | (AF5)     |                |      |   |
  | SPI2_MOSI +--- PB15 ----->| DI   GND |-- GND
  | (AF5)     |                |          |
  |   GPIO    +--- PB12 ----->| CSn      |
  |           |     |          +----------+
  +-----------+ [10k to 3.3V]

  Single sensor on SPI2 bus (SPI2 avoids SB21 solder bridge conflict on Nucleo-64).
  Sensor mounted on steering servo output shaft.
  SPI cable routed with 10mm separation from servo power lines.
```

## Steering Servo PWM Circuit

```
  STM32G474RE                       MG996R Steering Servo
  +-----------+                     +------------------+
  |           |                     |                  |
  | TIM2_CH1  +--- PA0 ----------->| Signal (Orange)  |
  | (AF1)     |     (3.3V PWM,     |                  |
  |           |      50 Hz)        | VCC (Red)        |---+
  +-----------+                    |                  |   |
                                   | GND (Brown)      |---+---GND
                                   +------------------+   |
                                                          |
                                              [6V Regulator]---[3A Fuse]---12V Actuator Rail
                                                          |
                                                      [470uF]
                                                          |
                                                         GND

  PWM: 50 Hz, 1.0-2.0 ms pulse width.
  Servo powered from 6V regulated rail (from 12V actuator bus via kill relay).
  470uF bulk cap on 6V rail for servo inrush.
```

## Brake Servo PWM Circuit

Same topology as steering servo using TIM2_CH2 on PA1. Separate 3A fuse. Shares the 6V regulated rail with the steering servo (regulator rated for 4A total).

## TFMini-S Lidar UART Circuit

```
  STM32G474RE                   TFMini-S Lidar Sensor
  +-----------+                 +------------------+
  |           |                 |                  |
  | USART2_TX +--- PA2 ------->| RX (Green)       |
  | (AF7)     |                 |                  |
  | USART2_RX +--- PA3 ---[TVS]<--| TX (White)    |
  | (AF7)     |                 |                  |
  +-----------+            5V-->| VCC (Red)        |
                                |                  |
                           GND->| GND (Black)      |
                                +------------------+

  UART: 115200 baud, 8N1.
  TFMini-S TX is 3.3V LVTTL (direct connect, no level shifter).
  TFMini-S power: 5V from external 5V rail with 470uF bulk cap.
  TVS diode (3.3V bidirectional) on USART2_RX for ESD protection.
  4-pin keyed connector prevents reverse polarity.
  DMA enabled on USART2_RX for background frame reception.
```

## Buzzer Driver Circuit

```
  3.3V
    |
  [Active Piezo Buzzer (85dB @ 10cm)]
    |
  Drain -- [2N7002 N-MOSFET] -- Source -- GND
                |
             Gate
                |
           [10k pulldown to GND]
                |
  PB4 (GPIO) ---+

  GPIO HIGH = buzzer ON, GPIO LOW = buzzer OFF.
  10k pulldown ensures buzzer OFF on MCU reset.
  Schottky diode (1N5819) across buzzer for inductive kickback.
```

## Watchdog Circuit

Same as CVC (see `05-cvc-circuits.md`) using PB0 as WDI GPIO.

## Pin Summary

| # | Function | Pin | AF | Direction | Net Name | ASIL |
|---|----------|-----|----|-----------|----------|------|
| 1 | CAN TX | PA12 | AF9 | OUT | FZC_CAN_TX | D |
| 2 | CAN RX | PA11 | AF9 | IN | FZC_CAN_RX | D |
| 3 | Steer servo PWM | PA0 | AF1 | OUT | FZC_STEER_PWM | D |
| 4 | Brake servo PWM | PA1 | AF1 | OUT | FZC_BRAKE_PWM | D |
| 5 | Lidar UART TX | PA2 | AF7 | OUT | FZC_LIDAR_TX | C |
| 6 | Lidar UART RX | PA3 | AF7 | IN | FZC_LIDAR_RX | C |
| 7 | SPI2 SCK | PB13 | AF5 | OUT | FZC_SPI2_SCK | D |
| 8 | SPI2 MISO | PB14 | AF5 | IN | FZC_SPI2_MISO | D |
| 9 | SPI2 MOSI | PB15 | AF5 | OUT | FZC_SPI2_MOSI | D |
| 10 | Steering CS | PB12 | — | OUT | FZC_STEER_CS | D |
| 11 | Buzzer drive | PB4 | — | OUT | FZC_BUZZER | B |
| 12 | WDT feed | PB0 | — | OUT | FZC_WDT_WDI | D |
| 13 | LED Green | PB5 | — | OUT | FZC_LED_GRN | QM |
| 14 | LED Red | PB1 | — | OUT | FZC_LED_RED | QM |

### Solder Bridge Modifications

| Bridge | Action | Reason |
|--------|--------|--------|
| SB63 | REMOVE | Free PA2 from ST-LINK VCP TX (needed for lidar UART TX) |
| SB65 | REMOVE | Free PA3 from ST-LINK VCP RX (needed for lidar UART RX) |

## BOM References

| Component | BOM # |
|-----------|-------|
| STM32G474RE Nucleo-64 | #1 |
| TJA1051T/3 module | #6 |
| AS5048A module | #13 |
| Diametric magnet | #14 |
| TFMini-S lidar | #15 |
| MG996R servos (x2) | #21 |
| TPS3823DBVR | #22 |
| SOT-23-5 breakout | #23 |
| Active piezo buzzer | #32 |
| 2N7002 MOSFET | #33 |
| 6V regulators (x2) | #42 |
| Glass tube fuse 3A (x2) | #40 |
| 470uF capacitors | #48 |
| 100nF capacitors | #43 |
| 10k resistors | #49 |
| 1N5819 Schottky | #57 |
| 3.3V TVS diode | #56 |
