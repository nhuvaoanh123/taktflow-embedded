---
document_id: ITS
title: "Integration Test Strategy"
version: "0.1"
status: planned
aspice_process: SWE.5
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


# Integration Test Strategy

<!-- Phase 12 deliverable -->

## Purpose

Define the integration order, test approach, and pass/fail criteria per ASPICE SWE.5.

## Integration Order

1. MCAL → CanIf → PduR → Com (data path)
2. Com → Rte → SWC (signal path)
3. Dcm → PduR → CanIf (diagnostic path)
4. WdgM → BswM (supervision path)
5. ECU-to-ECU via CAN bus

## Integration Levels

| Level | Scope | Method |
|-------|-------|--------|
| Intra-ECU | BSW + SWC on single ECU | SIL |
| Inter-ECU | Multiple ECUs on CAN bus | SIL → PIL → HIL |
| System | All 7 ECUs + gateway | HIL |

