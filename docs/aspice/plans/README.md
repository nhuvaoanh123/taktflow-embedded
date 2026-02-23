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

# ASPICE Planning Structure

Source baseline: `docs/plans/master-plan.md`

This folder contains execution-ready plans organized by Automotive SPICE process areas.

## Folder Map

- `docs/aspice/plans/MAN.3-project-management/`
- `docs/aspice/plans/SYS.1-system-requirements/`
- `docs/aspice/plans/SYS.2-system-architecture/`
- `docs/aspice/plans/SWE.1-2-requirements-and-architecture/`
- `docs/aspice/plans/SWE.3-implementation/`
- `docs/aspice/plans/SWE.4-6-verification-and-release/`

## MAN.3 Tracking Pack

- `docs/aspice/plans/MAN.3-project-management/progress-dashboard.md`
- `docs/aspice/plans/MAN.3-project-management/weekly-status-template.md`
- `docs/aspice/plans/MAN.3-project-management/weekly-status-2026-W08.md`
- `docs/aspice/plans/MAN.3-project-management/daily-progress-template.md`
- `docs/aspice/plans/MAN.3-project-management/daily-log/`
- `docs/aspice/plans/MAN.3-project-management/risk-register.md`
- `docs/aspice/plans/MAN.3-project-management/issue-log.md`
- `docs/aspice/plans/MAN.3-project-management/decision-log.md`
- `docs/aspice/plans/MAN.3-project-management/gate-readiness-checklist.md`

## How to Use

1. Start each week from `MAN.3-project-management/execution-roadmap.md`.
2. Execute tasks from SYS -> SWE order unless blocked.
3. For each completed task, attach evidence in the referenced work product file.
4. Do not close a gate until all gate checklist items are checked.

## Process Coverage

- MAN.3: planning, milestones, risks, status control
- SYS.1: item definition, HARA, safety goals, TSR baseline
- SYS.2: architecture, interfaces, CAN/E2E/HSI baseline
- SWE.1/SWE.2: software requirements and architecture traceability
- SWE.3: BSW + ECU + vECU implementation
- SWE.4/SWE.5/SWE.6: unit/integration/system verification, HIL, release evidence

