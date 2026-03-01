---
paths:
  - "firmware/**/*"
  - "hardware/**/*"
  - "docs/safety/**/*"
  - "docs/aspice/**/*"
---

# ASPICE 4.0 Process Requirements

**Target**: Level 2 minimum (OEM requirement), Level 3 for ASIL D.

## Process Areas

| Process | Name | Key Output |
|---------|------|------------|
| SYS.1 | Requirements Elicitation | Stakeholder requirements specification |
| SYS.2 | System Requirements Analysis | SRS, verification criteria |
| SYS.3 | System Architectural Design | System architecture, ICD |
| SYS.4 | System Integration & Test | Integration test plan, results |
| SYS.5 | System Verification | Verification test plan, results |
| SWE.1 | SW Requirements Analysis | SRS, verification criteria |
| SWE.2 | SW Architectural Design | SW architecture, interfaces |
| SWE.3 | SW Detailed Design & Unit Construction | Detailed design, source code |
| SWE.4 | SW Unit Verification | Unit tests, static analysis, coverage |
| SWE.5 | SW Component Verification & Integration | Integration strategy, results |
| SWE.6 | SW Verification | Qualification tests, release notes |
| HWE.1-4 | Hardware Engineering (new in 4.0) | HW reqs, design, verification |
| SUP.1 | Quality Assurance | Independent QA |
| SUP.8 | Configuration Management | Baselines, CM audits |
| MAN.3 | Project Management | Project plan, schedule, risks |

## V-Model Rule

Every specification activity (left side) has a corresponding verification activity (right side): SYS.2↔SYS.5, SYS.3↔SYS.4, SWE.1↔SWE.6, SWE.2↔SWE.5, SWE.3↔SWE.4.

## ASPICE + ISO 26262

- ASPICE = process maturity; ISO 26262 = safety integrity — both required, complementary
- "Work Products" renamed to "Information Items" in 4.0 — content over format
