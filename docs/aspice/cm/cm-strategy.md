---
document_id: CMS
title: "Configuration Management Strategy"
version: "0.1"
status: draft
aspice_process: SUP.8
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


# Configuration Management Strategy

## Purpose

Define CM strategy per ASPICE SUP.8: identification, control, status accounting, and auditing of configuration items.

## CM Tool

Git + GitHub — all configuration items are version-controlled.

## Branching Strategy

Git Flow:
- `main` — protected, tagged releases only (baselines)
- `develop` — integration branch
- `feature/` — one per feature/phase
- `release/` — release candidates
- `hotfix/` — emergency fixes

## Configuration Items

| Category | Items | Naming |
|----------|-------|--------|
| Source code | firmware/**/*.c, *.h | Per AUTOSAR module naming |
| Safety docs | docs/safety/**/*.md | Document ID in frontmatter |
| ASPICE docs | docs/aspice/**/*.md | Document ID in frontmatter |
| Build scripts | scripts/*, Makefile | Descriptive names |
| Test artifacts | test/**, firmware/*/test/ | TC-{ECU}-{MOD}-NNN |

## Baseline Strategy

Baselines are Git tags on `main`:

| Baseline | Tag | Content |
|----------|-----|---------|
| BL-001 | v0.1.0 | Phase 0 complete — architecture docs |
| BL-002 | v0.2.0 | Phase 1-3 complete — safety + requirements |
| BL-003 | v0.5.0 | Phase 5 complete — BSW baseline |
| BL-004 | v1.0.0 | Phase 14 complete — release |

## Change Control

- All changes via pull request to `develop`
- Safety-relevant changes require independent review
- Baseline changes require formal change request (see change-requests/)

## Naming Conventions

| Item | Convention | Example |
|------|-----------|---------|
| Requirements | {PREFIX}-{ECU}-{NNN} | SSR-CVC-001 |
| Test cases | TC-{ECU}-{MOD}-{NNN} | TC-CVC-PEDAL-001 |
| Change requests | CR-{NNN} | CR-001 |
| Baselines | BL-{NNN} | BL-001 |
| Documents | Document ID in YAML | HARA, FSC, SYSARCH |

## Status Accounting

Document status lifecycle: `planned` → `draft` → `review` → `approved` → `baselined`

## CM Audits

| Audit | Trigger | Scope |
|-------|---------|-------|
| Functional CM audit | Before baseline | All CIs match approved versions |
| Physical CM audit | Before release | Build reproduces from tagged source |

