# Project State — Taktflow Embedded

> Auto-updated before context compression. Read this to restore full context.

**Last updated**: 2026-02-20
**Branch**: `develop`
**Phase**: Pre-planning — product defined, starting Day 1

---

## What This Project Is

**Portfolio project** demonstrating ISO 26262 ASIL D automotive functional safety engineering.

**Product**: Mini Vehicle Platform — 7-ECU system-of-systems connected via CAN bus, demonstrating drive-by-wire with full safety architecture.

### The 7 ECUs

| ECU | Full Name | Role | ASIL | Hardware |
|-----|-----------|------|------|----------|
| VCU | Vehicle Control Unit | Master arbitrator, pedal input, E-stop | D | ESP32 |
| PCU | Powertrain Control Unit | Motor control, current/temp monitoring | D | STM32 |
| BCU | Brake Control Unit | Brake servo, parking brake, hill hold | D | STM32 |
| SCU | Steering Control Unit | Steering servo, angle feedback | D | Arduino Nano |
| ADAS | Advanced Driver Assist | Distance sensing, emergency brake request | B/D | Arduino Nano |
| BMS | Battery Management System | Voltage, current, temp, kill relay | C | Arduino Nano |
| Safety MCU | Independent Monitor | Alive monitoring, CAN watchdog, system kill | D | STM32 |

### 15 Demo Fault Scenarios
1. Normal driving
2. Pedal sensor disagreement → limp mode
3. Pedal sensor failure → motor stops
4. Object detected → emergency brake (cross-ECU)
5. Motor overcurrent → PCU cuts power
6. Battery overtemp → system derates
7. Battery critical → kill relay
8. Steering sensor fault → return to center
9. CAN bus failure → safety MCU kills
10. PCU hangs → safety MCU kills motor
11. Hill hold → auto brake
12. E-stop → broadcast stop
13. ADAS sensor blocked → disable auto-brake
14. VCU vs Safety MCU disagree → safety wins
15. Multiple faults → cascading degradation

---

## Repository Structure

```
taktflow-embedded/
├── .claude/
│   ├── settings.json, hooks/, rules/ (28 files), skills/ (3)
├── firmware/src/          — EMPTY, awaiting Day 4+
├── firmware/test/         — EMPTY, awaiting Day 9
├── hardware/              — EMPTY, awaiting Day 4
├── docs/
│   ├── plans/             — EMPTY, awaiting Day 1 plan
│   ├── safety/            — EMPTY, awaiting Day 1
│   ├── aspice/            — EMPTY, awaiting Day 3
│   ├── reference/         — process-playbook.md, lessons-learned.md
│   └── PROJECT_STATE.md   — THIS FILE
├── CLAUDE.md
└── .gitignore
```

---

## Git State

- **Branching**: Git Flow (main → develop → feature/)
- **Remote**: github.com/nhuvaoanh123/taktflow-embedded
- develop is 3 commits ahead of main

## User Preferences

- Auto commit and push after work
- Plan before implementing
- Update PROJECT_STATE.md before context compression
- Building as portfolio for automotive FSE job
- Full 7-ECU scope, no cutting corners
