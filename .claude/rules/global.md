---
paths:
  - "**/*"
---
# HITL Comment Lock

Never edit/reformat/move/delete text inside `HITL-LOCK` blocks. Append-only outside locks.
Markers: `HITL-LOCK START:<id>` / `HITL-LOCK END:<id>` (use comment syntax for file type: `<!-- -->`, `//`, `#`)

# Documentation

**Public functions**: `@brief`, `@param` (all params + constraints), `@return` (all values), `@note` (thread safety, ISR context).
**File headers**: `@file`, `@brief`, `@author`, `@date`.
**Comments**: explain WHY not WHAT. No commented-out code. No self-evident comments.
**Claim accuracy**: audit public-facing claims against code. Use qualifiers ("by default", "up to"). Release gate.

# Workflow

- Plan first → `docs/plans/plan-<desc>.md` → get approval → then code
- Update plan before next phase. Numbered phases: PENDING → IN PROGRESS → DONE.
- TODOs: `TODO:SCALE`, `TODO:POST-BETA`, `TODO:HARDWARE`, `TODO:SECURITY`, `TODO:ISO`, `TODO:TEST`

## Git Flow
`main` (protected, tagged vX.Y.Z) ← `release/` ← `develop` ← `feature/`, `bugfix/`. Hotfixes from `main` → both.
Branches: `feature/<plan>-<phase>`, `bugfix/<desc>`, `release/<ver>`, `hotfix/<desc>`.
Commits: imperative, <50 chars, include WHY, one change per commit.

## PR Checklist
Plan approved, TODOs intentional, tests pass, no secrets, input validation, fail-closed, docs updated.

## BOM
`hardware/bom.md` = ONLY file for procurement/delivery status.

# Decision Records

Decisions need: tier (T1-T4), rationale, effort, 2 alternatives, why chosen wins. Record in `docs/aspice/plans/MAN.3-project-management/decision-log.md` as `ADR-NNN`.
T1=architecture(locked), T2=design(weeks), T3=implementation(days), T4=process(hours). Score: Cost+Time+Safety+Resume (1-3 each).
Skip ADR for: regulation mandates, trivial choices, temp debug.
