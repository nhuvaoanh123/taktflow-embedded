# Project State — Taktflow Embedded

> Auto-updated before context compression. Read this to restore full context.

**Last updated**: 2026-02-20
**Branch**: `develop`
**Last commit**: `da66c4e` — Remove settings.local.json from tracking

---

## What This Project Is

IoT / microcontroller embedded system, part of the Taktflow ecosystem. Targeting **ISO 26262 ASIL D** automotive functional safety compliance with **ASPICE 4.0** process maturity.

**Current product concept**: Automated musical instrument — 32 glockenspiel bars + solenoid strikers, ESP32-S3 controller, BLE MIDI, phone app. (Conceptual stage, no plan written yet.)

---

## Repository Structure

```
taktflow-embedded/
├── .claude/
│   ├── settings.json          # Team-shared permissions + hooks
│   ├── hooks/
│   │   ├── protect-files.sh   # Blocks edits to .env, .pem, .key, secrets/
│   │   └── lint-firmware.sh   # Blocks banned C functions (gets, strcpy, sprintf, etc.)
│   ├── rules/                 # 28 rule files (see CLAUDE.md for full index)
│   │   ├── [15 embedded best practice rules]
│   │   └── [11 ISO 26262 / ASIL D / ASPICE rules]
│   └── skills/
│       ├── security-review/   # /security-review — audit code for vulnerabilities
│       ├── plan-feature/      # /plan-feature — structured implementation plan
│       └── firmware-build/    # /firmware-build — build + test + validate
├── firmware/
│   ├── src/                   # EMPTY — no code yet
│   ├── include/               # EMPTY
│   ├── lib/                   # EMPTY
│   └── test/                  # EMPTY
├── hardware/                  # EMPTY
├── scripts/                   # EMPTY
├── docs/
│   ├── plans/                 # EMPTY — awaiting first plan
│   ├── reference/
│   │   ├── process-playbook.md
│   │   └── lessons-learned-security-hardening.md
│   └── PROJECT_STATE.md       # THIS FILE
├── CLAUDE.md                  # Tier 1 project instructions (brief)
└── .gitignore
```

---

## Git State

- **Branching**: Git Flow — main (protected) → develop → feature/ / release/ / hotfix/
- **Remote**: https://github.com/nhuvaoanh123/taktflow-embedded.git
- **GitHub CLI**: Installed, authenticated as nhuvaoanh123
- **Default branch**: main

### Commit History
```
da66c4e (develop) Remove settings.local.json from tracking
e96fafd (develop) Add Git Flow branching strategy and update rules
41dfb28 (main)    Boundaries Docus — all 28 rules, skills, hooks, settings
f29d3df (main)    Initial project scaffold with reference docs
```

develop is 2 commits ahead of main.

---

## Rules System (28 files in .claude/rules/)

### Embedded Best Practices (15)
workflow, firmware-safety, security, input-validation, networking, ota-updates, hardware, testing, error-handling, code-style, power-management, state-machines, logging-diagnostics, device-provisioning, vendor-independence, build-and-ci, documentation

### ISO 26262 / ASIL D / Automotive (11)
iso-compliance, asil-d-software, asil-d-hardware, asil-d-architecture, asil-d-verification, misra-c, aspice, safety-lifecycle, tool-qualification, traceability, asil-decomposition

---

## User Preferences

- **Auto commit and push** — commit and push after completing work, no asking
- **Plan before implementing** — write to docs/plans/, get approval first
- **Comprehensive rules upfront** — update later with more restrictions
- **Security-first, fail-closed** throughout
- **Git Flow** branching strategy

---

## What's Next (not started)

- No firmware code written yet
- No implementation plans created yet
- Product concept (musical instrument) is in discussion stage
- First plan should go to `docs/plans/`
