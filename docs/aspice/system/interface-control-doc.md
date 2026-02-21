---
document_id: ICD
title: "Interface Control Document"
version: "0.1"
status: planned
aspice_process: SYS.3
---

# Interface Control Document

<!-- Phase 4 deliverable -->

## Purpose

Defines all inter-element interfaces per ASPICE SYS.3.

## Interface Summary

| Interface | Type | Elements | Protocol |
|-----------|------|----------|----------|
| CAN Bus | Communication | All ECUs | CAN 2.0B, 500 kbps |
| SOME/IP | Communication | Simulated ECUs | vsomeip |
| MQTT | Communication | Gateway ↔ Cloud | MQTT over TLS |
| SPI | Sensor | CVC ↔ AS5048A | SPI, 10 MHz |
| UART | Sensor | FZC ↔ TFMini-S | UART, 115200 bps |
| ADC | Sensor | RZC ↔ ACS723, NTC | ADC, DMA |
| PWM | Actuator | FZC/RZC ↔ Servos/Motor | Timer PWM |

## Detailed Interface Definitions

<!-- To be completed in Phase 4 -->
