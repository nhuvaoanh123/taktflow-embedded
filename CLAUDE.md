## HITL Comment Lock

Never edit/reformat/move/delete text inside `HITL-LOCK` blocks. Append-only outside locks.
Markers: `HITL-LOCK START:<id>` / `HITL-LOCK END:<id>` (use comment syntax for file type)

# Taktflow Embedded

ISO 26262 ASIL D zonal vehicle platform — 7 ECUs (4 physical STM32/TMS570 + 3 simulated Docker), AUTOSAR-like BSW, CAN, cloud IoT, edge ML, UDS, SAP QM.

## Workflow
Plan first → `docs/plans/` → approve → code. Update plan before next phase. Use `/plan-feature`.

## BOM
`hardware/bom.md` = single source of truth for procurement/delivery. Never duplicate status elsewhere.

## Build & Test
`make build` | `build-debug` | `build-release` | `test` | `flash` | `/firmware-build all`

## Project Layout
```
firmware/{cvc,fzc,rzc}/  — STM32 physical ECUs (src/, include/, cfg/, test/)
firmware/sc/             — TMS570 Safety Controller (NO AUTOSAR)
firmware/{bcm,icu,tcu}/  — Docker simulated ECUs
firmware/shared/bsw/     — AUTOSAR-like BSW (mcal/, ecual/, services/, rte/)
docker/                  — Dockerfiles for simulated ECUs
gateway/                 — RPi edge gateway, SAP QM mock
hardware/                — Pin mappings, BOM, schematics
scripts/                 — Build scripts, trace-gen, baseline-tag
test/{mil,sil,hil}/      — xIL testing
docs/INDEX.md            — Master document registry
docs/plans/              — Master plan (source of truth)
docs/safety/             — ISO 26262 (concept, plan, analysis, requirements, validation)
docs/aspice/             — ASPICE deliverables (plans, system, software, HW, verification, QA, CM, traceability)
docs/reference/          — Process playbook, lessons learned
```

## Standards
All rules in `.claude/rules/` — see filenames for topics. Full ASIL D reference: `docs/reference/asil-d-reference.md`.

## Skills
- `/plan-feature <name>` — Structured implementation plan
- `/security-review [path]` — Security audit
- `/firmware-build [debug|release|test|all]` — Build and validate

## Reference
- @docs/reference/process-playbook.md
