---
document_id: PIN-MAP
title: "Pin Mapping"
version: "0.1"
status: planned
---

# Pin Mapping

<!-- Phase 4 deliverable — see docs/plans/master-plan.md Phase 4 -->

## Purpose

Complete pin assignment for all physical ECUs. Cross-reference with board schematics.

## CVC — STM32G474RE Nucleo-64

| Function | Peripheral | Pin | Arduino Header | Notes |
|----------|-----------|-----|----------------|-------|
| CAN TX | FDCAN1 | PA12 | D2 | Via CAN transceiver |
| CAN RX | FDCAN1 | PA11 | D10 | Via CAN transceiver |
| Pedal SPI SCK | SPI1 | PA5 | D13 | AS5048A sensors |
| Pedal SPI MISO | SPI1 | PA6 | D12 | AS5048A sensors |
| Pedal CS1 | GPIO | PB6 | D5 | Pedal sensor 1 |
| Pedal CS2 | GPIO | PC7 | D9 | Pedal sensor 2 |
| OLED SDA | I2C1 | PB9 | D14 | SSD1306 |
| OLED SCL | I2C1 | PB8 | D15 | SSD1306 |
| E-stop | GPIO (EXTI) | PC13 | — | User button / external |
| Status LED | GPIO | PA5 | — | Onboard LED |
| WDT Feed | GPIO | PB0 | D3 | TPS3823 WDI |

## FZC — STM32G474RE Nucleo-64

| Function | Peripheral | Pin | Notes |
|----------|-----------|-----|-------|
| CAN TX | FDCAN1 | PA12 | — |
| CAN RX | FDCAN1 | PA11 | — |
| Steering servo PWM | TIM2_CH1 | PA0 | — |
| Brake servo PWM | TIM2_CH2 | PA1 | — |
| Lidar UART TX | USART1 | PA9 | TFMini-S |
| Lidar UART RX | USART1 | PA10 | TFMini-S (DMA) |
| Steering angle SPI | SPI2 | PB13-15 | AS5048A |
| Buzzer | GPIO | PB4 | — |
| WDT Feed | GPIO | PB0 | TPS3823 WDI |

## RZC — STM32G474RE Nucleo-64

| Function | Peripheral | Pin | Notes |
|----------|-----------|-----|-------|
| CAN TX | FDCAN1 | PA12 | — |
| CAN RX | FDCAN1 | PA11 | — |
| Motor PWM+ | TIM3_CH1 | PA6 | BTS7960 RPWM |
| Motor PWM- | TIM3_CH2 | PA7 | BTS7960 LPWM |
| Motor enable | GPIO | PB5 | BTS7960 R_EN + L_EN |
| Encoder A | TIM4_CH1 | PB6 | Quadrature |
| Encoder B | TIM4_CH2 | PB7 | Quadrature |
| Current sense | ADC1_IN1 | PA0 | ACS723 |
| Motor temp | ADC1_IN2 | PA1 | NTC 10K |
| Board temp | ADC1_IN3 | PA2 | NTC 10K |
| Battery voltage | ADC1_IN4 | PA3 | Voltage divider |
| WDT Feed | GPIO | PB0 | TPS3823 WDI |

## SC — TMS570LC43x LaunchPad

| Function | Peripheral | Pin | Notes |
|----------|-----------|-----|-------|
| CAN TX | DCAN1 | DCAN1TX (edge connector) | Via SN65HVD230 |
| CAN RX | DCAN1 | DCAN1RX (edge connector) | Silent mode via TEST reg |
| Kill relay | GIO A[0] | J3 pin | Via IRLZ44N MOSFET |
| Fault LED 1 (CVC) | GIO A[1] | J3 pin | Green/Red |
| Fault LED 2 (FZC) | GIO A[2] | J3 pin | Green/Red |
| Fault LED 3 (RZC) | GIO A[3] | J3 pin | Green/Red |
| Status LED | GIO B[1] | Onboard | Heartbeat blink |
| WDT Feed | GIO A[4] | J3 pin | TPS3823 WDI |

<!-- Pin assignments are preliminary — verify against board schematics in Phase 4 -->
