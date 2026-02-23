---
document_id: TRACE
title: "Traceability Matrix"
version: "1.0"
status: draft
aspice_processes: "SYS.1-5, SWE.1-6"
iso_26262_part: "3, 4, 5, 6"
date: 2026-02-21
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


# Traceability Matrix

## 1. Purpose

This document provides the complete bidirectional traceability matrix for the Taktflow Zonal Vehicle Platform, linking every requirement from the top-level safety goals through functional safety requirements, technical safety requirements, software/hardware safety requirements, software requirements, source code modules, and test cases. This matrix satisfies the traceability requirements of Automotive SPICE 4.0 (SWE.1-6, SYS.1-5) and ISO 26262:2018 (Parts 3, 4, 5, 6, 8).

The matrix enables:
- **Impact analysis**: When any requirement changes, all affected artifacts are immediately identifiable.
- **Coverage verification**: Every requirement is implemented and tested; no orphan code or tests exist.
- **Audit readiness**: Assessors can verify completeness at any traceability level in a single document.
- **Gap detection**: Missing links are flagged in the gap analysis section.

## 2. Traceability Chain

```
Stakeholder Requirements (STK-001 to STK-032)          — SYS.1
    | bidirectional
    v
System Requirements (SYS-001 to SYS-056)               — SYS.2
    | bidirectional
    v
 +-------------------+-------------------+
 |                                       |
 v                                       v
Safety Goals (SG-001 to SG-008)     Software Requirements (SWR-{ECU}-NNN)  — SWE.1
    |                                    |
    v                                    v
Functional Safety Reqs               SW Architecture Components             — SWE.2
(FSR-001 to FSR-025)                     |
    |                                    v
    v                                 SW Detailed Design / Source Code       — SWE.3
Technical Safety Reqs                    |
(TSR-001 to TSR-051)                     v
    |                                 Unit Tests (SWE.4)
    v                                    |
 +----------+----------+                 v
 |                      |             Integration Tests (SWE.5)
 v                      v                |
Software Safety     Hardware Safety      v
Requirements        Requirements      SW Verification Tests (SWE.6)
(SSR-{ECU}-NNN)    (HSR-{ECU}-NNN)       |
 81 total           25 total             v
                                      System Integration Tests (SYS.4)
                                         |
                                         v
                                      System Verification Tests (SYS.5)
```

## 3. Referenced Documents

| Document ID | Title | Version | Location |
|-------------|-------|---------|----------|
| STK | Stakeholder Requirements | 1.0 | docs/aspice/system/stakeholder-requirements.md |
| SYSREQ | System Requirements Specification | 1.0 | docs/aspice/system/system-requirements.md |
| SG | Safety Goals | 1.0 | docs/safety/concept/safety-goals.md |
| FSR | Functional Safety Requirements | 1.0 | docs/safety/requirements/functional-safety-reqs.md |
| TSR | Technical Safety Requirements | 1.0 | docs/safety/requirements/technical-safety-reqs.md |
| SSR | Software Safety Requirements | 1.0 | docs/safety/requirements/sw-safety-reqs.md |
| HSR | Hardware Safety Requirements | 1.0 | docs/safety/requirements/hw-safety-reqs.md |
| SWR-CVC | SW Requirements — CVC | 1.0 | docs/aspice/software/sw-requirements/SWR-CVC.md |
| SWR-FZC | SW Requirements — FZC | 1.0 | docs/aspice/software/sw-requirements/SWR-FZC.md |
| SWR-RZC | SW Requirements — RZC | 1.0 | docs/aspice/software/sw-requirements/SWR-RZC.md |
| SWR-SC | SW Requirements — SC | 0.1 | docs/aspice/software/sw-requirements/SWR-SC.md |
| SWR-BCM | SW Requirements — BCM | 1.0 | docs/aspice/software/sw-requirements/SWR-BCM.md |
| SWR-ICU | SW Requirements — ICU | 1.0 | docs/aspice/software/sw-requirements/SWR-ICU.md |
| SWR-TCU | SW Requirements — TCU | 1.0 | docs/aspice/software/sw-requirements/SWR-TCU.md |
| SWR-BSW | SW Requirements — BSW | 1.0 | docs/aspice/software/sw-requirements/SWR-BSW.md |

---

## 4. Safety Goal to FSR Matrix

This section traces each safety goal to its derived functional safety requirements. Every SG must trace to at least one FSR. Every FSR must trace to at least one SG.

| SG | ASIL | Derived FSRs |
|----|------|-------------|
| SG-001 (Unintended acceleration) | D | FSR-001, FSR-002, FSR-003, FSR-013, FSR-016, FSR-018, FSR-019, FSR-021, FSR-022, FSR-024 |
| SG-002 (Loss of drive torque) | B | FSR-013, FSR-019, FSR-023, FSR-024 |
| SG-003 (Unintended steering) | D | FSR-006, FSR-007, FSR-008, FSR-013, FSR-016, FSR-019, FSR-024 |
| SG-004 (Loss of braking) | D | FSR-009, FSR-010, FSR-013, FSR-016, FSR-019, FSR-024, FSR-025 |
| SG-005 (Unintended braking) | A | FSR-009, FSR-013, FSR-019, FSR-024 |
| SG-006 (Motor protection) | A | FSR-004, FSR-005, FSR-013, FSR-019, FSR-024 |
| SG-007 (Obstacle detection) | C | FSR-011, FSR-012, FSR-013, FSR-019, FSR-024 |
| SG-008 (Independent monitoring) | C | FSR-013, FSR-014, FSR-015, FSR-016, FSR-017, FSR-018, FSR-019, FSR-020, FSR-024 |

### 4.1 Reverse — FSR to SG

| FSR | ASIL | Parent SGs |
|-----|------|-----------|
| FSR-001 | D | SG-001 |
| FSR-002 | D | SG-001 |
| FSR-003 | D | SG-001 |
| FSR-004 | A | SG-006 |
| FSR-005 | A | SG-006 |
| FSR-006 | D | SG-003 |
| FSR-007 | D | SG-003 |
| FSR-008 | C | SG-003 |
| FSR-009 | D | SG-004, SG-005 |
| FSR-010 | D | SG-004 |
| FSR-011 | C | SG-007 |
| FSR-012 | C | SG-007 |
| FSR-013 | D | SG-001, SG-002, SG-003, SG-004, SG-005, SG-006, SG-007, SG-008 |
| FSR-014 | C | SG-008 |
| FSR-015 | C | SG-008 |
| FSR-016 | D | SG-001, SG-003, SG-004, SG-008 |
| FSR-017 | D | SG-008 |
| FSR-018 | B | SG-001, SG-008 |
| FSR-019 | D | SG-001, SG-002, SG-003, SG-004, SG-005, SG-006, SG-007, SG-008 |
| FSR-020 | C | SG-008 |
| FSR-021 | C | SG-001 |
| FSR-022 | C | SG-001 |
| FSR-023 | B | SG-001, SG-002, SG-003, SG-004, SG-005, SG-006, SG-007, SG-008 |
| FSR-024 | D | SG-001, SG-002, SG-003, SG-004, SG-005, SG-006, SG-007, SG-008 |
| FSR-025 | D | SG-004 |

---

## 5. FSR to TSR Matrix

Each functional safety requirement is refined into one or more technical safety requirements.

| FSR | ASIL | Derived TSRs |
|-----|------|-------------|
| FSR-001 | D | TSR-001, TSR-002 |
| FSR-002 | D | TSR-002, TSR-003 |
| FSR-003 | D | TSR-004, TSR-005 |
| FSR-004 | A | TSR-006, TSR-007 |
| FSR-005 | A | TSR-008, TSR-009 |
| FSR-006 | D | TSR-010, TSR-011 |
| FSR-007 | D | TSR-012, TSR-013 |
| FSR-008 | C | TSR-014 |
| FSR-009 | D | TSR-015, TSR-016 |
| FSR-010 | D | TSR-017 |
| FSR-011 | C | TSR-018, TSR-019 |
| FSR-012 | C | TSR-020, TSR-021 |
| FSR-013 | D | TSR-022, TSR-023, TSR-024 |
| FSR-014 | C | TSR-025, TSR-026 |
| FSR-015 | C | TSR-027, TSR-028 |
| FSR-016 | D | TSR-029, TSR-030 |
| FSR-017 | D | TSR-031, TSR-032 |
| FSR-018 | B | TSR-033, TSR-034 |
| FSR-019 | D | TSR-035, TSR-036, TSR-037 |
| FSR-020 | C | TSR-038, TSR-039 |
| FSR-021 | C | TSR-040 |
| FSR-022 | C | TSR-041, TSR-042 |
| FSR-023 | B | TSR-043, TSR-044, TSR-045 |
| FSR-024 | D | TSR-046, TSR-047 |
| FSR-025 | D | TSR-048, TSR-049 |

### 5.1 Reverse — TSR to FSR

| TSR | ASIL | Parent FSRs |
|-----|------|------------|
| TSR-001 | D | FSR-001 |
| TSR-002 | D | FSR-001, FSR-002 |
| TSR-003 | D | FSR-002 |
| TSR-004 | D | FSR-003 |
| TSR-005 | D | FSR-003 |
| TSR-006 | A | FSR-004 |
| TSR-007 | C | FSR-004, FSR-022 |
| TSR-008 | A | FSR-005 |
| TSR-009 | A | FSR-005 |
| TSR-010 | D | FSR-006 |
| TSR-011 | D | FSR-006 |
| TSR-012 | D | FSR-007 |
| TSR-013 | D | FSR-007 |
| TSR-014 | C | FSR-008 |
| TSR-015 | D | FSR-009 |
| TSR-016 | D | FSR-009, FSR-025 |
| TSR-017 | D | FSR-010 |
| TSR-018 | C | FSR-011, FSR-012 |
| TSR-019 | C | FSR-011 |
| TSR-020 | C | FSR-012 |
| TSR-021 | C | FSR-012 |
| TSR-022 | D | FSR-013 |
| TSR-023 | D | FSR-013 |
| TSR-024 | D | FSR-013 |
| TSR-025 | C | FSR-014 |
| TSR-026 | C | FSR-014, FSR-017 |
| TSR-027 | C | FSR-015 |
| TSR-028 | D | FSR-015, FSR-016 |
| TSR-029 | D | FSR-016 |
| TSR-030 | D | FSR-016 |
| TSR-031 | D | FSR-017 |
| TSR-032 | D | FSR-017 |
| TSR-033 | B | FSR-018 |
| TSR-034 | B | FSR-018 |
| TSR-035 | D | FSR-019 |
| TSR-036 | D | FSR-019 |
| TSR-037 | D | FSR-019 |
| TSR-038 | C | FSR-020 |
| TSR-039 | C | FSR-020 |
| TSR-040 | C | FSR-021 |
| TSR-041 | C | FSR-022 |
| TSR-042 | C | FSR-022 |
| TSR-043 | B | FSR-023 |
| TSR-044 | B | FSR-023 |
| TSR-045 | B | FSR-023 |
| TSR-046 | D | FSR-024 |
| TSR-047 | D | FSR-024 |
| TSR-048 | D | FSR-025 |
| TSR-049 | D | FSR-025 |
| TSR-050 | C | FSR-016, FSR-017 |
| TSR-051 | C | FSR-017 |

---

## 6. TSR to SSR Matrix

Each TSR is refined into software safety requirements allocated to specific ECUs.

| TSR | ASIL | Derived SSRs |
|-----|------|-------------|
| TSR-001 | D | SSR-CVC-001, SSR-CVC-002 |
| TSR-002 | D | SSR-CVC-003, SSR-CVC-004 |
| TSR-003 | D | SSR-CVC-005 |
| TSR-004 | D | SSR-CVC-006, SSR-CVC-007 |
| TSR-005 | D | SSR-RZC-001, SSR-RZC-002 |
| TSR-006 | A | SSR-RZC-003, SSR-RZC-004 |
| TSR-007 | C | SSR-RZC-005 |
| TSR-008 | A | SSR-RZC-006 |
| TSR-009 | A | SSR-RZC-007 |
| TSR-010 | D | SSR-FZC-001 |
| TSR-011 | D | SSR-FZC-002, SSR-FZC-003 |
| TSR-012 | D | SSR-FZC-004, SSR-FZC-005 |
| TSR-013 | D | SSR-FZC-006 |
| TSR-014 | C | SSR-FZC-007 |
| TSR-015 | D | SSR-FZC-008 |
| TSR-016 | D | SSR-FZC-009 |
| TSR-017 | D | SSR-FZC-010 |
| TSR-018 | C | SSR-FZC-011 |
| TSR-019 | C | SSR-FZC-012 |
| TSR-020 | C | SSR-FZC-013 |
| TSR-021 | C | SSR-FZC-014 |
| TSR-022 | D | SSR-CVC-008, SSR-FZC-015, SSR-RZC-008, SSR-SC-001 |
| TSR-023 | D | SSR-CVC-009, SSR-FZC-016, SSR-RZC-009 |
| TSR-024 | D | SSR-CVC-010, SSR-FZC-017, SSR-RZC-010, SSR-SC-002 |
| TSR-025 | C | SSR-CVC-011, SSR-FZC-018, SSR-RZC-011 |
| TSR-026 | C | SSR-CVC-012, SSR-FZC-019, SSR-RZC-012 |
| TSR-027 | C | SSR-SC-003, SSR-SC-004 |
| TSR-028 | D | SSR-SC-005 |
| TSR-029 | D | SSR-SC-006 |
| TSR-030 | D | SSR-SC-007 |
| TSR-031 | D | SSR-CVC-013, SSR-FZC-020, SSR-RZC-013, SSR-SC-008 |
| TSR-033 | B | SSR-CVC-014 |
| TSR-034 | B | SSR-CVC-015 |
| TSR-035 | D | SSR-CVC-016, SSR-CVC-017 |
| TSR-036 | D | SSR-CVC-018 |
| TSR-037 | D | SSR-CVC-019 |
| TSR-038 | C | SSR-CVC-020, SSR-FZC-021, SSR-RZC-014, SSR-SC-009 |
| TSR-039 | C | SSR-CVC-021 |
| TSR-040 | C | SSR-RZC-015, SSR-RZC-016 |
| TSR-041 | C | SSR-SC-010, SSR-SC-011 |
| TSR-042 | C | SSR-SC-012 |
| TSR-043 | B | SSR-CVC-022 |
| TSR-044 | B | SSR-FZC-022 |
| TSR-045 | B | SSR-SC-013 |
| TSR-046 | D | SSR-CVC-023, SSR-FZC-023, SSR-RZC-017, SSR-SC-014 |
| TSR-047 | D | SSR-CVC-023, SSR-FZC-023, SSR-RZC-017 |
| TSR-048 | D | SSR-FZC-024 |
| TSR-049 | D | SSR-SC-015 |
| TSR-050 | C | SSR-SC-016 |
| TSR-051 | C | SSR-SC-017 |

**Note**: TSR-032 traces only to HSR (hardware safety requirements) — see Section 7.

---

## 7. TSR to HSR Matrix

Each TSR with hardware implications is refined into hardware safety requirements allocated to specific ECUs.

| TSR | ASIL | Derived HSRs |
|-----|------|-------------|
| TSR-001 | D | HSR-CVC-001 |
| TSR-002 | D | HSR-CVC-001 |
| TSR-005 | D | HSR-RZC-004 |
| TSR-006 | A | HSR-RZC-001 |
| TSR-008 | A | HSR-RZC-002 |
| TSR-010 | D | HSR-FZC-001 |
| TSR-012 | D | HSR-FZC-006 |
| TSR-013 | D | HSR-FZC-006 |
| TSR-015 | D | HSR-FZC-002 |
| TSR-018 | C | HSR-FZC-003 |
| TSR-022 | D | HSR-CVC-004, HSR-FZC-005, HSR-RZC-005, HSR-SC-004 |
| TSR-029 | D | HSR-SC-001, HSR-SC-006 |
| TSR-030 | D | HSR-SC-006 |
| TSR-032 | D | HSR-CVC-002, HSR-FZC-004, HSR-RZC-003, HSR-SC-002 |
| TSR-033 | B | HSR-CVC-003 |
| TSR-038 | C | HSR-CVC-004, HSR-FZC-005, HSR-RZC-005, HSR-SC-004 |
| TSR-040 | C | HSR-RZC-004, HSR-RZC-006 |
| TSR-043 | B | HSR-CVC-005 |
| TSR-044 | B | HSR-FZC-007 |
| TSR-045 | B | HSR-SC-003 |
| TSR-050 | C | HSR-SC-005 |
| TSR-051 | C | HSR-SC-005 |

---

## 8. SYS to SWR Matrix

System requirements are refined into software requirements per ECU.

### 8.1 Physical ECUs

#### CVC — Central Vehicle Computer (35 SWR)

| SYS | SWR-CVC |
|-----|---------|
| SYS-001 | SWR-CVC-001, SWR-CVC-002 |
| SYS-002 | SWR-CVC-003, SWR-CVC-004, SWR-CVC-005, SWR-CVC-006 |
| SYS-003 | SWR-CVC-007 |
| SYS-004 | SWR-CVC-008, SWR-CVC-016, SWR-CVC-017 |
| SYS-021 | SWR-CVC-020, SWR-CVC-021 |
| SYS-022 | SWR-CVC-022 |
| SYS-027 | SWR-CVC-023, SWR-CVC-029 |
| SYS-028 | SWR-CVC-018, SWR-CVC-019 |
| SYS-029 | SWR-CVC-009, SWR-CVC-010, SWR-CVC-011, SWR-CVC-012, SWR-CVC-013, SWR-CVC-029 |
| SYS-030 | SWR-CVC-010 |
| SYS-031 | SWR-CVC-016, SWR-CVC-017 |
| SYS-032 | SWR-CVC-014, SWR-CVC-015 |
| SYS-033 | SWR-CVC-016, SWR-CVC-017 |
| SYS-034 | SWR-CVC-022, SWR-CVC-024, SWR-CVC-025 |
| SYS-037 | SWR-CVC-033 |
| SYS-038 | SWR-CVC-033, SWR-CVC-034 |
| SYS-039 | SWR-CVC-031, SWR-CVC-033, SWR-CVC-035 |
| SYS-040 | SWR-CVC-033 |
| SYS-041 | SWR-CVC-030, SWR-CVC-034 |
| SYS-044 | SWR-CVC-026, SWR-CVC-027, SWR-CVC-028 |
| SYS-047 | SWR-CVC-001 |
| SYS-053 | SWR-CVC-032 |

#### FZC — Front Zone Controller (32 SWR)

| SYS | SWR-FZC |
|-----|---------|
| SYS-010 | SWR-FZC-008, SWR-FZC-026, SWR-FZC-028, SWR-FZC-032 |
| SYS-011 | SWR-FZC-001, SWR-FZC-002, SWR-FZC-003 |
| SYS-012 | SWR-FZC-004, SWR-FZC-005, SWR-FZC-006 |
| SYS-013 | SWR-FZC-007 |
| SYS-014 | SWR-FZC-009, SWR-FZC-026, SWR-FZC-032 |
| SYS-015 | SWR-FZC-010, SWR-FZC-012, SWR-FZC-027 |
| SYS-016 | SWR-FZC-011 |
| SYS-018 | SWR-FZC-013 |
| SYS-019 | SWR-FZC-014 |
| SYS-020 | SWR-FZC-015, SWR-FZC-016 |
| SYS-021 | SWR-FZC-021, SWR-FZC-022 |
| SYS-027 | SWR-FZC-023, SWR-FZC-025 |
| SYS-029 | SWR-FZC-025 |
| SYS-031 | SWR-FZC-026, SWR-FZC-027 |
| SYS-032 | SWR-FZC-019, SWR-FZC-020 |
| SYS-034 | SWR-FZC-024, SWR-FZC-028 |
| SYS-037 | SWR-FZC-030 |
| SYS-038 | SWR-FZC-030 |
| SYS-039 | SWR-FZC-030 |
| SYS-041 | SWR-FZC-031 |
| SYS-045 | SWR-FZC-017, SWR-FZC-018 |
| SYS-047 | SWR-FZC-001 |
| SYS-048 | SWR-FZC-013 |
| SYS-050 | SWR-FZC-008, SWR-FZC-009 |
| SYS-053 | SWR-FZC-029 |

#### RZC — Rear Zone Controller (30 SWR)

| SYS | SWR-RZC |
|-----|---------|
| SYS-004 | SWR-RZC-001, SWR-RZC-002, SWR-RZC-003, SWR-RZC-016 |
| SYS-005 | SWR-RZC-005, SWR-RZC-006, SWR-RZC-007, SWR-RZC-008 |
| SYS-006 | SWR-RZC-009, SWR-RZC-010, SWR-RZC-011 |
| SYS-007 | SWR-RZC-012, SWR-RZC-013, SWR-RZC-014, SWR-RZC-015 |
| SYS-008 | SWR-RZC-017, SWR-RZC-018 |
| SYS-009 | SWR-RZC-012 |
| SYS-021 | SWR-RZC-021, SWR-RZC-022 |
| SYS-027 | SWR-RZC-023, SWR-RZC-025 |
| SYS-029 | SWR-RZC-025 |
| SYS-031 | SWR-RZC-026, SWR-RZC-027 |
| SYS-032 | SWR-RZC-019, SWR-RZC-020 |
| SYS-034 | SWR-RZC-016, SWR-RZC-024 |
| SYS-037 | SWR-RZC-029 |
| SYS-038 | SWR-RZC-029 |
| SYS-039 | SWR-RZC-029 |
| SYS-041 | SWR-RZC-030 |
| SYS-049 | SWR-RZC-005, SWR-RZC-008, SWR-RZC-009 |
| SYS-050 | SWR-RZC-003, SWR-RZC-004 |
| SYS-053 | SWR-RZC-028 |

#### SC — Safety Controller (stub — SWR-SC pending)

The SC SWR document is at version 0.1 (stub). SC software requirements are currently captured in the SSR-SC requirements (SSR-SC-001 to SSR-SC-017). The SWR-SC document will be completed in a subsequent phase.

| SYS | SSR-SC (interim traceability) |
|-----|------|
| SYS-021 | SSR-SC-003, SSR-SC-004 (heartbeat monitoring) |
| SYS-023 | SSR-SC-006, SSR-SC-007 (kill relay control) |
| SYS-024 | SSR-SC-010, SSR-SC-011, SSR-SC-012 (cross-plausibility) |
| SYS-025 | SSR-SC-016, SSR-SC-017 (self-test) |
| SYS-026 | SSR-SC-013 (fault LEDs) |
| SYS-027 | SSR-SC-008 (watchdog feed) |
| SYS-032 | SSR-SC-001, SSR-SC-002 (E2E protection) |
| SYS-034 | SSR-SC-009 (CAN bus loss detection) |

### 8.2 Simulated ECUs (Docker)

#### BCM — Body Control Module (12 SWR)

| SYS | SWR-BCM |
|-----|---------|
| SYS-029 | SWR-BCM-002 |
| SYS-030 | SWR-BCM-002 |
| SYS-031 | SWR-BCM-001 |
| SYS-035 | SWR-BCM-001, SWR-BCM-002, SWR-BCM-003, SWR-BCM-004, SWR-BCM-005, SWR-BCM-009, SWR-BCM-010, SWR-BCM-011, SWR-BCM-012 |
| SYS-036 | SWR-BCM-006, SWR-BCM-007, SWR-BCM-008, SWR-BCM-010, SWR-BCM-011 |

#### ICU — Instrument Cluster Unit (10 SWR)

| SYS | SWR-ICU |
|-----|---------|
| SYS-006 | SWR-ICU-004 |
| SYS-008 | SWR-ICU-005 |
| SYS-009 | SWR-ICU-002 |
| SYS-021 | SWR-ICU-009 |
| SYS-029 | SWR-ICU-007 |
| SYS-031 | SWR-ICU-001 |
| SYS-038 | SWR-ICU-008 |
| SYS-041 | SWR-ICU-008 |
| SYS-044 | SWR-ICU-002, SWR-ICU-003, SWR-ICU-004, SWR-ICU-005, SWR-ICU-006, SWR-ICU-007, SWR-ICU-008, SWR-ICU-009, SWR-ICU-010 |
| SYS-046 | SWR-ICU-006, SWR-ICU-009 |

#### TCU — Telematics Control Unit (15 SWR)

| SYS | SWR-TCU |
|-----|---------|
| SYS-031 | SWR-TCU-001 |
| SYS-037 | SWR-TCU-001, SWR-TCU-002, SWR-TCU-011, SWR-TCU-012, SWR-TCU-013, SWR-TCU-014, SWR-TCU-015 |
| SYS-038 | SWR-TCU-003, SWR-TCU-004, SWR-TCU-010, SWR-TCU-011 |
| SYS-039 | SWR-TCU-005, SWR-TCU-006, SWR-TCU-014 |
| SYS-040 | SWR-TCU-007 |
| SYS-041 | SWR-TCU-008, SWR-TCU-009 |

### 8.3 BSW — Base Software (27 SWR)

| SYS | SWR-BSW |
|-----|---------|
| SYS-027 | SWR-BSW-021 |
| SYS-029 | SWR-BSW-022, SWR-BSW-026 |
| SYS-030 | SWR-BSW-022, SWR-BSW-026 |
| SYS-031 | SWR-BSW-001, SWR-BSW-002, SWR-BSW-003, SWR-BSW-011, SWR-BSW-013 |
| SYS-032 | SWR-BSW-002, SWR-BSW-003, SWR-BSW-013, SWR-BSW-015, SWR-BSW-016, SWR-BSW-023, SWR-BSW-024, SWR-BSW-025 |
| SYS-034 | SWR-BSW-004, SWR-BSW-005, SWR-BSW-012 |
| SYS-037 | SWR-BSW-013, SWR-BSW-017 |
| SYS-038 | SWR-BSW-017 |
| SYS-039 | SWR-BSW-017 |
| SYS-040 | SWR-BSW-017 |
| SYS-041 | SWR-BSW-018, SWR-BSW-019, SWR-BSW-020 |
| SYS-047 | SWR-BSW-006, SWR-BSW-014 |
| SYS-049 | SWR-BSW-007, SWR-BSW-014 |
| SYS-050 | SWR-BSW-008, SWR-BSW-009, SWR-BSW-014 |
| SYS-053 | SWR-BSW-010, SWR-BSW-027 |

---

## 9. End-to-End Traceability — Safety Chain

This section provides the complete traceability chain from each safety goal through to source code modules and test cases. Source code modules and test cases are placeholders (status: planned) as implementation has not yet begun.

### 9.1 SG-001: Prevent Unintended Acceleration (ASIL D, FTTI 50 ms)

| SG | FSR | TSR | SSR | SWR | Source Module | Test Cases |
|----|-----|-----|-----|-----|--------------|------------|
| SG-001 | FSR-001 | TSR-001 | SSR-CVC-001, SSR-CVC-002 | SWR-CVC-001, SWR-CVC-002 | firmware/cvc/src/mcal/spi_cfg.c, firmware/cvc/src/swc/swc_pedal.c | TC-CVC-001 to TC-CVC-003 |
| SG-001 | FSR-001, FSR-002 | TSR-002 | SSR-CVC-003, SSR-CVC-004 | SWR-CVC-003, SWR-CVC-004 | firmware/cvc/src/swc/swc_pedal.c | TC-CVC-004 to TC-CVC-008 |
| SG-001 | FSR-002 | TSR-003 | SSR-CVC-005 | SWR-CVC-005 | firmware/cvc/src/swc/swc_pedal.c | TC-CVC-009 |
| SG-001 | FSR-003 | TSR-004 | SSR-CVC-006, SSR-CVC-007 | SWR-CVC-006, SWR-CVC-007, SWR-CVC-008 | firmware/cvc/src/swc/swc_pedal.c, firmware/cvc/src/swc/swc_torque.c | TC-CVC-010 to TC-CVC-014 |
| SG-001 | FSR-003 | TSR-005 | SSR-RZC-001, SSR-RZC-002 | SWR-RZC-001, SWR-RZC-002 | firmware/rzc/src/swc/swc_motor.c | TC-RZC-001 to TC-RZC-004 |
| SG-001 | FSR-013 | TSR-022 | SSR-CVC-008, SSR-FZC-015, SSR-RZC-008, SSR-SC-001 | SWR-BSW-023, SWR-BSW-024, SWR-BSW-025 | firmware/shared/bsw/services/e2e/ | TC-BSW-044 to TC-BSW-052 |
| SG-001 | FSR-016 | TSR-029, TSR-030 | SSR-SC-006, SSR-SC-007 | (SWR-SC pending) | firmware/sc/src/main.c | TC-SC-006, TC-SC-007 |
| SG-001 | FSR-019 | TSR-035, TSR-036 | SSR-CVC-016 to SSR-CVC-018 | SWR-CVC-009 to SWR-CVC-011, SWR-BSW-022, SWR-BSW-026 | firmware/cvc/src/swc/swc_state.c, firmware/shared/bsw/services/bswm/ | TC-CVC-017 to TC-CVC-021 |
| SG-001 | FSR-021 | TSR-040 | SSR-RZC-015, SSR-RZC-016 | SWR-RZC-014, SWR-RZC-015 | firmware/rzc/src/swc/swc_motor.c | TC-RZC-019 to TC-RZC-021 |
| SG-001 | FSR-022 | TSR-041, TSR-042 | SSR-SC-010 to SSR-SC-012 | (SWR-SC pending) | firmware/sc/src/main.c | TC-SC-010 to TC-SC-012 |

### 9.2 SG-002: Prevent Loss of Drive Torque (ASIL B, FTTI 200 ms)

| SG | FSR | TSR | SSR | SWR | Source Module | Test Cases |
|----|-----|-----|-----|-----|--------------|------------|
| SG-002 | FSR-013 | TSR-022 to TSR-024 | SSR-CVC-008 to SSR-CVC-010, SSR-FZC-015 to SSR-FZC-017, SSR-RZC-008 to SSR-RZC-010, SSR-SC-001, SSR-SC-002 | SWR-BSW-015, SWR-BSW-016, SWR-BSW-023 to SWR-BSW-025 | firmware/shared/bsw/services/e2e/, firmware/shared/bsw/services/com/ | TC-BSW-028 to TC-BSW-052 |
| SG-002 | FSR-019 | TSR-035 to TSR-037 | SSR-CVC-016 to SSR-CVC-019 | SWR-CVC-009 to SWR-CVC-013, SWR-BSW-022 | firmware/cvc/src/swc/swc_state.c | TC-CVC-017 to TC-CVC-025 |
| SG-002 | FSR-023 | TSR-043, TSR-044, TSR-045 | SSR-CVC-022, SSR-FZC-022, SSR-SC-013 | SWR-CVC-027, SWR-FZC-017 | firmware/cvc/src/swc/swc_oled.c, firmware/fzc/src/swc/swc_buzzer.c | TC-CVC-053, TC-FZC-033 |

### 9.3 SG-003: Prevent Unintended Steering (ASIL D, FTTI 100 ms)

| SG | FSR | TSR | SSR | SWR | Source Module | Test Cases |
|----|-----|-----|-----|-----|--------------|------------|
| SG-003 | FSR-006 | TSR-010, TSR-011 | SSR-FZC-001 to SSR-FZC-003 | SWR-FZC-001 to SWR-FZC-003 | firmware/fzc/src/swc/swc_steering.c | TC-FZC-001 to TC-FZC-006 |
| SG-003 | FSR-007 | TSR-012, TSR-013 | SSR-FZC-004 to SSR-FZC-006 | SWR-FZC-004 to SWR-FZC-006 | firmware/fzc/src/swc/swc_steering.c | TC-FZC-007 to TC-FZC-012 |
| SG-003 | FSR-008 | TSR-014 | SSR-FZC-007 | SWR-FZC-007 | firmware/fzc/src/swc/swc_steering.c | TC-FZC-013, TC-FZC-014 |
| SG-003 | FSR-013 | TSR-022 to TSR-024 | (cross-ECU — see SG-001 row) | SWR-BSW-023 to SWR-BSW-025 | firmware/shared/bsw/services/e2e/ | TC-BSW-044 to TC-BSW-052 |
| SG-003 | FSR-016 | TSR-029, TSR-030 | SSR-SC-006, SSR-SC-007 | (SWR-SC pending) | firmware/sc/src/main.c | TC-SC-006, TC-SC-007 |

### 9.4 SG-004: Prevent Loss of Braking (ASIL D, FTTI 50 ms)

| SG | FSR | TSR | SSR | SWR | Source Module | Test Cases |
|----|-----|-----|-----|-----|--------------|------------|
| SG-004 | FSR-009 | TSR-015, TSR-016 | SSR-FZC-008, SSR-FZC-009 | SWR-FZC-009, SWR-FZC-010, SWR-FZC-012 | firmware/fzc/src/swc/swc_brake.c | TC-FZC-015 to TC-FZC-019 |
| SG-004 | FSR-010 | TSR-017 | SSR-FZC-010 | SWR-FZC-011 | firmware/fzc/src/swc/swc_brake.c | TC-FZC-020, TC-FZC-021 |
| SG-004 | FSR-025 | TSR-048 | SSR-FZC-024 | SWR-FZC-012 | firmware/fzc/src/swc/swc_brake.c | TC-FZC-046 |
| SG-004 | FSR-025 | TSR-049 | SSR-SC-015 | (SWR-SC pending) | firmware/sc/src/main.c | TC-SC-015 |

### 9.5 SG-005: Prevent Unintended Braking (ASIL A, FTTI 200 ms)

| SG | FSR | TSR | SSR | SWR | Source Module | Test Cases |
|----|-----|-----|-----|-----|--------------|------------|
| SG-005 | FSR-009 | TSR-015, TSR-016 | SSR-FZC-008, SSR-FZC-009 | SWR-FZC-009, SWR-FZC-010 | firmware/fzc/src/swc/swc_brake.c | TC-FZC-015 to TC-FZC-019 |
| SG-005 | FSR-013 | TSR-022 to TSR-024 | (cross-ECU — see SG-001 row) | SWR-BSW-023 to SWR-BSW-025 | firmware/shared/bsw/services/e2e/ | TC-BSW-044 to TC-BSW-052 |

### 9.6 SG-006: Motor Protection (ASIL A, FTTI 500 ms)

| SG | FSR | TSR | SSR | SWR | Source Module | Test Cases |
|----|-----|-----|-----|-----|--------------|------------|
| SG-006 | FSR-004 | TSR-006 | SSR-RZC-003, SSR-RZC-004 | SWR-RZC-005, SWR-RZC-006 | firmware/rzc/src/swc/swc_current.c | TC-RZC-005 to TC-RZC-008 |
| SG-006 | FSR-004 | TSR-007 | SSR-RZC-005 | SWR-RZC-007 | firmware/rzc/src/swc/swc_current.c | TC-RZC-009, TC-RZC-010 |
| SG-006 | FSR-005 | TSR-008 | SSR-RZC-006 | SWR-RZC-009 | firmware/rzc/src/swc/swc_temp.c | TC-RZC-011, TC-RZC-012 |
| SG-006 | FSR-005 | TSR-009 | SSR-RZC-007 | SWR-RZC-010 | firmware/rzc/src/swc/swc_temp.c | TC-RZC-013 to TC-RZC-015 |

### 9.7 SG-007: Obstacle Detection (ASIL C, FTTI 200 ms)

| SG | FSR | TSR | SSR | SWR | Source Module | Test Cases |
|----|-----|-----|-----|-----|--------------|------------|
| SG-007 | FSR-011 | TSR-018 | SSR-FZC-011 | SWR-FZC-013 | firmware/fzc/src/swc/swc_lidar.c | TC-FZC-022, TC-FZC-023 |
| SG-007 | FSR-011 | TSR-019 | SSR-FZC-012 | SWR-FZC-014 | firmware/fzc/src/swc/swc_lidar.c | TC-FZC-024 to TC-FZC-026 |
| SG-007 | FSR-012 | TSR-020 | SSR-FZC-013 | SWR-FZC-015 | firmware/fzc/src/swc/swc_lidar.c | TC-FZC-027, TC-FZC-028 |
| SG-007 | FSR-012 | TSR-021 | SSR-FZC-014 | SWR-FZC-016 | firmware/fzc/src/swc/swc_lidar.c | TC-FZC-029 |

### 9.8 SG-008: Independent Monitoring (ASIL C, FTTI 100 ms)

| SG | FSR | TSR | SSR | SWR | Source Module | Test Cases |
|----|-----|-----|-----|-----|--------------|------------|
| SG-008 | FSR-014 | TSR-025 | SSR-CVC-011, SSR-FZC-018, SSR-RZC-011 | SWR-CVC-020, SWR-FZC-021, SWR-RZC-021 | firmware/{cvc,fzc,rzc}/src/swc/swc_heartbeat.c | TC-CVC-039, TC-FZC-037, TC-RZC-029 |
| SG-008 | FSR-014 | TSR-026 | SSR-CVC-012, SSR-FZC-019, SSR-RZC-012 | SWR-CVC-021, SWR-FZC-022, SWR-RZC-022 | firmware/{cvc,fzc,rzc}/src/swc/swc_heartbeat.c | TC-CVC-040, TC-FZC-038, TC-RZC-030 |
| SG-008 | FSR-015 | TSR-027, TSR-028 | SSR-SC-003 to SSR-SC-005 | (SWR-SC pending) | firmware/sc/src/main.c | TC-SC-003 to TC-SC-005 |
| SG-008 | FSR-016 | TSR-029, TSR-030 | SSR-SC-006, SSR-SC-007 | (SWR-SC pending) | firmware/sc/src/main.c | TC-SC-006, TC-SC-007 |
| SG-008 | FSR-017 | TSR-031 | SSR-CVC-013, SSR-FZC-020, SSR-RZC-013, SSR-SC-008 | SWR-CVC-023, SWR-FZC-023, SWR-RZC-023, SWR-BSW-021 | firmware/{cvc,fzc,rzc}/src/swc/swc_wdg.c, firmware/sc/src/main.c | TC-CVC-043, TC-FZC-041, TC-RZC-033, TC-SC-008 |
| SG-008 | FSR-017 | TSR-032 | (HSR only) | (HSR — no SWR) | — | TC-HW-001 to TC-HW-004 |
| SG-008 | FSR-018 | TSR-033, TSR-034 | SSR-CVC-014, SSR-CVC-015 | SWR-CVC-018, SWR-CVC-019 | firmware/cvc/src/swc/swc_estop.c | TC-CVC-033 to TC-CVC-036 |
| SG-008 | FSR-020 | TSR-038 | SSR-CVC-020, SSR-FZC-021, SSR-RZC-014, SSR-SC-009 | SWR-CVC-024, SWR-FZC-024, SWR-RZC-024, SWR-BSW-004, SWR-BSW-005 | firmware/{cvc,fzc,rzc}/src/swc/swc_can.c, firmware/sc/src/main.c | TC-CVC-045, TC-FZC-043, TC-RZC-035, TC-SC-009 |
| SG-008 | FSR-020 | TSR-039 | SSR-CVC-021 | SWR-CVC-025 | firmware/cvc/src/swc/swc_can.c | TC-CVC-046, TC-CVC-047 |

---

## 10. Coverage Summary

### 10.1 Safety Traceability Coverage

| Traceability Level | Total Items | Traced Down | Coverage | Gaps |
|--------------------|-------------|-------------|----------|------|
| SG to FSR | 8 SGs | 8 (all traced to at least 1 FSR) | 100% | 0 |
| FSR to TSR | 25 FSRs | 25 (all traced to at least 1 TSR) | 100% | 0 |
| TSR to SSR | 51 TSRs | 49 (TSR-032 is HW-only; TSR-009 has SSR) | 96% | 1 (TSR-032 is HSR-only, by design) |
| TSR to HSR | 51 TSRs | 22 (only TSRs with HW implications) | 43% | N/A (not all TSRs have HW implications) |
| SSR to SWR (CVC) | 23 SSR-CVC | 23 (mapped via SWR-CVC) | 100% | 0 |
| SSR to SWR (FZC) | 24 SSR-FZC | 24 (mapped via SWR-FZC) | 100% | 0 |
| SSR to SWR (RZC) | 17 SSR-RZC | 17 (mapped via SWR-RZC) | 100% | 0 |
| SSR to SWR (SC) | 17 SSR-SC | 0 (SWR-SC is stub v0.1) | 0% | 17 (pending SWR-SC completion) |

### 10.2 ASPICE Traceability Coverage

| Traceability Level | Total Items | Traced Down | Coverage | Gaps |
|--------------------|-------------|-------------|----------|------|
| STK to SYS | 32 STK | 31 traced + 1 process (STK-027) | 97% | 0 (STK-027 is process scope) |
| SYS to SWR (all ECUs) | 56 SYS | 48 traced to at least 1 SWR | 86% | 8 (see gap analysis) |
| SWR to Source | 161 total SWR | 0 (not yet implemented) | 0% | 161 (pending Phase 1+) |
| Source to Test | — | 0 (not yet implemented) | 0% | — (pending Phase 1+) |

### 10.3 SWR Count per Document

| Document | SWR Count | ASIL D | ASIL C | ASIL B | ASIL A | QM |
|----------|-----------|--------|--------|--------|--------|-----|
| SWR-CVC | 35 | 18 | 6 | 2 | 0 | 9 |
| SWR-FZC | 32 | 16 | 8 | 0 | 0 | 8 |
| SWR-RZC | 30 | 10 | 7 | 0 | 5 | 8 |
| SWR-SC | 1 (stub) | — | — | — | — | — |
| SWR-BCM | 12 | 0 | 0 | 0 | 0 | 12 |
| SWR-ICU | 10 | 0 | 0 | 0 | 0 | 10 |
| SWR-TCU | 15 | 0 | 0 | 0 | 0 | 15 |
| SWR-BSW | 27 | 21 | 0 | 3 | 1 | 2 |
| **Total** | **162** | **65** | **21** | **5** | **6** | **64** |

### 10.4 Requirement Totals Across All Levels

| Level | Count |
|-------|-------|
| Stakeholder Requirements (STK) | 32 |
| System Requirements (SYS) | 56 |
| Safety Goals (SG) | 8 |
| Functional Safety Requirements (FSR) | 25 |
| Technical Safety Requirements (TSR) | 51 |
| Software Safety Requirements (SSR) | 81 |
| Hardware Safety Requirements (HSR) | 25 |
| Software Requirements (SWR) | 162 |
| **Grand Total** | **440** |

### 10.5 ASIL Distribution Summary

| ASIL | SG | FSR | TSR | SSR | HSR | SWR | Total |
|------|----|-----|-----|-----|-----|-----|-------|
| D | 3 | 13 | 26 | 43 | 15 | 65 | 165 |
| C | 2 | 7 | 17 | 23 | 3 | 21 | 73 |
| B | 1 | 2 | 4 | 6 | 3 | 5 | 21 |
| A | 1 | 2 | 4 | 5 | 2 | 6 | 20 |
| QM | 0 | 0 | 0 | 0 | 1 | 64 | 65 |
| **Total** | **8** | **25** | **51** | **81** | **25** | **162** | **344** |

Note: Totals exclude STK and SYS from ASIL count as their ASIL assignment is not always 1:1 (some SYS requirements span multiple ASIL levels depending on the safety goal they support).

---

## 11. Gap Analysis

### 11.1 Orphan Requirements Check

| Check | Result | Details |
|-------|--------|---------|
| Safety goals without FSR children | PASS | All 8 SGs trace to at least 1 FSR |
| FSRs without SG parent | PASS | All 25 FSRs trace to at least 1 SG |
| FSRs without TSR children | PASS | All 25 FSRs trace to at least 1 TSR |
| TSRs without FSR parent | PASS | All 51 TSRs trace to at least 1 FSR |
| TSRs without SSR or HSR child | PASS | All 51 TSRs trace to at least 1 SSR or HSR (TSR-032 is HSR-only) |
| SSRs without TSR parent | PASS | All 81 SSRs trace to at least 1 TSR |
| STK without SYS child | PASS | 31 of 32 STK trace directly; STK-027 is process-scope |
| SYS without STK parent | PASS | All 56 SYS trace to at least 1 STK |
| SWR without SYS parent | PASS | All SWR documents contain SYS-to-SWR mapping |

### 11.2 Identified Gaps

| Gap ID | Level | Description | Impact | Resolution |
|--------|-------|-------------|--------|------------|
| GAP-001 | SWR-SC | SWR-SC document is a v0.1 stub — SC has 17 SSR-SC requirements without corresponding SWR-SC | SC software requirements not formally documented at SWE.1 level | Complete SWR-SC document in a subsequent phase. Interim traceability via SSR-SC (Section 8.1). |
| GAP-002 | SYS to SWR | 8 SYS requirements (SYS-042, SYS-043, SYS-051, SYS-052, SYS-054, SYS-055, SYS-056, SYS-026) have no SWR child | These are cloud/telemetry (SYS-042, SYS-043), non-functional (SYS-051 to SYS-056), or process-level requirements | SYS-042, SYS-043 will be addressed by gateway software (Python, not SWR scope). SYS-051 to SYS-056 are non-functional/architectural constraints verified by analysis and inspection. SYS-026 is a process constraint (independent monitoring architecture). |
| GAP-003 | Source | No source code exists yet for any SWR | 0% code coverage | Expected — project is in Phase 0 (architecture and requirements). Code implementation begins in Phase 1. |
| GAP-004 | Test | No test cases have been executed | 0% test coverage | Expected — test execution follows code implementation. Test case IDs (TC-xxx) are allocated in all SWR documents. |

### 11.3 Coverage Gap Disposition

| Gap Category | Count | Acceptable | Rationale |
|--------------|-------|------------|-----------|
| SWR-SC stub | 17 SSR | Yes (interim) | SSR-SC provides equivalent content; SWR-SC will be completed before SWE.2 |
| SYS without SWR (cloud/non-functional) | 8 SYS | Yes | These requirements are outside the embedded SWR scope (cloud, process, or architectural constraints) |
| Source code not yet written | 162 SWR | Yes | Phase 0 (requirements) — code implementation is Phase 1 |
| Tests not yet executed | All TC-xxx | Yes | Phase 0 (requirements) — test execution follows implementation |

---

## 12. ECU Allocation Summary

### 12.1 Requirements per ECU

| ECU | Type | SWR Count | SSR Count | HSR Count | Safety-Critical |
|-----|------|-----------|-----------|-----------|-----------------|
| CVC (STM32G474RE) | Physical | 35 | 23 | 5 | Yes (ASIL D) |
| FZC (STM32G474RE) | Physical | 32 | 24 | 7 | Yes (ASIL D) |
| RZC (STM32G474RE) | Physical | 30 | 17 | 7 | Yes (ASIL D) |
| SC (TMS570LC43x) | Physical | 1 (stub) | 17 | 6 | Yes (ASIL D) |
| BCM (Docker) | Simulated | 12 | 0 | 0 | No (QM) |
| ICU (Docker) | Simulated | 10 | 0 | 0 | No (QM) |
| TCU (Docker) | Simulated | 15 | 0 | 0 | No (QM) |
| BSW (shared) | Library | 27 | — | — | Yes (ASIL D) |
| **Total** | | **162** | **81** | **25** | |

### 12.2 TSR Allocation per ECU

| ECU | TSRs Allocated |
|-----|---------------|
| CVC | TSR-001, TSR-002, TSR-003, TSR-004, TSR-022, TSR-023, TSR-024, TSR-025, TSR-026, TSR-031, TSR-033, TSR-034, TSR-035, TSR-036, TSR-037, TSR-038, TSR-039, TSR-043, TSR-046, TSR-047 |
| FZC | TSR-010, TSR-011, TSR-012, TSR-013, TSR-014, TSR-015, TSR-016, TSR-017, TSR-018, TSR-019, TSR-020, TSR-021, TSR-022, TSR-023, TSR-024, TSR-025, TSR-026, TSR-031, TSR-038, TSR-044, TSR-046, TSR-047, TSR-048 |
| RZC | TSR-005, TSR-006, TSR-007, TSR-008, TSR-009, TSR-022, TSR-023, TSR-024, TSR-025, TSR-026, TSR-031, TSR-038, TSR-040, TSR-046, TSR-047 |
| SC | TSR-022, TSR-024, TSR-027, TSR-028, TSR-029, TSR-030, TSR-031, TSR-038, TSR-041, TSR-042, TSR-045, TSR-046, TSR-049, TSR-050, TSR-051 |

---

## 13. Verification Method Summary

### 13.1 Verification Methods by Level

| Level | Test | Analysis | Inspection | Demonstration |
|-------|------|----------|------------|---------------|
| SYS | 42 | 6 | 1 | 7 |
| SWR-CVC | 28 unit, 18 SIL, 10 PIL | 1 WCET | — | 2 |
| SWR-FZC | 22 unit, 16 SIL, 8 PIL | 1 WCET | — | 2 |
| SWR-RZC | 22 unit, 14 SIL, 10 PIL | 1 WCET | — | — |
| SWR-BCM | 21 test cases planned | — | — | — |
| SWR-ICU | 18 test cases planned | — | — | — |
| SWR-TCU | 35 test cases planned | — | — | — |
| SWR-BSW | 52 test cases planned | — | — | — |

### 13.2 Test Case ID Ranges

| Document | Test Case Range | Count |
|----------|----------------|-------|
| TC-CVC-xxx | TC-CVC-001 to TC-CVC-070 | ~70 |
| TC-FZC-xxx | TC-FZC-001 to TC-FZC-064 | ~64 |
| TC-RZC-xxx | TC-RZC-001 to TC-RZC-060 | ~60 |
| TC-SC-xxx | TC-SC-001 to TC-SC-034 | ~34 |
| TC-BCM-xxx | TC-BCM-001 to TC-BCM-021 | 21 |
| TC-ICU-xxx | TC-ICU-001 to TC-ICU-018 | 18 |
| TC-TCU-xxx | TC-TCU-001 to TC-TCU-035 | 35 |
| TC-BSW-xxx | TC-BSW-001 to TC-BSW-052 | 52 |
| **Total** | | **~354** |

---

## 14. FTTI Compliance Traceability

This section links each safety goal's FTTI to the timing chain of requirements that ensures compliance.

| SG | FTTI | Detection Req | Detection Budget | Reaction Req | Reaction Budget | Actuation Req | Actuation Budget | Margin | Compliance |
|----|------|--------------|------------------|-------------|-----------------|--------------|------------------|--------|------------|
| SG-001 | 50 ms | TSR-002 (pedal plausibility) | 20 ms | TSR-004 (CVC zero-torque) | 10 ms | TSR-005 (RZC motor disable) | 5 ms | 15 ms | PASS |
| SG-002 | 200 ms | TSR-009 (motor health) | 50 ms | TSR-036 (state broadcast) | 20 ms | TSR-005 (controlled ramp) | 100 ms | 30 ms | PASS |
| SG-003 | 100 ms | TSR-011 (steering feedback) | 50 ms | TSR-012 (return-to-center) | 10 ms | TSR-013 (servo travel) | 30 ms | 10 ms | PASS |
| SG-004 | 50 ms | TSR-015 (brake PWM check) | 10 ms | TSR-016 (fault notify) | 5 ms | TSR-048 (motor cutoff) | 15 ms | 20 ms | PASS |
| SG-005 | 200 ms | TSR-015 (brake plausibility) | 20 ms | TSR-016 (brake release) | 10 ms | — (servo return) | 100 ms | 70 ms | PASS |
| SG-006 | 500 ms | TSR-006 (current sampling) | 100 ms | TSR-009 (derating) | 10 ms | — (GPIO write) | 1 ms | 389 ms | PASS |
| SG-007 | 200 ms | TSR-018 (lidar UART) | 50 ms | TSR-019 (graduated response) | 10 ms | TSR-017 (brake ramp) | 100 ms | 40 ms | PASS |
| SG-008 | 100 ms | TSR-027 (heartbeat timeout) | 50 ms | TSR-028 (confirmation) | 5 ms | TSR-029 (relay dropout) | 10 ms | 35 ms | PASS |

---

## 15. Generation Notes

### 15.1 Manual vs. Automated

This traceability matrix was manually compiled from the requirement documents listed in Section 3. Future updates should be generated automatically using:

```bash
python scripts/trace-gen.py  # Scans docs/ and firmware/ for trace tags
```

The trace-gen.py script scans for the following tags:
- `Traces up:` in requirement documents
- `Traces down:` in requirement documents
- `@safety_req` in source code files
- `@traces_to` in source code files
- `@verifies` in test files

### 15.2 Update Triggers

This document shall be updated whenever:
- A new requirement is added at any level (SG, FSR, TSR, SSR, HSR, SWR)
- A requirement is modified, deleted, or decomposed
- Source code is written that implements a requirement
- Test cases are created or executed
- A new ECU or module is added to the platform

### 15.3 Review Schedule

| Review | Frequency | Reviewer |
|--------|-----------|----------|
| Traceability completeness check | Every phase gate | Configuration Manager |
| Gap analysis update | Monthly during development | System Engineer |
| ASIL distribution audit | Every SWE.1 review | Functional Safety Engineer |
| Independent traceability verification | Before each release baseline | Independent Reviewer (I3) |

---

## 16. Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 0.1 | 2026-02-21 | System | Initial stub with placeholder matrix |
| 1.0 | 2026-02-21 | System | Complete traceability matrix: SG-to-FSR (8 SGs, 25 FSRs), FSR-to-TSR (25 FSRs, 51 TSRs), TSR-to-SSR (81 SSRs), TSR-to-HSR (25 HSRs), SYS-to-SWR (162 SWRs across 8 documents), end-to-end safety chains for all 8 SGs, coverage summary, gap analysis, FTTI compliance, ECU allocation, verification method summary |

