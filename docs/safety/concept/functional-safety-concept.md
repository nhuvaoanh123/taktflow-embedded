---
document_id: FSC
title: "Functional Safety Concept"
version: "0.1"
status: planned
iso_26262_part: 3
iso_26262_clause: "8"
aspice_process: SYS.1
---

# Functional Safety Concept

<!-- Phase 1 deliverable — see docs/plans/master-plan.md Phase 1 -->

## Purpose

Derives functional safety requirements (FSR) from safety goals. Defines safety mechanisms, warning concepts, and degradation strategies.

## Functional Safety Requirements

| FSR-ID | Description | ASIL | Traces to SG | Allocation |
|--------|-------------|------|-------------|------------|
| FSR-001 | — | — | SG-001 | — |

## Safety Mechanisms

| Mechanism | Purpose | Allocated to |
|-----------|---------|-------------|
| Dual sensor plausibility | Detect pedal sensor failure | CVC |
| Heartbeat monitoring | Detect ECU hang | SC |
| Kill relay | Force safe state | SC |
| E2E protection | Detect CAN corruption | All physical ECUs |
| External watchdog | Detect SW hang | All ECUs |

## Warning and Degradation Concept

### Warning Strategy
- OLED display (CVC)
- Buzzer (FZC)
- Fault LEDs (SC)

### Degradation Levels
1. NORMAL → full operation
2. DEGRADED → reduced performance
3. LIMP → minimal function
4. SAFE_STOP → controlled shutdown
