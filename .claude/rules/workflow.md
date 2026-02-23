---
paths:
  - "**/*"
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


# Workflow & Planning Discipline

## Planning Rules

- ALWAYS write a plan to `docs/plans/` before implementing any feature or change
- ALWAYS get user approval on the plan before writing code
- ALWAYS update the plan doc to reflect current state before starting next phase
- Never start coding without an approved plan — no exceptions
- If requirements are unclear, ask questions first, then plan

## Phase-Based Execution

- Break work into numbered phases with clear DONE criteria
- Maintain a master plan with a status table: Phase | Name | Status
- Each phase transitions: `PENDING` -> `IN PROGRESS` -> `DONE`
- Mark individual items with `[x]` checkboxes as they complete
- Do not skip phases — complete in order unless explicitly approved otherwise

## TODO Markers

Use greppable markers for deferred work:

```
// TODO:SCALE      — change needed for production scale
// TODO:POST-BETA  — enhancement for after launch
// TODO:HARDWARE   — requires hardware validation
// TODO:SECURITY   — security improvement needed
// TODO:ISO        — ISO compliance requirement pending
// TODO:TEST       — test coverage needed
```

## Git Flow Branching Strategy

```
main (protected)          ← Production releases only. Tagged vX.Y.Z.
  └── develop             ← Integration branch. All features merge here first.
       ├── feature/xxx    ← One branch per feature or plan phase
       ├── bugfix/xxx     ← Bug fixes targeting develop
       └── release/x.y    ← Release candidate. Freeze, test, sign firmware.
            └── hotfix/xxx ← Emergency fix on main, backported to develop
```

### Branch Naming Convention

| Type | Pattern | Example |
|------|---------|---------|
| Feature | `feature/<plan-name>-<phase>` | `feature/mqtt-client-phase1` |
| Bugfix | `bugfix/<issue-or-description>` | `bugfix/uart-overflow-fix` |
| Release | `release/<version>` | `release/1.0.0` |
| Hotfix | `hotfix/<description>` | `hotfix/ota-signature-bypass` |

### Branch Rules

- **main**: Protected. Only merge from `release/` or `hotfix/`. Every merge tagged with semver. This is the baseline for ASPICE SUP.8 and ISO 26262 configuration management.
- **develop**: Integration branch. All `feature/` and `bugfix/` branches merge here via PR. CI runs on every push.
- **feature/**: Created from `develop`. One branch per plan or plan phase. Merged back to `develop` via PR after review. Delete after merge.
- **release/**: Created from `develop` when ready for release. Only bug fixes allowed. Merged to both `main` (tagged) and `develop`. Delete after merge.
- **hotfix/**: Created from `main` for emergency production fixes. Merged to both `main` (tagged) and `develop`. Delete after merge.

### Workflow

1. Create feature branch: `git checkout develop && git checkout -b feature/<name>`
2. Develop on feature branch (commit often, push regularly)
3. Open PR to `develop` — triggers CI, code review, plan checklist
4. Merge to `develop` after approval
5. When ready for release: `git checkout develop && git checkout -b release/x.y.z`
6. Stabilize on release branch (only fixes, no new features)
7. Merge release to `main`, tag, merge back to `develop`

### Tagging (Baselines for ISO 26262 / ASPICE)

- Every merge to `main` gets a semver tag: `vMAJOR.MINOR.PATCH`
- Release tags include firmware version and build metadata: `v1.0.0+build.42`
- Tags are immutable baselines — never delete or move a tag
- Tag message includes: version, date, release notes summary, signed-by

## Commit Discipline

- Commit messages: imperative mood, first line under 50 chars
- Include why the change was needed, not just what changed
- Reference issue numbers when applicable
- One logical change per commit — don't bundle unrelated changes
- On feature branches: commit freely (can squash on merge)
- On develop/main: clean, atomic commits only

## Code Review Checklist (before any PR)

- [ ] Plan exists and is approved
- [ ] All TODO markers are intentional (not forgotten work)
- [ ] Tests pass
- [ ] No hardcoded credentials or secrets
- [ ] Input validation on all external boundaries
- [ ] Error handling is fail-closed
- [ ] Documentation updated if public API changed

