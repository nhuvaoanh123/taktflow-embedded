---
document_id: UTP
title: "Unit Test Plan"
version: "0.1"
status: planned
aspice_process: SWE.4
---

# Unit Test Plan

<!-- Phase 12 deliverable -->

## Purpose

Define unit test strategy, framework, and coverage targets per ASPICE SWE.4.

## Test Framework

| Target | Framework | Runner |
|--------|-----------|--------|
| STM32 BSW/SWC | Unity | Native (PC) + target |
| TMS570 SC | CCS Test | Target |
| Python gateway | pytest | CI |

## Coverage Targets

| ASIL | Statement | Branch | MC/DC |
|------|-----------|--------|-------|
| D | 100% | 100% | 100% |
| QM | 80% | 80% | N/A |

## Modules Under Test

| Module | Test File | Priority |
|--------|-----------|----------|
| Com | test_Com.c | High |
| Dcm | test_Dcm.c | High |
| Dem | test_Dem.c | High |
| WdgM | test_WdgM.c | High |
| E2E | test_E2E.c | High |
| Swc_Pedal | test_Swc_Pedal.c | Critical |
| Swc_Motor | test_Swc_Motor.c | Critical |

<!-- Complete list in Phase 12 -->
