# 05 — CVC Circuits

**Block**: Central Vehicle Computer — CAN, dual AS5048A, OLED, E-stop, WDT, power
**Source**: HWDES Section 5.1

## CAN Transceiver Circuit

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

  120R termination resistor at CVC end of bus.
```

## Dual Pedal Sensor SPI Circuit

```
  STM32G474RE                  AS5048A #1              AS5048A #2
  +-----------+                +----------+            +----------+
  |           |                |          |            |          |
  | SPI1_SCK  +--- PA5 ------>| CLK      |    +------>| CLK      |
  | (AF5)     |                |          |    |       |          |
  | SPI1_MISO +--- PA6 ---<---| DO   VDD |--[100nF]   | DO   VDD |--[100nF]
  | (AF5)     |                |      |   |   |       |      |   |   |
  | SPI1_MOSI +--- PA7 ------>| DI   GND |   |  +--->| DI   GND |   |
  | (AF5)     |                |          |   |  |    |          |   |
  |           |                |     CSn  |   |  |    |     CSn  |   |
  |   GPIO    +--- PA4 ------>|  (act-lo) |   |  |    |  (act-lo) |  |
  |           |     |          +----------+   |  |    +----------+   |
  |   GPIO    +--- PA15 ------+---------------+--+--->               |
  |           |     |                                                |
  +-----------+     |                                                |
                [10k to 3.3V] pull-up on PA4                     [10k to 3.3V]
                                                                 pull-up on PA15

  Both sensors share SPI1 bus (SCK, MISO, MOSI).
  Independent chip-selects (PA4, PA15) with 10k pull-ups.
  Sensors powered from external 3.3V rail with local 100nF decoupling each.
```

## OLED Display I2C Circuit

```
  STM32G474RE                    SSD1306 128x64 OLED
  +-----------+                  +-------------------+
  |           |                  |                   |
  | I2C1_SCL  +--- PB8 ---+---->| SCL      VCC(3.3V)|--[10uF]--[100nF]--3.3V
  | (AF4)     |            |     |                   |
  | I2C1_SDA  +--- PB9 --+|-+-->| SDA          GND  |-- GND
  | (AF4)     |           || |   |                   |
  +-----------+           || |   | RST               |-- GPIO (optional)
                          || |   | ADDR = 0x3C       |
                       [4.7k]|   +-------------------+
                          |[4.7k]
                       3.3V  3.3V

  Pull-ups: 4.7k ohm on SCL and SDA to 3.3V (external).
  I2C speed: 400 kHz (Fast Mode). Address: 0x3C (7-bit).
```

## E-Stop Button Circuit

```
  3.3V (internal pull-up on PC13, ~40k)
    |
    +--- PC13 (EXTI, rising-edge) ---[10k series R]---+--- [100nF] --- GND
    |                                                  |    (RC debounce)
    |                                              [TVS 3.3V]
    |                                                  |
    +--- [NC Push Button] --- GND                     GND

  NC button: fail-safe — broken wire = E-stop activated.
  RC debounce: tau = 10k * 100nF = 1 ms.
```

## Watchdog Circuit

```
                    TPS3823DBVR
                    +-----------+
  3.3V --[100nF]--->| VDD    MR |--- 3.3V (MR tied high = no manual reset)
                    |           |
  PB0 (GPIO) ----->| WDI  RESET|---[100nF]--- STM32 NRST
                    |           |         |
                    | CT    GND |       (open-drain, active-low)
                    +---+-------+
                        |
                     [100nF] (sets timeout = 1.6 sec)
                        |
                       GND

  Firmware toggles PB0 at regular intervals (conditioned on self-checks).
  If PB0 stops toggling for 1.6 sec, TPS3823 pulls RESET low -> MCU resets.
```

## Power Supply Design

```
  12V Main Rail
       |
  [Nucleo Vin pin] ---> Nucleo onboard LDO (LD39050) ---> 3.3V (MCU power)

  External 3.3V rail ---> SPI sensors (AS5048A x2)
                     ---> CAN transceiver (TJA1051T/3)
                     ---> TPS3823 watchdog
                     ---> I2C OLED (SSD1306)
                     ---> Status LEDs

  Power domains separated:
  - MCU powered from Nucleo onboard LDO
  - Peripherals powered from external 3.3V rail
```

## Pin Summary

| # | Function | Pin | AF | Direction | Net Name | ASIL |
|---|----------|-----|----|-----------|----------|------|
| 1 | CAN TX | PA12 | AF9 | OUT | CVC_CAN_TX | D |
| 2 | CAN RX | PA11 | AF9 | IN | CVC_CAN_RX | D |
| 3 | SPI1 SCK | PA5 | AF5 | OUT | CVC_SPI1_SCK | D |
| 4 | SPI1 MISO | PA6 | AF5 | IN | CVC_SPI1_MISO | D |
| 5 | SPI1 MOSI | PA7 | AF5 | OUT | CVC_SPI1_MOSI | D |
| 6 | Pedal CS1 | PA4 | — | OUT | CVC_PED_CS1 | D |
| 7 | Pedal CS2 | PA15 | — | OUT | CVC_PED_CS2 | D |
| 8 | I2C1 SCL | PB8 | AF4 | BI | CVC_I2C1_SCL | B |
| 9 | I2C1 SDA | PB9 | AF4 | BI | CVC_I2C1_SDA | B |
| 10 | E-stop | PC13 | — | IN | CVC_ESTOP | B |
| 11 | WDT feed | PB0 | — | OUT | CVC_WDT_WDI | D |
| 12 | LED Green | PB4 | — | OUT | CVC_LED_GRN | QM |
| 13 | LED Red | PB5 | — | OUT | CVC_LED_RED | QM |

## BOM References

| Component | BOM # |
|-----------|-------|
| STM32G474RE Nucleo-64 | #1 |
| TJA1051T/3 module | #6 |
| AS5048A modules (x2) | #13 |
| Diametric magnets (x2) | #14 |
| TPS3823DBVR | #22 |
| SOT-23-5 breakout | #23 |
| SSD1306 OLED | #28 |
| E-stop button | #27 |
| 120 ohm terminator | #9 |
| 100nF capacitors | #43 |
| 10uF capacitors | #45 |
| 10k resistors | #49 |
| 4.7k resistors | #53 |
| 330 ohm resistors | #51 |
| 3.3V TVS diode | #56 |
| Common-mode choke | #10 |
| PESD1CAN TVS | #11 |
