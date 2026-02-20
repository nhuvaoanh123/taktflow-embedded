---
paths:
  - "firmware/**/*"
  - "firmware/test/**/*"
---

# ASIL D Verification & Validation (ISO 26262 Parts 4, 5, 6, 8)

## Verification Methods by ASIL Level

Legend: ++ = highly recommended, + = recommended, o = no recommendation

### Software Safety Requirements Verification

| Method | ASIL A | ASIL B | ASIL C | ASIL D |
|--------|--------|--------|--------|--------|
| Walk-through | ++ | + | + | + |
| Inspection | + | ++ | ++ | **++** |
| Semi-formal verification | o | + | ++ | **++** |
| **Formal verification** | o | o | + | **++** |
| Prototyping / simulation | o | + | + | **++** |

### Software Architecture Verification

| Method | ASIL A | ASIL B | ASIL C | ASIL D |
|--------|--------|--------|--------|--------|
| Walk-through | ++ | + | o | o |
| Inspection | + | ++ | ++ | **++** |
| Semi-formal verification | o | + | ++ | **++** |
| **Formal verification** | o | o | + | **++** |
| Simulation | o | + | ++ | **++** |

### Software Unit Design Verification

| Method | ASIL A | ASIL B | ASIL C | ASIL D |
|--------|--------|--------|--------|--------|
| Walk-through | ++ | + | o | o |
| Inspection | + | ++ | ++ | **++** |
| Semi-formal verification | o | + | + | **++** |
| **Formal verification** | o | o | + | **++** |
| Static code analysis | + | + | ++ | **++** |

**ASIL D is the ONLY level where formal verification is highly recommended across all verification activities.**

## Unit Testing at ASIL D (Part 6, Tables 7-9)

### Test Methods (Table 7)

| Method | ASIL D |
|--------|--------|
| Requirements-based testing | ++ |
| Interface testing | ++ |
| **Fault injection testing** | **++** |
| **Resource usage testing** | **++** |
| **Back-to-back testing** | **++** |

### Test Case Derivation (Table 8)

| Method | ASIL D |
|--------|--------|
| Analysis of requirements | ++ |
| Equivalence classes | ++ |
| Boundary value analysis | ++ |
| **Error guessing / fault seeding** | **++** |

### Structural Coverage (Table 9)

| Metric | ASIL D |
|--------|--------|
| Statement coverage | ++ |
| Branch coverage | ++ |
| **MC/DC coverage** | **++** |

**MC/DC target: 100%**. Any shortfall requires formal documented justification.

## Integration Testing at ASIL D

- Test cases derived from software architectural design
- Fault injection at integration level is highly recommended
- Resource usage testing on target hardware is highly recommended
- Back-to-back comparison (model vs implementation) is highly recommended
- All integration interfaces must be tested for correct and incorrect data

## What Makes ASIL D Unique (vs lower ASILs)

1. **Formal verification** is highly recommended (++ at D, + at C, o at A/B)
2. **MC/DC coverage** is highly recommended (++ at D and C, + at B, o at A)
3. **Fault injection testing** elevated to ++ at unit and integration
4. **Resource usage testing** on target hardware is ++
5. **Back-to-back testing** is ++
6. **Error guessing** in test derivation is ++
7. **Spatial isolation** of software components is ++
8. **I3 independence** required for assessments (external body)

## Independence Requirements for Confirmation Measures

| Level | Meaning |
|-------|---------|
| I0 | Should be performed; different person than creator |
| I1 | Shall be performed; different person |
| I2 | Shall be performed; independent from team |
| **I3** | **Shall be performed; independent from department (external org)** |

### Independence by Activity at ASIL D

| Activity | ASIL D Requirement |
|----------|-------------------|
| Confirmation review of safety plan | **I3** (external org) |
| Confirmation review of safety case | **I3** (external org) |
| Confirmation review of HARA | I2 (independent from team) |
| Functional safety audit | I2 (independent from team) |
| **Functional safety assessment** | **I3 (external org — TUV, SGS, exida)** |

**Budget for external assessment from project start** — not as an afterthought.

## Fault Injection Testing Requirements

For ASIL D, fault injection must cover:

### Software Fault Injection
- Corrupt input parameters (out of range, null, max, min, boundary)
- Corrupt return values from dependencies
- Simulate resource exhaustion (stack overflow, queue full)
- Inject timing faults (delayed responses, missed deadlines)
- Corrupt memory content (bit flips in safety-critical variables)

### Hardware Fault Injection
- Power supply transients (brownout, overvoltage)
- Clock frequency perturbation
- Communication bus errors (bit errors, bus-off)
- Sensor fault simulation (stuck, out-of-range, drift)
- Memory corruption (single-bit, multi-bit errors)

### Expected Behavior
- System must detect the fault within FDTI
- System must reach safe state within FTTI
- No undetected safety goal violation
- Fault detection and reaction logged

## Safety Validation (Part 4)

- Demonstrates safety goals are met at vehicle level
- Vehicle-level testing in intended use environment
- Methods: HIL simulation, track testing, field data analysis
- Safety validation plan required — reviewed by independent assessor
- Results documented in safety validation report
