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

# Project Reading Checklist

Use this as your clickable review tracker.

## Rank 1 - Critical
- [x] [Stakeholder requirements](./aspice/system/stakeholder-requirements.md) — Reviewed: 2026-02-25
- [x] [System requirements](./aspice/system/system-requirements.md) — Reviewed: 2026-02-26 (56 HITL comments, ASIL summary fixed)
- [x] [System architecture](./aspice/system/system-architecture.md) — Reviewed: 2026-02-26 (24 HITL comments)
- [x] [Interface control doc](./aspice/system/interface-control-doc.md) — Reviewed: 2026-02-26 (18 HITL comments)
- [x] [CAN message matrix](./aspice/system/can-message-matrix.md) — Reviewed: 2026-02-26 (17 HITL comments)

## Rank 2 — Safety Chain (read in order)
- [ ] 1. [Item definition](./safety/concept/item-definition.md)
- [ ] 2. [HARA](./safety/concept/hara.md)
- [ ] 3. [Safety goals](./safety/concept/safety-goals.md)
- [ ] 4. [Functional safety concept](./safety/concept/functional-safety-concept.md)
- [ ] 5. [Functional safety requirements](./safety/requirements/functional-safety-reqs.md)
- [ ] 6. [Technical safety requirements](./safety/requirements/technical-safety-reqs.md)
- [ ] 7. [Software safety requirements](./safety/requirements/sw-safety-reqs.md)
- [ ] 8. [Hardware safety requirements](./safety/requirements/hw-safety-reqs.md)
- [ ] 9. [HSI specification](./safety/requirements/hsi-specification.md)
- [ ] 10. [Traceability matrix](./aspice/traceability/traceability-matrix.md)

## Rank 3 — Architecture + Per-ECU SWR
- [ ] 11. [Software architecture](./aspice/software/sw-architecture/sw-architecture.md)
- [ ] 12. [BSW architecture](./aspice/software/sw-architecture/bsw-architecture.md)
- [ ] 13. [vECU architecture](./aspice/software/sw-architecture/vecu-architecture.md)
- [ ] 14. [SWR - CVC](./aspice/software/sw-requirements/SWR-CVC.md)
- [ ] 15. [SWR - FZC](./aspice/software/sw-requirements/SWR-FZC.md)
- [ ] 16. [SWR - RZC](./aspice/software/sw-requirements/SWR-RZC.md)
- [ ] 17. [SWR - SC](./aspice/software/sw-requirements/SWR-SC.md)
- [ ] 18. [SWR - BCM](./aspice/software/sw-requirements/SWR-BCM.md)
- [ ] 19. [SWR - ICU](./aspice/software/sw-requirements/SWR-ICU.md)
- [ ] 20. [SWR - TCU](./aspice/software/sw-requirements/SWR-TCU.md)
- [ ] 21. [SWR - BSW](./aspice/software/sw-requirements/SWR-BSW.md)
- [ ] 22. [Hardware overview](../hardware/README.md)
- [ ] 23. [Pin mapping](../hardware/pin-mapping.md)
- [ ] 24. [BOM](../hardware/bom.md)
- [ ] 25. [Gateway overview](../gateway/README.md)

## Rank 4 — Reference (skim)
- [ ] 26. [ASPICE plans index](./aspice/plans/README.md)
- [ ] 27. [Verification index](./aspice/verification/INDEX.md)
- [ ] 28. [QA plan](./aspice/quality/qa-plan.md)
- [ ] 29. [CM strategy](./aspice/cm/cm-strategy.md)
- [ ] 30. [Process playbook](./reference/process-playbook.md)

