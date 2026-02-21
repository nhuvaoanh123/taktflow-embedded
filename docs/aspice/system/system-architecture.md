---
document_id: SYSARCH
title: "System Architecture"
version: "0.1"
status: planned
aspice_process: SYS.3
---

# System Architecture

<!-- Phase 3 deliverable -->

## Purpose

System-level architectural design per ASPICE SYS.3. Defines system elements, their interactions, and allocation of requirements to elements.

## Architecture Overview

7-ECU zonal architecture (4 physical + 3 simulated) + edge gateway + cloud.

## System Elements

| Element | Type | ASIL | Hardware |
|---------|------|------|----------|
| CVC | Physical ECU | D | STM32G474RE |
| FZC | Physical ECU | D | STM32G474RE |
| RZC | Physical ECU | D | STM32G474RE |
| SC | Physical ECU | D | TMS570LC43x |
| BCM | Simulated ECU | QM | Docker |
| ICU | Simulated ECU | QM | Docker |
| TCU | Simulated ECU | QM | Docker |
| Edge Gateway | Gateway | QM | Raspberry Pi 4 |
| Cloud | Service | QM | AWS IoT Core |

## Communication Architecture

See [can-message-matrix.md](can-message-matrix.md) and [interface-control-doc.md](interface-control-doc.md).

## Requirement Allocation

| System Requirement | Allocated To |
|-------------------|-------------|
| — | — |
