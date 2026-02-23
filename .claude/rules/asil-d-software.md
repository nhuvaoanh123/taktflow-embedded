---
paths:
  - "firmware/**/*.c"
  - "firmware/**/*.h"
  - "firmware/**/*.cpp"
  - "firmware/**/*.hpp"
---

## Human-in-the-Loop (HITL) Comment Lock

`HITL` means human-reviewer-owned comment content.

**Marker standard (code-friendly):**
- Markdown: `<!-- HITL-LOCK START:<id> -->` ... `<!-- HITL-LOCK END:<id> -->`
- C/C++/Java/JS/TS: `// HITL-LOCK START:<id>` ... `// HITL-LOCK END:<id>`
- Python/Shell/YAML/TOML: `# HITL-LOCK START:<id>` ... `# HITL-LOCK END:<id>`

**Rules:**
- AI must never edit, reformat, move, or delete text inside any `HITL-LOCK` block.
- Append-only: AI may add new comments/changes only; prior HITL comments stay unchanged.
- If a locked comment needs revision, add a new note outside the lock or ask the human reviewer to unlock it.


# ASIL D Software Development Constraints (ISO 26262 Part 6)

## Software Architectural Design (Part 6, Section 7)

At ASIL D, the following are **highly recommended (++)** (effectively mandatory):

| Principle | ASIL D |
|-----------|--------|
| Hierarchical structure of software components | ++ |
| Restricted size of software components | ++ |
| Restricted size of interfaces | ++ |
| High cohesion within each software component | ++ |
| Restricted coupling between software components | ++ |
| **Appropriate spatial isolation (MPU/MMU)** | **++** |
| **Appropriate management of shared resources** | **++** |

Spatial isolation and shared resource management are the key differentiators vs lower ASILs.

## Software Unit Design (Part 6, Section 8)

| Principle | ASIL D |
|-----------|--------|
| Enforcement of low complexity | ++ |
| Use of language subsets (MISRA C) | ++ |
| Enforcement of strong typing | ++ |
| **Use of defensive implementation techniques** | **++** |
| **Use of established design principles** | **++** |
| **Use of unambiguous graphical representation** | **++** |
| Use of style guides | ++ |
| Use of naming conventions | ++ |

## Defensive Programming (MANDATORY at ASIL D)

All of the following are **highly recommended (++)** at ASIL D:

### Range Checks
- Validate ALL input parameters against defined min/max bounds
- Validate ALL sensor readings against physical plausibility
- Validate ALL array indices before access
- Validate ALL pointer values before dereference

### Plausibility Checks
- Cross-check sensor readings against physical models
- Cross-check redundant data sources
- Verify output values are within expected physical range
- Detect stuck-at or out-of-range sensor values

### Program Flow Monitoring
- **Logical supervision**: Verify correct order of checkpoint execution
- **Deadline supervision**: Verify execution completes within time bounds
- **Alive supervision**: Watchdog counts checkpoint arrivals per period
- Any deviation from expected sequence = transition to safe state

### Data Flow Monitoring (AUTOSAR E2E)
- CRC protection on all safety-critical data transfers
- Sequence counters to detect message loss, repetition, delay
- Data IDs to detect message insertion and masquerade
- Timeout monitoring for periodic messages

### Redundant Storage of Critical Variables
- Store safety-critical variables in duplicate (original + complement/inverse)
- Verify consistency before every use
- Detects RAM bit-flips and transient memory corruption

### Memory Protection (MPU)
- **SHALL be supported by dedicated hardware features** at ASIL D
- Each software partition in a separate memory region
- MPU enforces spatial isolation between ASIL levels (freedom from interference)
- Access violations trigger fault handler -> safe state

### Stack Monitoring
- Static stack usage analysis for every call chain (worst-case)
- Runtime stack watermarking or hardware stack overflow detection
- Stack overflow = transition to safe state

### Watchdog
- External window watchdog (independent of monitored processor)
- Service window: too early = reset, too late = reset
- Feed ONLY from main loop — proves system is alive and responsive
- Watchdog must be independent of the CPU being monitored

### Diverse Redundancy
- Implement critical safety functions using different algorithms
- Optionally: different compilers, different hardware
- Cross-check results — mismatch = fault detected
- Applicable: N-version programming, Triple Modular Redundancy (TMR)

## Memory Constraints

- **NO dynamic memory allocation (malloc/calloc/realloc) in production**
- Use only static allocation or pre-allocated fixed-size memory pools
- Runtime allocations cannot be proven deterministic for WCET
- All memory usage must be statically analyzable

## Timing Constraints

- **Worst-Case Execution Time (WCET) analysis is MANDATORY**
- Use static analysis tools (AbsInt aiT, Rapita RapiTime) or measurement-based approaches
- All safety functions must complete within their time budget
- WCET + jitter must not exceed the task period or FTTI

## Code Coverage Requirements (Part 6, Table 9)

| Coverage Metric | ASIL D Requirement |
|-----------------|-------------------|
| Statement coverage | ++ (highly recommended) |
| Branch coverage (decision) | ++ (highly recommended) |
| **MC/DC coverage** | **++ (highly recommended)** |

**MC/DC (Modified Condition/Decision Coverage)**: Each condition in a decision must independently affect the decision outcome. Target: **100%**. Any shortfall requires formal justification.

## Static Analysis Requirements (Part 6)

| Method | ASIL D |
|--------|--------|
| Static code analysis (MISRA compliance) | ++ |
| Control flow analysis | ++ |
| Data flow analysis | ++ |
| Complexity metrics (cyclomatic complexity) | ++ |
| Stack usage analysis | ++ |

## Dynamic Testing Requirements (Part 6)

### Unit Testing Methods (Table 7)

| Method | ASIL D |
|--------|--------|
| Requirements-based testing | ++ |
| Interface testing | ++ |
| **Fault injection testing** | **++** |
| **Resource usage testing** | **++** |
| **Back-to-back testing (model vs code)** | **++** |

### Test Case Derivation (Table 8)

| Method | ASIL D |
|--------|--------|
| Analysis of requirements | ++ |
| Equivalence classes | ++ |
| Boundary value analysis | ++ |
| **Error guessing / fault seeding** | **++** |

## Prohibited Practices at ASIL D

- No dynamic memory allocation (malloc, calloc, realloc, free)
- No recursion without proven stack bound
- No self-modifying code
- No pointer arithmetic without bounds proof
- No goto (except for single-exit-point error handling patterns)
- No implicit type conversions between signed/unsigned
- No floating-point in safety-critical calculations without analysis
- No compiler-specific extensions without qualification
- No inline assembly without formal justification
- No global variables without protection mechanism (mutex, critical section)

