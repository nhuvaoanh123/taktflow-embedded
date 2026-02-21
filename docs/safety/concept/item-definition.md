---
document_id: ITEM-DEF
title: "Item Definition"
version: "0.1"
status: planned
iso_26262_part: 3
iso_26262_clause: "5"
aspice_process: SYS.1
---

# Item Definition

<!-- Phase 1 deliverable — see docs/plans/master-plan.md Phase 1 -->

## Purpose

Defines the item under consideration, its boundaries, interfaces, functionality, and operating environment per ISO 26262-3 Clause 5.

## Sections (to be completed in Phase 1)

### 1. Item Description
- System name, purpose, intended use
- Operational modes

### 2. Functional Description
- Drive-by-wire (pedal → motor)
- Steering control
- Braking control
- Distance sensing (lidar)
- Safety monitoring (independent controller)

### 3. Interfaces
- CAN bus (500 kbps, 7 ECU nodes)
- Sensor interfaces (SPI, UART, ADC)
- Actuator interfaces (PWM, GPIO)
- Cloud (MQTT over TLS)

### 4. Operating Environment
- Indoor demo platform
- 12V power supply
- Controlled conditions

### 5. Legal and Regulatory Requirements
- ISO 26262 ASIL D target
- MISRA C:2012/2023

### 6. Known Limitations and Assumptions
- Demo platform, not production vehicle
- S/E/C ratings assume real vehicle context
