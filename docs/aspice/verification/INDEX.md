---
document_id: VER-INDEX
title: "Verification Documentation Index"
version: "0.1"
status: draft
aspice_processes: "SWE.4, SWE.5, SWE.6, SYS.4, SYS.5"
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


# Verification Documentation Index

Right side of the V-model — all verification and validation evidence.

## Unit Verification (SWE.4)

| Document | Path | Status |
|----------|------|--------|
| Unit Test Plan | [unit-test/unit-test-plan.md](unit-test/unit-test-plan.md) | Planned |
| Unit Test Report | [unit-test/unit-test-report.md](unit-test/unit-test-report.md) | Planned |

## Integration Test (SWE.5)

| Document | Path | Status |
|----------|------|--------|
| Integration Strategy | [integration-test/integration-strategy.md](integration-test/integration-strategy.md) | Planned |
| Integration Test Report | [integration-test/integration-test-report.md](integration-test/integration-test-report.md) | Planned |

## SW Qualification (SWE.6)

| Document | Path | Status |
|----------|------|--------|
| SW Verification Plan | [sw-qualification/sw-verification-plan.md](sw-qualification/sw-verification-plan.md) | Planned |
| Release Notes | [sw-qualification/release-notes.md](sw-qualification/release-notes.md) | Planned |

## System Integration (SYS.4)

| Document | Path | Status |
|----------|------|--------|
| System Integration Report | [system-integration/system-integration-report.md](system-integration/system-integration-report.md) | Planned |

## System Verification (SYS.5)

| Document | Path | Status |
|----------|------|--------|
| System Verification Report | [system-verification/system-verification-report.md](system-verification/system-verification-report.md) | Planned |

## xIL Test Reports

| Document | Path | Level | Status |
|----------|------|-------|--------|
| MIL Report | [xil/mil-report.md](xil/mil-report.md) | Model-in-the-Loop | Planned |
| SIL Report | [xil/sil-report.md](xil/sil-report.md) | Software-in-the-Loop | Planned |
| PIL Report | [xil/pil-report.md](xil/pil-report.md) | Processor-in-the-Loop | Planned |
| HIL Report | [xil/hil-report.md](xil/hil-report.md) | Hardware-in-the-Loop | Planned |

