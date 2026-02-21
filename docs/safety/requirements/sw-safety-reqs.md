---
document_id: SSR
title: "Software Safety Requirements"
version: "0.1"
status: planned
iso_26262_part: 6
aspice_process: SWE.1
---

# Software Safety Requirements

<!-- Phase 3 deliverable — per-ECU SW safety requirements -->

## Purpose

Software safety requirements derived from TSRs, allocated to software components.

## Naming Convention

`SSR-{ECU}-NNN` (e.g., SSR-CVC-001)

## Requirements by ECU

### CVC — Central Vehicle Computer

### SSR-CVC-001: {Title}

- **ASIL**: D
- **Traces up**: TSR-001, FSR-001, SG-001
- **Traces down**: firmware/cvc/src/{file}
- **Verified by**: TC-CVC-xxx
- **Status**: planned

{Description}

### FZC — Front Zone Controller

### RZC — Rear Zone Controller

### SC — Safety Controller

<!-- Add requirements as they are derived in Phase 3 -->
