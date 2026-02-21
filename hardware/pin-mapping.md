---
document_id: PIN-MAP
title: "Pin Mapping"
version: "1.0"
status: draft
date: 2026-02-21
---

# Pin Mapping

## 1. Purpose

Complete pin assignment for all physical ECUs (CVC, FZC, RZC, SC). This document serves as the hardware-software interface reference for firmware development and is the authoritative source for which MCU pin connects to which external component. Cross-referenced with the hardware design (HWDES), hardware safety requirements (HSR), and BOM.

## 2. Referenced Documents

| Document ID | Title | Version |
|-------------|-------|---------|
| HWDES | Hardware Design | 1.0 |
| HSR | Hardware Safety Requirements | 1.0 |
| HWREQ | Hardware Requirements | 1.0 |
| SYSARCH | System Architecture | 1.0 |

## 3. Pin Naming Convention

- **Pin Name**: STM32 port/pin (e.g., PA5) or TMS570 GIO/DCAN pin name
- **Nucleo Pin**: Physical pin number on the Nucleo-64 board morpho or Arduino header
- **AF**: Alternate function number for STM32 (e.g., AF5 for SPI1)
- **Direction**: IN (input), OUT (output), BI (bidirectional), AN (analog input)
- **Voltage**: Logic level (3.3V, 5V) or analog range
- **ASIL**: ASIL level of the function using this pin
- **Net Name**: Signal name on the schematic for cross-reference

---

## 4. CVC -- STM32G474RE Nucleo-64

### 4.1 Pin Assignment Table

| # | Function | Peripheral | Pin Name | AF | Nucleo Arduino | Nucleo Morpho | Direction | Voltage | External Component | Net Name | ASIL |
|---|----------|-----------|----------|-----|----------------|---------------|-----------|---------|-------------------|----------|------|
| 1 | CAN TX | FDCAN1 | PA12 | AF9 | D2 | CN10-12 | OUT | 3.3V | TJA1051T/3 TXD | CVC_CAN_TX | D |
| 2 | CAN RX | FDCAN1 | PA11 | AF9 | D10 | CN10-14 | IN | 3.3V | TJA1051T/3 RXD | CVC_CAN_RX | D |
| 3 | Pedal SPI SCK | SPI1 | PA5 | AF5 | D13 | CN10-11 | OUT | 3.3V | AS5048A x2 CLK | CVC_SPI1_SCK | D |
| 4 | Pedal SPI MISO | SPI1 | PA6 | AF5 | D12 | CN10-13 | IN | 3.3V | AS5048A x2 DO | CVC_SPI1_MISO | D |
| 5 | Pedal SPI MOSI | SPI1 | PA7 | AF5 | D11 | CN10-15 | OUT | 3.3V | AS5048A x2 DI | CVC_SPI1_MOSI | D |
| 6 | Pedal CS1 | GPIO | PA4 | -- | A2 | CN7-32 | OUT | 3.3V | AS5048A #1 CSn (10k PU) | CVC_PED_CS1 | D |
| 7 | Pedal CS2 | GPIO | PA15 | -- | -- | CN7-17 | OUT | 3.3V | AS5048A #2 CSn (10k PU) | CVC_PED_CS2 | D |
| 8 | OLED SCL | I2C1 | PB8 | AF4 | D15 | CN10-3 | BI | 3.3V | SSD1306 SCL (4.7k PU) | CVC_I2C1_SCL | B |
| 9 | OLED SDA | I2C1 | PB9 | AF4 | D14 | CN10-5 | BI | 3.3V | SSD1306 SDA (4.7k PU) | CVC_I2C1_SDA | B |
| 10 | E-Stop input | EXTI | PC13 | -- | -- | CN7-23 | IN | 3.3V | NC button + RC debounce + TVS | CVC_ESTOP | B |
| 11 | WDT Feed | GPIO | PB0 | -- | D3 | CN10-31 | OUT | 3.3V | TPS3823 WDI | CVC_WDT_WDI | D |
| 12 | Status LED Green | GPIO | PB4 | -- | D5 | CN10-27 | OUT | 3.3V | Green LED + 330R | CVC_LED_GRN | QM |
| 13 | Status LED Red | GPIO | PB5 | -- | D4 | CN10-29 | OUT | 3.3V | Red LED + 330R | CVC_LED_RED | QM |

**Total pins used: 13** (out of 51 available GPIO on STM32G474RE Nucleo-64)

### 4.2 Pin Conflict Check

| Check | Result |
|-------|--------|
| PA5 used for both SPI1_SCK and onboard LED (LD2)? | CONFLICT -- PA5 is the Nucleo onboard LED. Resolved: use PB4 for status LED instead. SPI1_SCK on PA5 takes priority (ASIL D). Onboard LED LD2 is disabled by removing SB21 solder bridge or accepting it blinks with SPI clock. |
| PA11/PA12 used for both FDCAN1 and USB? | No conflict -- USB D+/D- are on PA11/PA12 but FDCAN1 uses the same pins with AF9. USB is not used simultaneously with CAN. Nucleo ST-LINK uses a separate USB connection. |
| PC13 used for both E-stop and onboard button (B1)? | SHARED -- PC13 is the Nucleo user button B1. For the CVC, B1 doubles as the E-stop input in bench testing. For final integration, an external NC button replaces B1. |
| I2C1 on PB8/PB9 vs PB6/PB7? | Using PB8/PB9 (AF4). PB6/PB7 are reserved for potential TIM4 encoder (not used on CVC). |

### 4.3 Unused Pin Handling

All unused GPIO pins on the CVC shall be configured as GPIO output LOW at initialization. This prevents floating inputs from consuming excess current and prevents unintended peripheral activation.

---

## 5. FZC -- STM32G474RE Nucleo-64

### 5.1 Pin Assignment Table

| # | Function | Peripheral | Pin Name | AF | Nucleo Arduino | Nucleo Morpho | Direction | Voltage | External Component | Net Name | ASIL |
|---|----------|-----------|----------|-----|----------------|---------------|-----------|---------|-------------------|----------|------|
| 1 | CAN TX | FDCAN1 | PA12 | AF9 | D2 | CN10-12 | OUT | 3.3V | TJA1051T/3 TXD | FZC_CAN_TX | D |
| 2 | CAN RX | FDCAN1 | PA11 | AF9 | D10 | CN10-14 | IN | 3.3V | TJA1051T/3 RXD | FZC_CAN_RX | D |
| 3 | Steering servo PWM | TIM2_CH1 | PA0 | AF1 | A0 | CN7-28 | OUT | 3.3V | MG996R signal (orange wire) | FZC_STEER_PWM | D |
| 4 | Brake servo PWM | TIM2_CH2 | PA1 | AF1 | A1 | CN7-30 | OUT | 3.3V | MG996R signal (orange wire) | FZC_BRAKE_PWM | D |
| 5 | Lidar UART TX | USART2 | PA2 | AF7 | A7 | CN10-35 | OUT | 3.3V | TFMini-S RX (green wire) | FZC_LIDAR_TX | C |
| 6 | Lidar UART RX | USART2 | PA3 | AF7 | A2 | CN10-37 | IN | 3.3V | TFMini-S TX (white wire) + TVS | FZC_LIDAR_RX | C |
| 7 | Steering angle SPI SCK | SPI2 | PB13 | AF5 | -- | CN10-30 | OUT | 3.3V | AS5048A CLK | FZC_SPI2_SCK | D |
| 8 | Steering angle SPI MISO | SPI2 | PB14 | AF5 | -- | CN10-28 | IN | 3.3V | AS5048A DO | FZC_SPI2_MISO | D |
| 9 | Steering angle SPI MOSI | SPI2 | PB15 | AF5 | -- | CN10-26 | OUT | 3.3V | AS5048A DI | FZC_SPI2_MOSI | D |
| 10 | Steering angle CS | GPIO | PB12 | -- | -- | CN10-16 | OUT | 3.3V | AS5048A CSn (10k PU) | FZC_STEER_CS | D |
| 11 | Buzzer drive | GPIO | PB4 | -- | D5 | CN10-27 | OUT | 3.3V | 2N7002 gate + 10k PD | FZC_BUZZER | B |
| 12 | WDT Feed | GPIO | PB0 | -- | D3 | CN10-31 | OUT | 3.3V | TPS3823 WDI | FZC_WDT_WDI | D |
| 13 | Status LED Green | GPIO | PB5 | -- | D4 | CN10-29 | OUT | 3.3V | Green LED + 330R | FZC_LED_GRN | QM |
| 14 | Status LED Red | GPIO | PB1 | -- | -- | CN10-24 | OUT | 3.3V | Red LED + 330R | FZC_LED_RED | QM |

**Total pins used: 14** (out of 51 available GPIO)

### 5.2 Pin Conflict Check

| Check | Result |
|-------|--------|
| FZC SPI2 on PB13-PB15 conflict? | No conflict -- SPI2 pins (PB13-PB15) have no onboard peripheral. SPI2 avoids SB21 solder bridge conflict (PA5/LD2) on Nucleo-64. |
| PA2/PA3 used for both USART2 and Nucleo ST-LINK VCP? | CONFLICT -- PA2/PA3 are the Nucleo ST-LINK virtual COM port USART2. Resolved: disconnect SB63/SB65 solder bridges on Nucleo to free PA2/PA3 for lidar UART. Debug printf can use LPUART1 or SWO instead. |
| TIM2_CH1 (PA0) and TIM2_CH2 (PA1) on same timer? | By design -- both servo PWMs on TIM2 for synchronized 50 Hz update. |

### 5.3 Unused Pin Handling

All unused GPIO pins configured as GPIO output LOW at initialization.

---

## 6. RZC -- STM32G474RE Nucleo-64

### 6.1 Pin Assignment Table

| # | Function | Peripheral | Pin Name | AF | Nucleo Arduino | Nucleo Morpho | Direction | Voltage | External Component | Net Name | ASIL |
|---|----------|-----------|----------|-----|----------------|---------------|-----------|---------|-------------------|----------|------|
| 1 | CAN TX | FDCAN1 | PA12 | AF9 | D2 | CN10-12 | OUT | 3.3V | TJA1051T/3 TXD | RZC_CAN_TX | D |
| 2 | CAN RX | FDCAN1 | PA11 | AF9 | D10 | CN10-14 | IN | 3.3V | TJA1051T/3 RXD | RZC_CAN_RX | D |
| 3 | Motor PWM (RPWM) | TIM1_CH1 | PA8 | AF6 | D7 | CN10-23 | OUT | 3.3V | BTS7960 RPWM | RZC_MOT_RPWM | D |
| 4 | Motor PWM (LPWM) | TIM1_CH2 | PA9 | AF6 | D8 | CN10-21 | OUT | 3.3V | BTS7960 LPWM | RZC_MOT_LPWM | D |
| 5 | Motor enable R | GPIO | PB0 | -- | D3 | CN10-31 | OUT | 3.3V | BTS7960 R_EN (10k PD) | RZC_MOT_REN | D |
| 6 | Motor enable L | GPIO | PB1 | -- | -- | CN10-24 | OUT | 3.3V | BTS7960 L_EN (10k PD) | RZC_MOT_LEN | D |
| 7 | Encoder A | TIM4_CH1 | PB6 | AF2 | D5 | CN10-17 | IN | 3.3V | Encoder Ch A (10k PU + TVS) | RZC_ENC_A | C |
| 8 | Encoder B | TIM4_CH2 | PB7 | AF2 | -- | CN7-21 | IN | 3.3V | Encoder Ch B (10k PU + TVS) | RZC_ENC_B | C |
| 9 | Current sense | ADC1_IN1 | PA0 | AN | A0 | CN7-28 | AN | 0-3.3V | ACS723 VIOUT (1nF + 100nF + Zener) | RZC_CURR_SENSE | A |
| 10 | Motor temp NTC | ADC1_IN2 | PA1 | AN | A1 | CN7-30 | AN | 0-3.3V | NTC 10k divider (100nF) | RZC_MOT_TEMP | A |
| 11 | Board temp NTC | ADC1_IN3 | PA2 | AN | -- | CN10-35 | AN | 0-3.3V | NTC 10k divider (100nF) | RZC_BRD_TEMP | QM |
| 12 | Battery voltage | ADC1_IN4 | PA3 | AN | A2 | CN10-37 | AN | 0-3.3V | 47k/10k divider (100nF + Zener) | RZC_VBAT | QM |
| 13 | BTS7960 IS_R | ADC1_IN15 | PB15 | AN | -- | CN10-26 | AN | 0-3.3V | BTS7960 R_IS (current sense backup) | RZC_ISR | A |
| 14 | BTS7960 IS_L | ADC1_IN5 | PA4 | AN | -- | CN7-32 | AN | 0-3.3V | BTS7960 L_IS (current sense backup) | RZC_ISL | A |
| 15 | WDT Feed | GPIO | PB4 | -- | D5 | CN10-27 | OUT | 3.3V | TPS3823 WDI | RZC_WDT_WDI | D |
| 16 | Status LED Green | GPIO | PB5 | -- | D4 | CN10-29 | OUT | 3.3V | Green LED + 330R | RZC_LED_GRN | QM |
| 17 | Status LED Red | GPIO | PB3 | -- | D3 | CN10-25 | OUT | 3.3V | Red LED + 330R | RZC_LED_RED | QM |

**Total pins used: 17** (out of 51 available GPIO)

### 6.2 Pin Conflict Check

| Check | Result |
|-------|--------|
| PA0-PA3 used for both ADC and digital functions? | No conflict -- these pins are dedicated to analog inputs (ADC1). No digital alternate functions assigned. |
| PB0 used for WDT on CVC but Motor Enable on RZC? | No conflict -- PB0 is on different physical ECUs. Each Nucleo board has its own PB0. |
| PB6/PB7 for encoder (TIM4) vs I2C1? | By design -- RZC uses TIM4 encoder mode on PB6/PB7. I2C1 is not used on RZC (no OLED on RZC). |
| PA2/PA3 Nucleo ST-LINK VCP conflict? | PA2/PA3 used for ADC on RZC. SB63/SB65 solder bridges must be removed (same as FZC). Debug via SWO or LPUART1. |
| TIM1 for motor PWM vs TIM4 for encoder? | No conflict -- different timers on different pins. |

### 6.3 Unused Pin Handling

All unused GPIO pins configured as GPIO output LOW at initialization.

---

## 7. SC -- TMS570LC43x LaunchPad (LAUNCHXL2-TMS57012)

### 7.1 Pin Assignment Table

| # | Function | Peripheral | Pin Name | LaunchPad Connector | Direction | Voltage | External Component | Net Name | ASIL |
|---|----------|-----------|----------|--------------------|-----------|---------|--------------------|----------|------|
| 1 | CAN TX | DCAN1 | DCAN1TX | J5 (edge connector) | OUT | 3.3V | SN65HVD230 TXD | SC_CAN_TX | D |
| 2 | CAN RX | DCAN1 | DCAN1RX | J5 (edge connector) | IN | 3.3V | SN65HVD230 RXD | SC_CAN_RX | D |
| 3 | Kill relay control | GIO | GIO_A0 | J3-1 (HDR1) | OUT | 3.3V | IRLZ44N gate (100R + 10k PD) | SC_KILL_RELAY | D |
| 4 | CVC fault LED | GIO | GIO_A1 | J3-2 (HDR1) | OUT | 3.3V | Red LED + 330R | SC_LED_CVC | B |
| 5 | FZC fault LED | GIO | GIO_A2 | J3-3 (HDR1) | OUT | 3.3V | Red LED + 330R | SC_LED_FZC | B |
| 6 | RZC fault LED | GIO | GIO_A3 | J3-4 (HDR1) | OUT | 3.3V | Red LED + 330R | SC_LED_RZC | B |
| 7 | System fault LED | GIO | GIO_A4 | J3-5 (HDR1) | OUT | 3.3V | Amber LED + 330R | SC_LED_SYS | B |
| 8 | WDT Feed | GIO | GIO_A5 | J3-6 (HDR1) | OUT | 3.3V | TPS3823 WDI | SC_WDT_WDI | D |
| 9 | Heartbeat LED | GIO | GIO_B1 | J12 (onboard LED1) | OUT | 3.3V | Onboard LED (green) | SC_LED_HB | QM |

**Total pins used: 9** (DCAN1: 2 pins, GIO_A: 6 pins, GIO_B: 1 pin)

### 7.2 TMS570LC43x LaunchPad Connector Reference

| Connector | Pins Available | Used By |
|-----------|---------------|---------|
| J5 (edge connector) | DCAN1TX, DCAN1RX, SPI1, etc. | DCAN1 TX/RX |
| J3 (HDR1 header) | GIO_A0 through GIO_A7 | Kill relay, LEDs, WDT |
| J12 | GIO_B1 (onboard LED1) | Heartbeat blink |
| J1 (XDS110 USB) | Debug/power | USB power + JTAG |

### 7.3 Pin Conflict Check

| Check | Result |
|-------|--------|
| DCAN1 vs DCAN4? | Using DCAN1 (via edge connector J5). DCAN4 NOT used due to HALCoGen v04.07.01 mailbox bug. |
| GIO_A pins: any conflict with onboard peripherals? | J3 header provides direct access to GIO_A0-A7. No conflict with onboard components (onboard LEDs are on GIO_B). |
| GIO_B1 (LED1) used for heartbeat vs other function? | GIO_B1 is the onboard LED1. Used only for heartbeat blink (QM). No conflict. |

### 7.4 DCAN1 Configuration Notes

1. **DCAN1 module**: Enabled via HALCoGen configuration tool.
2. **Silent mode**: Set DCAN1 TEST register bit 3 = 1 for listen-only operation.
3. **Bit timing**: Configured for 500 kbps with 80% sample point (see HW Design section 6.3).
4. **Mailboxes**: Configure message objects 1-15 for reception of heartbeat (0x010-0x012), vehicle state (0x100), torque request (0x101), motor current (0x301).
5. **Edge connector wiring**: DCAN1TX and DCAN1RX available on edge connector J5. Wire to SN65HVD230 breakout board.

---

## 8. Alternate Function Summary

### 8.1 STM32G474RE Alternate Function Numbers Used

| AF | Peripheral | Pins |
|----|-----------|------|
| AF1 | TIM2 | PA0 (CH1), PA1 (CH2) -- FZC servos |
| AF2 | TIM4 | PB6 (CH1), PB7 (CH2) -- RZC encoder |
| AF4 | I2C1 | PB8 (SCL), PB9 (SDA) -- CVC OLED |
| AF5 | SPI1 | PA5 (SCK), PA6 (MISO), PA7 (MOSI) -- CVC pedal sensors |
| AF5 | SPI2 | PB13 (SCK), PB14 (MISO), PB15 (MOSI) -- FZC steering angle sensor |
| AF6 | TIM1 | PA8 (CH1), PA9 (CH2) -- RZC motor PWM |
| AF7 | USART2 | PA2 (TX), PA3 (RX) -- FZC lidar UART |
| AF9 | FDCAN1 | PA11 (RX), PA12 (TX) -- all STM32 ECUs |
| AN | ADC1 | PA0-PA4, PB15 -- RZC analog inputs |

### 8.2 Cross-ECU Pin Commonality

| Pin | CVC Function | FZC Function | RZC Function |
|-----|-------------|-------------|-------------|
| PA11 | FDCAN1_RX | FDCAN1_RX | FDCAN1_RX |
| PA12 | FDCAN1_TX | FDCAN1_TX | FDCAN1_TX |
| PA5 | SPI1_SCK | (unused) | (unused) |
| PA6 | SPI1_MISO | (unused) | (unused) |
| PA7 | SPI1_MOSI | (unused) | (unused) |
| PA4 | SPI1_CS (pedal 1) | (unused) | ADC1_IN5 (BTS7960 IS_L) |
| PB13 | (unused) | SPI2_SCK (steering) | (unused) |
| PB14 | (unused) | SPI2_MISO (steering) | (unused) |
| PB15 | (unused) | SPI2_MOSI (steering) | ADC1_IN15 (BTS7960 IS_R) |
| PB12 | (unused) | SPI2_CS (steering) | (unused) |
| PA0 | (unused) | TIM2_CH1 (steer servo) | ADC1_IN1 (current) |
| PA1 | (unused) | TIM2_CH2 (brake servo) | ADC1_IN2 (motor temp) |
| PA2 | (unused) | USART2_TX (lidar) | ADC1_IN3 (board temp) |
| PA3 | (unused) | USART2_RX (lidar) | ADC1_IN4 (battery V) |
| PB0 | WDT WDI | WDT WDI | Motor R_EN |
| PB4 | Status LED Green | Buzzer drive | WDT WDI |

**Note**: PB0 serves different functions on different ECUs. On CVC and FZC, it feeds the watchdog. On RZC, it controls the motor R_EN (watchdog moved to PB4).

---

## 9. Nucleo-64 Solder Bridge Modifications

The following solder bridge changes are required on the Nucleo-64 boards:

| ECU | Solder Bridge | Action | Reason |
|-----|--------------|--------|--------|
| FZC | SB63 | REMOVE | Free PA2 from ST-LINK VCP TX (needed for lidar UART TX) |
| FZC | SB65 | REMOVE | Free PA3 from ST-LINK VCP RX (needed for lidar UART RX) |
| RZC | SB63 | REMOVE | Free PA2 from ST-LINK VCP TX (needed for ADC1_IN3) |
| RZC | SB65 | REMOVE | Free PA3 from ST-LINK VCP RX (needed for ADC1_IN4) |
| CVC | SB21 | REMOVE (optional) | Disconnect PA5 from onboard LED LD2 (SPI1_SCK interference). Alternatively, accept LED blinking with SPI clock. |

**Note**: After removing SB63/SB65, printf debug output must use LPUART1 (PG7/PG8 on morpho) or SWO (PB3) instead of the default USART2 VCP.

---

## 10. Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 0.1 | 2026-02-21 | System | Initial preliminary pin mapping |
| 1.0 | 2026-02-21 | System | Complete pin mapping: CVC (13 pins), FZC (14 pins), RZC (17 pins), SC (9 pins). Added AF numbers, Nucleo header references, voltage levels, net names, ASIL, pin conflict checks, solder bridge modifications, unused pin handling |
