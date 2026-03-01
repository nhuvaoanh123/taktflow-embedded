---
paths:
  - "firmware/**/*"
  - "hardware/**/*"
  - "docs/safety/**/*"
  - "docs/aspice/**/*"
---

# ISO 26262 & Automotive Compliance

## Applicable Standards

| Standard | Applicability |
|----------|---------------|
| **ISO 26262:2018** | PRIMARY — all 12 parts |
| **Automotive SPICE 4.0** | REQUIRED — OEM supplier qualification |
| **ISO/SAE 21434** | REQUIRED — automotive cybersecurity |
| **MISRA C:2012/2023** | REQUIRED — referenced by Part 6 |

## ASIL Levels

| ASIL | Risk | Examples |
|------|------|---------|
| QM | Tolerable | Infotainment, comfort |
| A-B | Low-Moderate | Lights, cruise control |
| C | High | Power steering |
| **D** | **Highest** | **Airbag, ABS, autonomous braking, EPS** |

**ASIL D** = S3 (fatal) + E4 (high exposure) + C3 (uncontrollable)

## Cross-Reference to Detailed Rules

- `asil-d.md` — All ASIL D constraints (Parts 4-6, 8-9)
- `misra-c.md` — MISRA C coding standard
- `aspice.md` — ASPICE 4.0 process requirements
- `safety-lifecycle.md` — HARA, safety case, safety plan
- `tool-qualification.md` — Tool chain qualification (Part 8)
- `traceability.md` — Bidirectional traceability
- Full reference tables: `docs/reference/asil-d-reference.md`

## TODO Markers

```c
// TODO:ISO — ISO 26262 Part X, Section Y: description
// TODO:ASIL — ASIL D specific requirement: description
// TODO:ASPICE — ASPICE SWE.X base practice: description
// TODO:MISRA — MISRA C:2012 Rule X.Y: description
```

## Documentation Retention

All safety-relevant documentation must be retained for **vehicle lifetime + 15 years** (product liability).
