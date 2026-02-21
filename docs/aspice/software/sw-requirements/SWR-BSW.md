---
document_id: SWR-BSW
title: "Software Requirements — Shared BSW"
version: "0.1"
status: planned
aspice_process: SWE.1
---

# Software Requirements — Shared BSW Layer

## Purpose

Requirements for the AUTOSAR-like BSW modules shared across physical ECUs.

## Modules

| Module | AUTOSAR Name | Requirements Prefix |
|--------|-------------|-------------------|
| Can | CAN Driver (MCAL) | SWR-BSW-CAN-xxx |
| CanIf | CAN Interface | SWR-BSW-CANIF-xxx |
| PduR | PDU Router | SWR-BSW-PDUR-xxx |
| Com | Communication | SWR-BSW-COM-xxx |
| Dcm | Diagnostic Comm Mgr | SWR-BSW-DCM-xxx |
| Dem | Diagnostic Event Mgr | SWR-BSW-DEM-xxx |
| WdgM | Watchdog Manager | SWR-BSW-WDGM-xxx |
| BswM | BSW Mode Manager | SWR-BSW-BSWM-xxx |
| Rte | Runtime Environment | SWR-BSW-RTE-xxx |

## Requirements

### SWR-BSW-CAN-001: {Title}

- **Traces up**: SYS-xxx
- **Traces down**: firmware/shared/bsw/mcal/Can.c
- **Verified by**: TC-BSW-CAN-xxx
- **Status**: planned

{Description}
