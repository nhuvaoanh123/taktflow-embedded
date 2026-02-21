---
document_id: HWDES
title: "Hardware Design"
version: "0.1"
status: planned
aspice_process: HWE.2
---

# Hardware Design

<!-- Phase 4 deliverable -->

## Purpose

Hardware design documentation per ASPICE HWE.2.

## Design Overview

| ECU | MCU | Key Peripherals |
|-----|-----|----------------|
| CVC | STM32G474RE | FDCAN1, SPI1, I2C1 |
| FZC | STM32G474RE | FDCAN1, TIM2, USART1, SPI2 |
| RZC | STM32G474RE | FDCAN1, TIM3, TIM4, ADC1 |
| SC | TMS570LC43x | DCAN1, GIO, RTI |

## Schematic References

See `hardware/schematics/`

## PCB / Wiring

See `hardware/pin-mapping.md`
