---
paths:
  - "**/*"
---

# Safety Lifecycle (ISO 26262 Parts 2, 3, 4)

## Functional Safety Engineer (FSE) Role

### Responsibilities
- Develop and maintain the safety plan
- Lead Hazard Analysis and Risk Assessment (HARA)
- Derive safety goals, FSR, TSR, SSR, HSR
- Perform/oversee FMEA, FTA, DFA safety analyses
- Create functional safety concept and technical safety concept
- Compile and maintain the safety case
- Coordinate with independent assessors (TUV, SGS, etc.)
- Manage supplier safety requirements (DIA)
- Assess safety implications of all design changes
- Promote functional safety culture

### Competency & Certification

| Certification | Provider | Notes |
|---------------|----------|-------|
| FSCP | TUV SUD | Engineer, Professional, Expert levels |
| FSEng | TUV Rheinland | Training + exam, dominant in German automotive |
| AFSP/AFSE | SGS-TUV Saar | Professional + Expert levels |
| CFSP/CFSE | exida | Decoupled from training; CFSE = 10 yrs experience |

## Safety Plan (Part 2)

The governing document for all functional safety activities:

| Element | Content |
|---------|---------|
| Project scope | Lifecycle phases in scope, item definition |
| Roles & responsibilities | Safety manager, FSEs, project manager, assessors |
| Safety activities | Mapped to each lifecycle phase |
| Confirmation measures | Schedule for reviews, audits, assessments |
| Work products | Deliverables per phase |
| Tools & methods | FMEA/FTA tools, tool qualification status |
| Resources & training | Competency requirements |
| Schedule | Aligned with project milestones |
| Safety case planning | Preliminary -> interim -> final |
| Supplier management | DIA, flow-down of safety requirements |

## Hazard Analysis and Risk Assessment (HARA) — Part 3

### Process
1. **Item definition**: Functions, interfaces, environment, legal requirements
2. **Situation analysis**: Operational situations, driving scenarios, environmental conditions
3. **Hazard identification**: Malfunctioning behaviors + hazardous events
4. **Risk assessment**: For each hazardous event, assess S, E, C
5. **ASIL determination**: Combine S + E + C using the determination matrix

### Outputs
- HARA document with hazard list and ASIL assignments
- **Safety goals**: Top-level safety requirements (one per hazardous event)
- **Safe states**: Defined for each safety goal
- **Fault Tolerant Time Interval (FTTI)**: Max time to reach safe state

### Participants
Led by FSE, cross-functional workshop with: system engineers, HW engineers, SW engineers, test engineers, domain experts (vehicle dynamics, etc.)

## Safety Requirements Flow-Down

```
Safety Goals (SG)                     — Vehicle level, from HARA
    |
    v
Functional Safety Requirements (FSR)  — WHAT must be done (Part 3)
    |                                    Safety mechanisms, safe states,
    |                                    warning concepts, degradation
    v
Technical Safety Requirements (TSR)   — HOW to do it (Part 4)
    |                                    Allocated to system elements,
    |                                    includes HSI specification
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

### Three Constituents

1. **Claims**: Safety goals, safety requirements — what the system must achieve
2. **Argument**: Logical reasoning connecting evidence to claims (GSN, CAE, or narrative)
3. **Evidence**: Work products substantiating the argument

### Safety Case Versions
- **Preliminary**: After concept phase
- **Interim**: During development (updated as phases complete)
- **Final**: Before production release

### Evidence Includes
- HARA results
- Safety analyses (FMEA, FTA, DFA)
- Verification and validation reports
- Confirmation review records
- Test reports (unit, integration, system)
- Configuration management records
- Tool qualification records
- Safety audit reports

## Dependent Failure Analysis (DFA) — Part 9

### Common Cause Failures (CCF)
Two+ elements fail simultaneously from a single shared root cause.
- Coupling factors: shared power, physical proximity, common design, shared manufacturing, shared software

### Cascading Failures (CF)
Failure of one element causes subsequent failure of another.
- Propagation: data flow, control flow, power flow, mechanical coupling

### DFA Process
1. Identify elements with assumed independence
2. Identify coupling factors between them
3. Determine if CCF/CF could violate a safety goal
4. Implement mitigations (diverse HW, physical separation, independent power)
5. Document analysis and residual risk

## Safety Validation (Part 4)

- Demonstrate safety goals met at vehicle level
- Vehicle-level testing in intended use environment
- Methods: HIL, track testing, field data
- Results documented in safety validation report
- Reviewed by independent assessor at ASIL D
