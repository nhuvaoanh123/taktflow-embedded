---
paths:
  - "firmware/**/*"
  - "hardware/**/*"
---

# ASIL D Constraints (ISO 26262 Parts 4, 5, 6, 8, 9)

Full reference tables: `docs/reference/asil-d-reference.md`

## Software Constraints (Part 6)

### Mandatory Defensive Programming
- Validate ALL input parameters (min/max bounds)
- Validate ALL sensor readings (plausibility, range, rate-of-change)
- Validate ALL array indices and pointers before access
- Cross-check redundant data sources
- Program flow monitoring: logical, deadline, and alive supervision
- Data flow monitoring: CRC, sequence counters, data IDs, timeouts (AUTOSAR E2E)
- Redundant storage of safety-critical variables (original + complement)
- Stack monitoring: static WCET analysis + runtime watermarking
- External window watchdog, fed ONLY from main loop

### Memory Rules
- NO dynamic memory allocation (`malloc`/`calloc`/`realloc`/`free`) in production
- MPU/MMU hardware spatial isolation between ASIL levels (mandatory at ASIL D)
- Static allocation or pre-allocated fixed-size pools only
- Stack overflow = transition to safe state

### Coverage Requirements
- Statement coverage: 100%
- Branch coverage: 100%
- MC/DC coverage: 100% (shortfall requires formal justification)

### Static Analysis (all highly recommended at ASIL D)
- MISRA compliance, control flow, data flow, complexity metrics, stack usage

### Prohibited Practices
- No dynamic memory allocation
- No recursion without proven stack bound
- No self-modifying code
- No pointer arithmetic without bounds proof
- No goto (except single-exit-point error handling)
- No implicit signed/unsigned conversions
- No floating-point in safety-critical calculations without analysis
- No compiler-specific extensions without qualification
- No inline assembly without formal justification
- No unprotected global variables

## Hardware Constraints (Part 5)

### Architectural Metrics
- **SPFM >= 99%** — max 1% undetected single-point faults
- **LFM >= 90%** — max 10% latent multi-point faults
- **PMHF < 10 FIT** — requires redundancy; individual components rarely achieve alone
- **Diagnostic coverage >= 99%** (High DC)

### Required Safety Mechanisms
- Lockstep CPU or independent monitoring processor
- ECC on all safety-critical RAM and flash
- Memory BIST at startup + periodic runtime checks
- CRC on all bus communications + alive counters + timeouts
- Independent voltage, clock, and power supply monitoring
- ADC self-test + sensor plausibility checking

### Redundancy
- **1oo2D** = standard ASIL D architecture (two channels + diagnostics)
- **2oo3** = fail-operational (steer/brake-by-wire)
- Common cause failure analysis required (Annex F checklist)

## Architecture Constraints (Part 4)

### Freedom from Interference (FFI)
Three types: spatial (memory), temporal (timing), communication.
- Hardware separation (strongest) or MPU/MMU partitioning
- Safety-qualified RTOS with time-partitioned scheduling
- Higher-ASIL element validates ALL data from lower-ASIL
- If full separation not achieved: lower-ASIL element developed at higher ASIL

### Timing
- FDTI + FRTI < FTTI (from HARA)
- WCET analysis mandatory for every safety-critical task
- Jitter bounded, interrupt latency analyzed, deterministic scheduling

### Communication Safety (E2E)
Apply to ALL safety-critical data transfers (inter-ECU, intra-ECU, sensors, actuators):
- CRC (8-bit CAN 2.0, 32/64-bit CAN FD/Ethernet)
- Sequence counter (4-bit CAN, 8-bit CAN FD)
- Data ID per data element
- Timeout monitoring

### Safe States
- Every safety goal has at least one defined safe state
- Precisely defined, reachable within FTTI, independent of failed component
- Verified via fault injection

### Mixed-Criticality
- All software at highest ASIL unless FFI demonstrated
- Safety-qualified hypervisor/RTOS required for partitioning
- Partition interfaces are safety-critical elements

## Verification Constraints (Parts 4, 6, 8)

### ASIL D Unique Requirements
- **Formal verification** highly recommended (++ at D only)
- **Fault injection testing** at unit and integration levels
- **Resource usage testing** on target hardware
- **Back-to-back testing** (model vs code)
- **Error guessing / fault seeding** in test derivation
- **I3 independence** for safety assessment (external org: TUV, SGS, exida)

### Fault Injection Must Cover
- Software: corrupt inputs, return values, resource exhaustion, timing faults, memory corruption
- Hardware: power transients, clock perturbation, bus errors, sensor faults, memory errors
- Expected: detect within FDTI, reach safe state within FTTI, no undetected safety goal violation

## ASIL Decomposition (Part 9)

### Decomposition Options for ASIL D
| Option 1 | Option 2 | Option 3 |
|----------|----------|----------|
| D(D) + QM(D) | C(D) + A(D) | B(D) + B(D) |

### Critical Constraints
- Hardware metrics remain at original ASIL (SPFM >= 99%, LFM >= 90%, PMHF < 10 FIT)
- Integration testing at original ASIL level
- Independence must be demonstrated via DFA (common cause + cascading failure analysis)
- D(D) + QM(D) is dangerous: QM element provides zero safety integrity
- Decomposition requires FSE approval and formal documentation

### Decision Framework
Can safety goal be met by single element? If no: can two independent implementations be proven independent (DFA, FFI)? If no: develop everything at ASIL D.

### Required Documentation
Decomposition rationale, ASIL allocation, independence argument, DFA report, FFI evidence, integration test plan at original ASIL.
