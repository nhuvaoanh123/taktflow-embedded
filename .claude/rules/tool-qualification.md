---
paths:
  - "scripts/**/*"
  - "firmware/**/*"
---

# Tool Qualification (ISO 26262 Part 8)

## Tool Classification Process

### Step 1: Tool Impact (TI)

| TI Level | Description |
|----------|-------------|
| TI1 | Tool cannot introduce or fail to detect errors in safety work products |
| TI2 | Tool CAN introduce errors OR fail to detect errors |

### Step 2: Tool Error Detection (TD)

| TD Level | Description |
|----------|-------------|
| TD1 | High confidence that tool errors will be detected |
| TD2 | Medium confidence |
| TD3 | Low or unknown confidence |

### Tool Confidence Level (TCL) Matrix

| | TD1 | TD2 | TD3 |
|---|---|---|---|
| **TI1** | TCL1 | TCL1 | TCL1 |
| **TI2** | TCL1 | TCL2 | **TCL3** |

- **TCL1**: No qualification required
- **TCL2**: Qualification required (medium rigor)
- **TCL3**: Qualification required (highest rigor)

## Qualification Methods

| Method | TCL2 ASIL A-B | TCL2 ASIL C-D | TCL3 ASIL A-B | TCL3 ASIL C-D |
|--------|---------------|---------------|---------------|---------------|
| (1a) Increased confidence from use | ++ | + | ++ | + |
| (1b) Evaluation of tool dev process | ++ | ++ | + | + |
| **(1c) Validation of the tool** | ++ | ++ | ++ | **++** |
| **(1d) Development per safety standard** | + | ++ | + | **++** |

For **ASIL D with TCL3 tools**: Methods (1c) or (1d) are highly recommended.

## Common Tool Classifications

| Tool | Typical TI | Typical TD | Typical TCL |
|------|-----------|-----------|-------------|
| **Compiler / code generator** | TI2 | TD2-TD3 | **TCL2-TCL3** |
| **Static analysis tool** | TI2 | TD2 | TCL2 |
| **Test coverage tool** | TI2 | TD2 | TCL2 |
| **MISRA checker** | TI2 | TD2 | TCL2 |
| **Model checker** | TI1 | — | TCL1 |
| Requirements management tool | TI1 | — | TCL1 |
| Text editor | TI1 | — | TCL1 |
| Version control (Git) | TI1 | — | TCL1 |
| Linker / debugger | TI2 | TD2 | TCL2 |
| Code generator (Simulink) | TI2 | TD3 | **TCL3** |

## Compiler Qualification at ASIL D

Compilers are typically TCL3 (TI2 + TD3): a miscompilation can silently introduce safety defects.

### Options:
1. **Pre-certified compiler** (e.g., HighTec with TUV Qualification Kit, Green Hills MULTI certified)
   - Ships with Safety Manual — user must follow it
   - Qualification evidence provided by vendor
2. **Qualify the compiler yourself**
   - Use vendor-provided validation test suites
   - Document test results and coverage
   - Follow method (1c): validate against specification
3. **Diverse compilation** (compensating measure)
   - Compile with two different compilers
   - Compare outputs for equivalence
   - Reduces dependence on single compiler correctness

## Tool Qualification Documentation

For each TCL2/TCL3 tool, maintain:

| Document | Content |
|----------|---------|
| Tool Classification Report | TI, TD, TCL determination with rationale |
| Tool Qualification Plan | Selected qualification method, scope, acceptance criteria |
| Tool Validation Report | Test results, coverage, pass/fail |
| Tool Use Restrictions | Known limitations, workarounds, configuration requirements |
| Tool Anomaly Log | Known bugs, impact on safety, compensating measures |

## Rules for This Project

- ALL tools used in the safety-critical development chain MUST be classified
- TCL2/TCL3 tools MUST be qualified before use in safety-relevant work
- Tool versions MUST be pinned and documented
- Tool upgrades require re-evaluation of qualification status
- Supplier tools must also be classified and qualified (flowed down via DIA)
- Tool qualification records are part of the safety case evidence
