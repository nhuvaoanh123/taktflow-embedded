# MISRA C:2012 Deviation Register

**Project:** Taktflow Embedded — Zonal Vehicle Platform
**Standard:** MISRA C:2012 / MISRA C:2023
**ASIL:** D (ISO 26262 Part 6)
**Tool:** cppcheck with MISRA addon
**Last Updated:** 2026-02-24

---

## Purpose

Per ISO 26262 Part 6, Section 8.4.6 and MISRA Compliance:2020, any deviation from a
**required** MISRA rule must be formally documented with:

1. Rule number and headline text
2. Specific code location (file:line)
3. Technical justification for why the rule cannot be followed
4. Risk assessment of the deviation
5. Compensating measure (additional testing, review, etc.)
6. Independent reviewer sign-off

**Mandatory rules** permit ZERO deviations. Advisory rules do not require formal
deviation but are tracked for completeness at ASIL D.

---

## Deviation Summary

| ID | Rule | Category | File(s) | Status |
|----|------|----------|---------|--------|
| — | — | — | — | No deviations recorded yet |

---

## Deviation Template

### DEV-NNN: Rule X.Y — [Rule Headline]

- **Category:** Required / Advisory
- **Location:** `file.c:line`
- **Code:**
  ```c
  // the violating code
  ```
- **Justification:** [Why this rule cannot be followed in this specific case]
- **Risk Assessment:** [What could go wrong, likelihood, impact]
- **Compensating Measure:** [Additional testing, code review, runtime check, etc.]
- **Reviewed By:** [Name, date]
- **Approved By:** [Name, date]

---

## Process Notes

- Deviations are only created during triage of MISRA analysis results (Phase 5)
- Each deviation must be reviewed by someone other than the code author
- The deviation register is part of the ISO 26262 safety case evidence
- Deviations must be re-assessed when affected code changes
- This register is auditable by external assessors (TUV, SGS, exida)
