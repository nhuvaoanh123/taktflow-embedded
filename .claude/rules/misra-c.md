---
paths:
  - "firmware/**/*.c"
  - "firmware/**/*.h"
  - "firmware/**/*.cpp"
  - "firmware/**/*.hpp"
---

# MISRA C Coding Standard

## Rule Classification at ASIL D

| Category | Compliance | Deviations |
|----------|-----------|------------|
| **Mandatory** | ZERO deviations | None allowed |
| **Required** | Full compliance | Formal deviation with rationale, risk assessment, independent review |
| **Advisory** | Tracked | Compliance expected at ASIL D |

## Critical Rules (quick reference)

**Type Safety**: 10.1, 10.3, 10.4, 10.8 (implicit conversions), 11.1, 11.3 (pointer casts)
**Control Flow**: 14.3 (invariant expressions), 15.1 (no goto), 15.7 (else required), 16.4 (default label), 17.2 (no recursion)
**Memory**: 17.7 (check return values), 18.1 (array bounds), 21.3 + Dir 4.12 (no malloc)
**Side Effects**: 2.2 (no dead code), 13.2 (evaluation order), 13.5 (no side effects in && ||)
**Preprocessor**: 20.1, 20.4, 20.7 (macro safety)

## Deviation Process

1. Document: rule, location, justification, risk assessment, compensating measures
2. Independent review by someone other than author
3. Approval by safety engineer
4. Track in deviation database, link to affected code

## Enforcement

- Automated MISRA checker MANDATORY (cppcheck with MISRA addon for this project)
- MISRA compliance report per build
- Zero mandatory violations in release builds
- All required violations either fixed or formally deviated
