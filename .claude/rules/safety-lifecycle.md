---
paths:
  - "firmware/**/*"
  - "hardware/**/*"
  - "docs/safety/**/*"
  - "docs/aspice/**/*"
---

# Safety Lifecycle (ISO 26262 Parts 2, 3, 4)

## Safety Plan (Part 2)

The governing document for all functional safety activities. Must cover: project scope, roles, safety activities per phase, confirmation measures, work products, tools & qualification status, resources, schedule, safety case planning, supplier management (DIA).

## HARA Process (Part 3)

1. Item definition (functions, interfaces, environment)
2. Situation analysis (operational situations, driving scenarios)
3. Hazard identification (malfunctioning behaviors)
4. Risk assessment (Severity, Exposure, Controllability per hazardous event)
5. ASIL determination (S + E + C matrix)

**Outputs**: Safety goals, safe states, FTTI per safety goal.
**Led by FSE**, cross-functional workshop.

## Safety Requirements Flow-Down

```
Safety Goals (SG)                     — Vehicle level, from HARA
    |
    v
Functional Safety Requirements (FSR)  — WHAT must be done (Part 3)
    |
    v
Technical Safety Requirements (TSR)   — HOW to do it (Part 4)
    |
    v
 +----------+----------+
 |                      |
 v                      v
Hardware Safety     Software Safety
Requirements        Requirements
(HSR, Part 5)      (SSR, Part 6)
```

Each level: bidirectional traceability to levels above and below.

## Safety Case (Part 2)

Three constituents: **Claims** (safety goals/requirements), **Argument** (GSN/CAE/narrative connecting evidence to claims), **Evidence** (work products).

Versions: Preliminary (after concept) → Interim (during dev) → Final (before production).

Evidence includes: HARA, FMEA/FTA/DFA, verification reports, test reports, CM records, tool qualification records, safety audit reports.

## Dependent Failure Analysis (DFA) — Part 9

- **CCF**: Two+ elements fail from shared root cause (shared power, proximity, common design)
- **Cascading**: One element's failure causes another's (data/control/power flow)
- Process: identify independent elements → identify coupling factors → determine if safety goal violated → mitigate → document

## Safety Validation (Part 4)

- Demonstrate safety goals met at vehicle level
- Methods: HIL, track testing, field data
- Reviewed by independent assessor at ASIL D (I3)
