---
document_id: SWARCH
title: "Software Architecture"
version: "0.1"
status: planned
aspice_process: SWE.2
---

# Software Architecture

<!-- Phase 3 deliverable -->

## Purpose

Software architectural design per ASPICE SWE.2. Static and dynamic views, component interfaces, resource estimates.

## Static Architecture

### AUTOSAR-like Layered Model

```
Application Layer (SWCs)
    ↕
RTE (Runtime Environment)
    ↕
BSW Services (Com, Dcm, Dem, WdgM, BswM)
    ↕
ECU Abstraction (CanIf, PduR, IoHwAb)
    ↕
MCAL (Can, Spi, Adc, Pwm, Dio, Gpt)
    ↕
Hardware
```

## Dynamic Architecture

### Task Scheduling

| Task | Period | Priority | Content |
|------|--------|----------|---------|
| — | — | — | — |

## Component Interfaces

<!-- To be completed in Phase 3 -->

## Resource Estimates

| ECU | Flash (est.) | RAM (est.) | CPU Load (est.) |
|-----|-------------|------------|-----------------|
| CVC | — | — | — |
| FZC | — | — | — |
| RZC | — | — | — |
