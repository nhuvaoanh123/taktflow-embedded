---
paths:
  - "firmware/**/*"
  - "hardware/**/*"
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


# ASIL Decomposition (ISO 26262 Part 9)

## What Is ASIL Decomposition

ASIL decomposition allows splitting a safety requirement into redundant sub-requirements at lower ASIL levels, allocated to sufficiently independent elements. This can reduce development process rigor (systematic requirements) but does NOT reduce hardware metrics requirements.

## Decomposition Table

| Original ASIL | Option 1 | Option 2 | Option 3 |
|---------------|----------|----------|----------|
| **ASIL D** | **D(D) + QM(D)** | **C(D) + A(D)** | **B(D) + B(D)** |
| ASIL C | C(C) + QM(C) | B(C) + A(C) | — |
| ASIL B | B(B) + QM(B) | A(B) + A(B) | — |
| ASIL A | A(A) + QM(A) | — | — |

Notation: `B(D)` = developed to ASIL B processes, original requirement was ASIL D.

## Critical Constraints

### 1. Hardware Metrics Remain at Original ASIL
If ASIL D is decomposed into B(D) + B(D), **both elements must still meet ASIL D hardware metrics**:
- SPFM >= 99%
- LFM >= 90%
- PMHF < 10 FIT

Decomposition only relaxes **systematic** (process) requirements, NOT random hardware failure targets.

### 2. Integration Testing at Original ASIL
Integration and verification of the combined decomposed elements must follow the **original ASIL's methods**. No effort savings on integration testing.

### 3. Independence Must Be Demonstrated
The decomposed elements must be **sufficiently independent**. Requires formal Dependent Failure Analysis (DFA) proving:
- No common cause failures between elements
- Freedom from interference (spatial, temporal, communication)
- Physical separation or other measures against common cause initiators

### 4. D(D) + QM(D) Is Dangerous
Decomposing with one QM element means that element provides **zero safety integrity**. Only valid if the D(D) element alone can fully achieve the safety goal.

**Invalid example**: ASIL D for ECU decomposed as QM(D) for ECU + D(D) for watchdog — the watchdog alone cannot cover all MCU failure modes.

### 5. When Decomposition Is NOT Allowed
- Sufficient independence cannot be guaranteed
- Elements share common cause failure modes that cannot be mitigated
- Single element cannot be isolated from interference by the other
- Cost/complexity of proving independence exceeds the benefit

### 6. Cascaded Decomposition
Decomposed requirements can be further decomposed, but:
- Each level requires its own independence argument
- Original ASIL is always preserved in notation
- Complexity of independence proof grows rapidly — use sparingly

## Decision Framework

```
Can the safety goal be met by a single element?
├── YES → No decomposition needed
└── NO → Are two independent implementations feasible?
    ├── YES → Can you prove independence (DFA, FFI)?
    │   ├── YES → Decompose. Select ASIL pair from table.
    │   └── NO → Cannot decompose. Develop everything at ASIL D.
    └── NO → Cannot decompose. Develop everything at ASIL D.
```

## Documentation Required for Decomposition

| Document | Content |
|----------|---------|
| Decomposition rationale | Why decomposition is needed and appropriate |
| ASIL allocation | Which element gets which ASIL level |
| Independence argument | How spatial, temporal, communication independence is achieved |
| DFA report | Common cause failure and cascading failure analysis |
| FFI evidence | Test results, analysis, and architecture proof |
| Integration test plan | Testing at the original ASIL level |

## Rules for This Project

- Decomposition decisions must be documented in the safety plan
- Every decomposition must include a formal DFA
- FFI must be demonstrated by analysis AND testing
- Hardware metrics always at the original (highest) ASIL level
- Integration verification always at the original ASIL level
- Decomposition is a safety architecture decision — requires FSE approval

