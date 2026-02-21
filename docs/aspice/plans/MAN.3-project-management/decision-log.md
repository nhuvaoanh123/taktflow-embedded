---
document_id: MAN3-DECISION-LOG
title: "Decision Log"
version: "0.1"
status: active
updated: "2026-02-21"
---

# Decision Log

| Decision ID | Date | Decision | Rationale | Alternatives Considered | Impacted Docs |
|-------------|------|----------|-----------|-------------------------|---------------|
| D-001 | 2026-02-21 | Structure planning docs under ASPICE process areas | Improves auditability and ownership clarity | Keep flat plan files under `docs/plans/` | `docs/aspice/plans/README.md` |
| D-002 | 2026-02-21 | Keep `docs/plans/master-plan.md` as source baseline | Preserve single strategic source while operationalizing execution docs | Split and delete master plan | `docs/plans/master-plan.md` |
| D-003 | 2026-02-21 | Create central `docs/research/` repository | Ensure references are tracked and reusable across artifacts | Keep references scattered in notes | `docs/research/README.md`, `docs/research/link-log.md` |
| D-004 | 2026-02-21 | Add MAN.3 live tracking set (dashboard/logs/gates) | Close progress visibility gap and enable gate-based management | Use only checklist checkboxes | `docs/aspice/plans/MAN.3-project-management/*` |
