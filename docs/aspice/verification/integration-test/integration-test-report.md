---
document_id: ITR
title: "Integration Test Report"
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


# Integration Test Report

<!-- Phase 12-13 deliverable -->

## Summary

| Metric | Value |
|--------|-------|
| Integration tests | — |
| Passed | — |
| Failed | — |
| Blocking issues | — |

## Results

<!-- To be completed in Phase 12-13 -->

