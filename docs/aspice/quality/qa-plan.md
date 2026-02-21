---
document_id: QAP
title: "Quality Assurance Plan"
version: "0.1"
status: planned
aspice_process: SUP.1
---

# Quality Assurance Plan

<!-- Phase 0 deliverable -->

## Purpose

Define QA activities, responsibilities, and criteria per ASPICE SUP.1.

## QA Activities

| Activity | Frequency | Method |
|----------|-----------|--------|
| Code review | Every PR | GitHub PR review |
| Static analysis | Every build | cppcheck + MISRA |
| Unit test execution | Every build | Unity + pytest |
| Traceability audit | Per phase | Manual review |
| Safety review | Per phase | Independent review |

## Quality Criteria

| Criterion | Target |
|-----------|--------|
| MISRA mandatory violations | 0 |
| Unit test pass rate | 100% |
| MC/DC coverage (ASIL D) | 100% |
| Traceability gaps | 0 |
| Open critical defects at release | 0 |

## Review Schedule

| Phase | Review Type | Scope |
|-------|------------|-------|
| 1 | Safety concept review | HARA, safety goals |
| 3 | Requirements review | TSR, SSR, HSR |
| 5 | Architecture review | BSW design |
| 12 | Verification review | Test results, coverage |
| 14 | Release review | Safety case, traceability |
