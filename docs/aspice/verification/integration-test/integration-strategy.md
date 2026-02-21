---
document_id: ITS
title: "Integration Test Strategy"
version: "0.1"
status: planned
aspice_process: SWE.5
---

# Integration Test Strategy

<!-- Phase 12 deliverable -->

## Purpose

Define the integration order, test approach, and pass/fail criteria per ASPICE SWE.5.

## Integration Order

1. MCAL → CanIf → PduR → Com (data path)
2. Com → Rte → SWC (signal path)
3. Dcm → PduR → CanIf (diagnostic path)
4. WdgM → BswM (supervision path)
5. ECU-to-ECU via CAN bus

## Integration Levels

| Level | Scope | Method |
|-------|-------|--------|
| Intra-ECU | BSW + SWC on single ECU | SIL |
| Inter-ECU | Multiple ECUs on CAN bus | SIL → PIL → HIL |
| System | All 7 ECUs + gateway | HIL |
