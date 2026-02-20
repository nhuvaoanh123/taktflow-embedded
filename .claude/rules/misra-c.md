---
paths:
  - "firmware/**/*.c"
  - "firmware/**/*.h"
  - "firmware/**/*.cpp"
  - "firmware/**/*.hpp"
---

# MISRA C Coding Standard (ISO 26262 Part 6 Referenced)

## Standard Versions

| Version | Rules | Directives | Total |
|---------|-------|------------|-------|
| MISRA C:2012 (base) | 143 | 16 | 159 |
| + Amendment 1 (2016) | 156 | 17 | 173 |
| + Amendment 2 (2020) | 158 | 17 | 175 |
| MISRA C:2023 (consolidated) | 200 | 21 | 221 |
| MISRA C:2025 (latest) | — | — | 225 active |

## Rule Classification

| Category | Compliance at ASIL D | Deviations |
|----------|---------------------|------------|
| **Mandatory** | ZERO deviations permitted | None allowed under any circumstance |
| **Required** | Full compliance expected | Formal documented deviation with rationale, risk assessment, independent review |
| **Advisory** | Compliance tracked | Formal deviation not required, but compliance expected at ASIL D |

## Key Mandatory Rules (no exceptions)

- **Rule 1.3**: No undefined or critical unspecified behavior
- **Rule 13.6**: sizeof operand shall not have side effects
- **Rule 19.1**: No assignment to overlapping objects
- **Rule 21.13**: No undefined behavior with ctype.h functions
- **Rule 21.17-21.18**: String handling shall not result in out-of-bounds access
- **Rule 22.1**: No undefined behavior with memory allocation (if used)

## Critical Required Rules

### Type Safety
- **Rule 10.1**: No implicit conversion of operands
- **Rule 10.3**: No implicit narrowing conversion
- **Rule 10.4**: Both operands shall have same essential type category
- **Rule 10.8**: No cast to wider essential type of a composite expression
- **Rule 11.1**: No conversion between function pointer and other type
- **Rule 11.3**: No cast between pointer to object and pointer to different type

### Control Flow
- **Rule 14.3**: Controlling expression shall not be invariant
- **Rule 15.1**: goto shall not be used (or only for error-exit patterns)
- **Rule 15.7**: All if...else if chains shall have a final else
- **Rule 16.4**: Every switch shall have a default label
- **Rule 17.2**: No recursion (direct or indirect)

### Memory & Pointers
- **Rule 17.7**: Return values shall not be discarded
- **Rule 18.1**: No pointer arithmetic beyond array bounds
- **Rule 18.6**: No address of automatic variable outside its scope
- **Rule 21.3**: No dynamic memory allocation (malloc, calloc, realloc, free)
- **Directive 4.12**: Dynamic memory allocation shall not be used

### Side Effects
- **Rule 2.2**: No dead code
- **Rule 13.2**: No dependence on evaluation order of operands
- **Rule 13.5**: RHS of && and || shall not have persistent side effects

### Preprocessor
- **Rule 20.1**: #include preceded only by preprocessor directives or comments
- **Rule 20.4**: No macro defined with same name as keyword
- **Rule 20.7**: Macro parameters in expansion shall be parenthesized

## Deviation Process at ASIL D

When a required rule must be deviated:

1. **Document the deviation** with:
   - Rule number and text
   - Specific location in code (file, line)
   - Technical justification for why the rule cannot be followed
   - Risk assessment of the deviation
   - Compensating measures (additional testing, review, etc.)
2. **Independent review** of the deviation by someone other than the author
3. **Approval** by safety engineer or project lead
4. **Traceability** — deviation tracked in deviation database, linked to affected code

## Enforcement

- Automated MISRA checking tool MANDATORY (e.g., Polyspace, Axivion, QA-C, PC-lint, cppcheck with MISRA addon)
- Tool must be qualified per ISO 26262 Part 8 (typically TCL2)
- MISRA compliance report generated for every build
- Zero mandatory rule violations in release builds
- All required rule violations either fixed or formally deviated
- Advisory violations reviewed and disposition documented

## MISRA C++ (if using C++)

- MISRA C++:2023 or AUTOSAR C++14 guidelines apply
- Same deviation process as MISRA C
- Additional restrictions: no exceptions, no RTTI, limited template usage
- C++ standard library usage restricted to analyzed subset
