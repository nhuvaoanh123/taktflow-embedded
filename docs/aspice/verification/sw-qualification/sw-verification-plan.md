---
document_id: SVP
title: "SW Verification Plan"
version: "0.1"
status: planned
aspice_process: SWE.6
---

# SW Verification Plan

<!-- Phase 12 deliverable -->

## Purpose

Define the software qualification test approach per ASPICE SWE.6.

## Verification Methods

| Method | Tool | ASIL D Required |
|--------|------|----------------|
| Requirements-based testing | Unity/pytest | Yes |
| Interface testing | SIL + CAN | Yes |
| Fault injection testing | CAN injection | Yes |
| Static analysis | cppcheck + MISRA | Yes |
| Resource usage measurement | linker map | Yes |

## Qualification Criteria

- All safety requirements have passing tests
- No MISRA mandatory violations
- MC/DC coverage >= 100% for ASIL D modules
- WCET within budget for all safety functions
