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
- [ ] [System requirements](./aspice/system/system-requirements.md) — In progress: ASIL summary fixed, SYS-028 HITL done, detail review by section pending
- [ ] [System architecture](./aspice/system/system-architecture.md)
- [ ] [Interface control doc](./aspice/system/interface-control-doc.md)
- [ ] [CAN message matrix](./aspice/system/can-message-matrix.md)

## Rank 2 - High
- [ ] [Safety index](./safety/INDEX.md)
- [ ] [Item definition](./safety/concept/item-definition.md)
- [ ] [HARA](./safety/concept/hara.md)
- [ ] [Safety goals](./safety/concept/safety-goals.md)
- [ ] [Functional safety concept](./safety/concept/functional-safety-concept.md)
- [ ] [Functional safety requirements](./safety/requirements/functional-safety-reqs.md)
- [ ] [Technical safety requirements](./safety/requirements/technical-safety-reqs.md)
- [ ] [Software safety requirements](./safety/requirements/sw-safety-reqs.md)
- [ ] [Hardware safety requirements](./safety/requirements/hw-safety-reqs.md)
- [ ] [HSI specification](./safety/requirements/hsi-specification.md)
- [ ] [Traceability matrix](./aspice/traceability/traceability-matrix.md)

## Rank 3 - Medium
- [ ] [Software architecture](./aspice/software/sw-architecture/sw-architecture.md)
- [ ] [BSW architecture](./aspice/software/sw-architecture/bsw-architecture.md)
- [ ] [vECU architecture](./aspice/software/sw-architecture/vecu-architecture.md)
- [ ] [SWR - CVC](./aspice/software/sw-requirements/SWR-CVC.md)
- [ ] [SWR - FZC](./aspice/software/sw-requirements/SWR-FZC.md)
- [ ] [SWR - RZC](./aspice/software/sw-requirements/SWR-RZC.md)
- [ ] [SWR - SC](./aspice/software/sw-requirements/SWR-SC.md)
- [ ] [SWR - BCM](./aspice/software/sw-requirements/SWR-BCM.md)
- [ ] [SWR - ICU](./aspice/software/sw-requirements/SWR-ICU.md)
- [ ] [SWR - TCU](./aspice/software/sw-requirements/SWR-TCU.md)
- [ ] [SWR - BSW](./aspice/software/sw-requirements/SWR-BSW.md)
- [ ] [Hardware overview](../hardware/README.md)
- [ ] [Pin mapping](../hardware/pin-mapping.md)
- [ ] [BOM](../hardware/bom.md)
- [ ] [Gateway overview](../gateway/README.md)

## Rank 4 - Reference
- [ ] [ASPICE plans index](./aspice/plans/README.md)
- [ ] [Verification index](./aspice/verification/INDEX.md)
- [ ] [QA plan](./aspice/quality/qa-plan.md)
- [ ] [CM strategy](./aspice/cm/cm-strategy.md)
- [ ] [Process playbook](./reference/process-playbook.md)

