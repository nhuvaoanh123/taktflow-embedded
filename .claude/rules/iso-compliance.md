---
paths:
  - "**/*"
---

# ISO 26262 & Automotive Compliance Overview

## Applicable Standards

| Standard | Domain | Applicability |
|----------|--------|---------------|
| **ISO 26262:2018** | Automotive functional safety | PRIMARY — all 12 parts apply |
| **Automotive SPICE 4.0** | Process assessment model | REQUIRED — OEM supplier qualification |
| **ISO/SAE 21434** | Automotive cybersecurity | REQUIRED — complements ISO 26262 |
| **MISRA C:2012/2023** | C coding standard | REQUIRED — referenced by ISO 26262 Part 6 |
| IEC 62443 | Industrial cybersecurity | Applicable to factory/fleet systems |
| IEC 61508 | Generic functional safety | Parent standard of ISO 26262 |
| ETSI EN 303 645 | Consumer IoT security | Applicable if consumer-facing |

## ISO 26262 Structure (12 Parts)

| Part | Title | Scope |
|------|-------|-------|
| 1 | Vocabulary | Definitions, abbreviations |
| 2 | Management of Functional Safety | Safety plan, safety case, competence, culture |
| 3 | Concept Phase | Item definition, HARA, ASIL determination, safety goals |
| 4 | System Level Development | Technical safety concept, system design, integration |
| 5 | Hardware Level Development | HW safety requirements, FMEDA, architectural metrics |
| 6 | Software Level Development | SW safety requirements, architecture, unit design, testing |
| 7 | Production & Operations | Manufacturing, service, decommissioning |
| 8 | Supporting Processes | Config mgmt, change mgmt, tool qualification, verification |
| 9 | Safety Analyses | ASIL decomposition, dependent failure analysis, FMEA/FTA |
| 10 | Guidelines | Non-normative guidance |
| 11 | Semiconductors | IC, ASIC, FPGA specific guidance |
| 12 | Motorcycles | Two-wheeler adaptation |

## ASIL Determination Matrix

ASIL is determined by Severity (S), Exposure (E), Controllability (C):

| Severity | Exposure | C1 | C2 | C3 |
|----------|----------|-----|-----|-----|
| S1 | E1 | QM | QM | QM |
| S1 | E2 | QM | QM | QM |
| S1 | E3 | QM | QM | A |
| S1 | E4 | QM | A | B |
| S2 | E1 | QM | QM | QM |
| S2 | E2 | QM | QM | A |
| S2 | E3 | QM | A | B |
| S2 | E4 | A | B | C |
| S3 | E1 | QM | QM | A |
| S3 | E2 | QM | A | B |
| S3 | E3 | A | B | C |
| S3 | E4 | B | C | **D** |

**ASIL D** = S3 (fatal/life-threatening) + E4 (high probability) + C3 (uncontrollable)

## ASIL Level Summary

| ASIL | Risk | Example Systems |
|------|------|-----------------|
| QM | Tolerable | Infotainment, comfort |
| A | Low | Rear lights, horn |
| B | Moderate | Headlights, cruise control |
| C | High | Power steering, HVAC safety |
| **D** | **Highest** | **Airbag, ABS, autonomous braking, EPS torque** |

## Cross-Reference to Detailed Rules

All ASIL D specific constraints are in dedicated rule files:

- `asil-d-software.md` — Software development (Part 6), coding, coverage, defensive programming
- `asil-d-hardware.md` — Hardware metrics (Part 5), SPFM, LFM, PMHF, redundancy
- `asil-d-architecture.md` — System architecture (Part 4), FFI, timing, communication
- `asil-d-verification.md` — Verification/validation methods, independence requirements
- `misra-c.md` — MISRA C:2012/2023 coding standard requirements
- `aspice.md` — Automotive SPICE 4.0 process requirements
- `safety-lifecycle.md` — Safety lifecycle, HARA, safety case, safety plan
- `tool-qualification.md` — Tool chain qualification (Part 8)
- `traceability.md` — Bidirectional traceability requirements
- `asil-decomposition.md` — Decomposition rules and constraints

## TODO Markers for ISO Compliance

```c
// TODO:ISO — ISO 26262 Part X, Section Y: description
// TODO:ASIL — ASIL D specific requirement: description
// TODO:ASPICE — ASPICE SWE.X base practice: description
// TODO:MISRA — MISRA C:2012 Rule X.Y: description
```

## Documentation Retention

All safety-relevant documentation must be retained for the **lifetime of the vehicle** (production lifetime + 15 years for product liability).
