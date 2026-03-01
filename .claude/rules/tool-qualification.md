---
paths:
  - "scripts/**/*"
  - "firmware/**/*"
---

# Tool Qualification (ISO 26262 Part 8)

## TCL Matrix

Tool Impact (TI) x Tool Error Detection (TD) = Tool Confidence Level:

| | TD1 (high) | TD2 (medium) | TD3 (low) |
|---|---|---|---|
| **TI1** (can't introduce errors) | TCL1 | TCL1 | TCL1 |
| **TI2** (can introduce errors) | TCL1 | TCL2 | **TCL3** |

- TCL1: no qualification needed
- TCL2/TCL3: qualification required

## Common Tool Classifications

| Tool | TCL | Notes |
|------|-----|-------|
| Compiler | TCL2-3 | Miscompilation = silent safety defect |
| Static analysis / MISRA checker | TCL2 | Can fail to detect errors |
| Test coverage tool | TCL2 | Can misreport coverage |
| Code generator (Simulink) | TCL3 | Generated code = safety-critical |
| Requirements tool / Git / editor | TCL1 | No qualification needed |

For ASIL D + TCL3: use pre-certified tool, validate yourself (method 1c), or diverse compilation.

## Project Rules

- ALL tools in the safety chain MUST be classified (TI/TD/TCL)
- TCL2/TCL3 tools MUST be qualified before safety-relevant use
- Tool versions pinned and documented
- Upgrades require re-evaluation
- Tool qualification records are part of the safety case
