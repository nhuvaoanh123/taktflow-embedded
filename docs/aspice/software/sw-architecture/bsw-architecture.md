---
document_id: BSW-ARCH
title: "BSW Architecture"
version: "0.1"
status: planned
aspice_process: SWE.2
---

# BSW Architecture

<!-- Phase 5 deliverable -->

## Purpose

Detailed architecture of the AUTOSAR-like BSW layer shared across physical ECUs.

## Module Dependency Graph

```
SWC ← Rte ← Com ← PduR ← CanIf ← Can (MCAL)
              ↑      ↑
             Dcm    Dem
              ↑
           WdgM, BswM
```

## Module Specifications

### Can (MCAL)

- API: Can_Init(), Can_Write(), Can_MainFunction_Read()
- Platform variants: STM32 (FDCAN HAL), TMS570 (DCAN), POSIX (SocketCAN)

### CanIf

- API: CanIf_Transmit(), CanIf_RxIndication()
- Routes PDUs between Can driver and PduR

<!-- Continue for all modules -->

## Configuration Strategy

- Per-ECU config files: `firmware/{ecu}/cfg/`
- Compile-time selection via preprocessor defines
