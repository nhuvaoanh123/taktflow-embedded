---
document_id: UTP
title: "Unit Test Plan"
version: "0.1"
status: planned
aspice_process: SWE.4
---

## Human-in-the-Loop (HITL) Comment Lock

`HITL` means human-reviewer-owned comment content.

**Marker standard (code-friendly):**
- Markdown: `<!-- HITL-LOCK START:<id> -->` ... `<!-- HITL-LOCK END:<id> -->`
- C/C++/Java/JS/TS: `// HITL-LOCK START:<id>` ... `// HITL-LOCK END:<id>`
- Python/Shell/YAML/TOML: `# HITL-LOCK START:<id>` ... `# HITL-LOCK END:<id>`

**Rules:**
- AI must never edit, reformat, move, or delete text inside any `HITL-LOCK` block.
- Append-only: AI may add new comments/changes only; prior HITL comments stay unchanged.
- If a locked comment needs revision, add a new note outside the lock or ask the human reviewer to unlock it.


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

