---
paths:
  - "**/*"
---

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

## Commit Discipline

- Commit messages: imperative mood, first line under 50 chars
- Include why the change was needed, not just what changed
- Reference issue numbers when applicable
- One logical change per commit — don't bundle unrelated changes

## Code Review Checklist (before any PR)

- [ ] Plan exists and is approved
- [ ] All TODO markers are intentional (not forgotten work)
- [ ] Tests pass
- [ ] No hardcoded credentials or secrets
- [ ] Input validation on all external boundaries
- [ ] Error handling is fail-closed
- [ ] Documentation updated if public API changed
