# SYS.1 Safety and Requirement Baseline Plan

Process area: SYS.1 System Requirements Analysis
Scope: phases 1 to 3 from master plan

## Entry Criteria

- [ ] Repository baseline is stable
- [ ] System boundary approved (7 ECUs + gateway)
- [ ] Hazard assumptions documented for demo context

## Current Artifact Status (SYS.1 / SYS.2 Inputs)

- [x] `docs/aspice/system/stakeholder-requirements.md` (SYS.1, drafted)
- [x] `docs/aspice/system/system-requirements.md` (SYS.2, drafted)
- [x] `docs/safety/concept/item-definition.md` (drafted)
- [x] `docs/safety/concept/hara.md` (drafted)
- [x] `docs/safety/concept/safety-goals.md` (drafted)
- [x] `docs/safety/concept/functional-safety-concept.md` (drafted)
- [x] `docs/safety/plan/safety-plan.md` (drafted)
- [ ] Formal review + baseline signoff pending

## Work Breakdown

## WS1 Item Definition and Safety Concept

- [ ] Define operating context and interfaces in `docs/safety/concept/item-definition.md`
- [ ] Produce HARA in `docs/safety/concept/hara.md` with at least 12 hazardous events
- [ ] Derive safety goals in `docs/safety/concept/safety-goals.md`
- [ ] Define safe states and FTTI per safety goal
- [ ] Build functional safety concept in `docs/safety/concept/functional-safety-concept.md`
- [ ] Publish safety plan in `docs/safety/plan/safety-plan.md`

## WS2 Safety Analysis Baseline

- [ ] Create system FMEA in `docs/safety/analysis/fmea.md`
- [ ] Create DFA in `docs/safety/analysis/dfa.md`
- [ ] Compute hardware metrics in `docs/safety/analysis/hardware-metrics.md`
- [ ] Document ASIL decomposition decisions in `docs/safety/analysis/asil-decomposition.md`
- [ ] Record assumptions for non-certified MCU failure rates

## WS3 Requirement and Traceability Baseline

- [ ] Publish TSR set in `docs/safety/requirements/technical-safety-reqs.md`
- [ ] Publish SSR set in `docs/safety/requirements/sw-safety-reqs.md`
- [ ] Publish HSR set in `docs/safety/requirements/hw-safety-reqs.md`
- [ ] Publish architecture links in `docs/aspice/system/system-architecture.md`
- [ ] Publish software architecture links in `docs/aspice/software/sw-architecture/sw-architecture.md`
- [ ] Complete `docs/aspice/traceability/traceability-matrix.md` with SG -> FSR -> TSR -> SSR -> implementation

## Required Outputs

- [ ] Safety concept package complete
- [ ] Safety analysis package complete
- [ ] Traceability baseline complete

## Review Checklist (Gate G1)

- [ ] Every hazardous event has ASIL and rationale
- [ ] Every safety goal has FTTI and safe state
- [ ] No requirement in matrix without linked architecture element
- [ ] Assumptions and open risks explicitly listed
