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

# Taktflow Embedded

Automotive functional safety portfolio project — ISO 26262 ASIL D zonal vehicle platform with 7 ECUs (4 physical STM32/TMS570 + 3 simulated Docker), AUTOSAR-like BSW, CAN bus, cloud IoT, edge ML, UDS diagnostics, and SAP QM integration.

## Workflow

- **Plan before implementing** — write to `docs/plans/`, get approval before coding
- **Update plans before continuing** — plan doc is source of truth
- **Ask before implementing** — get user approval first
- Use `/plan-feature` skill for structured planning

## Build & Test

- Build: `make build` (debug: `make build-debug`, release: `make build-release`)
- Test: `make test`
- Flash: `make flash`
- Full validation: `/firmware-build all`

## Project Layout

```
firmware/
  cvc/src/, include/, cfg/, test/     — Central Vehicle Computer (STM32, physical)
  fzc/src/, include/, cfg/, test/     — Front Zone Controller (STM32, physical)
  rzc/src/, include/, cfg/, test/     — Rear Zone Controller (STM32, physical)
  sc/src/, include/, test/            — Safety Controller (TMS570, physical, NO AUTOSAR)
  bcm/src/, include/, cfg/, test/     — Body Control Module (Docker, simulated)
  icu/src/, include/, cfg/, test/     — Instrument Cluster Unit (Docker, simulated)
  tcu/src/, include/, cfg/, test/     — Telematics Control Unit (Docker, simulated)
  shared/bsw/                         — AUTOSAR-like BSW (MCAL, CanIf, PduR, Com, Dcm, Dem, WdgM, BswM, RTE)
    mcal/                             — CAN, SPI, ADC, PWM, Dio, Gpt
    ecual/                            — CanIf, PduR, IoHwAb
    services/                         — Com, Dcm, Dem, WdgM, BswM, E2E
    rte/                              — Runtime Environment
    include/                          — Platform_Types, Std_Types
docker/                               — Dockerfile, docker-compose for simulated ECUs
gateway/                              — Raspberry Pi edge gateway (Python), SAP QM mock
  sap_qm_mock/                        — SAP QM mock API
  tests/, models/                     — Gateway tests, ML models
hardware/                             — Pin mappings, BOM, schematics
scripts/                              — Build scripts, trace-gen.py, baseline-tag.sh
test/mil/, test/sil/, test/pil/       — xIL testing (MIL, SIL, PIL)
docs/
  INDEX.md                            — Master document registry (single entry point)
  plans/                              — Master plan (source of truth)
  safety/                             — ISO 26262 Parts 2-9
    concept/                          — Item definition, HARA, safety goals, FSC
    plan/                             — Safety plan, safety case
    analysis/                         — FMEA, DFA, hardware metrics, ASIL decomposition
    requirements/                     — FSR, TSR, SSR, HSR, HSI
    validation/                       — Safety validation report
  aspice/                             — ASPICE deliverables (point of truth)
    plans/                            — Execution plans by process area (MAN.3, SYS, SWE)
    system/                           — SYS.1-3: stakeholder reqs, system reqs, architecture, CAN matrix
    software/                         — SWE.1-2: SW requirements (per ECU), SW architecture
    hardware-eng/                     — HWE.1-3: HW requirements, HW design
    verification/                     — SWE.4-6, SYS.4-5: unit/integration/system test, xIL reports
    quality/                          — SUP.1: QA plan
    cm/                               — SUP.8: CM strategy, baselines, change requests
    traceability/                     — Traceability matrix (SG → FSR → TSR → SSR → code → test)
  reference/                          — Process playbook, lessons learned
```

## Standards

All coding standards, security rules, and best practices are in `.claude/rules/`:

### Embedded Best Practices
- `workflow.md` — Planning discipline, phase execution, commit rules
- `firmware-safety.md` — Memory safety, banned functions, compiler flags
- `security.md` — Fail-closed, credentials, crypto, secure boot
- `input-validation.md` — Validation at every external boundary
- `networking.md` — TLS, MQTT, BLE, Wi-Fi, connection resilience
- `ota-updates.md` — Signed updates, A/B partitions, rollback
- `hardware.md` — HAL abstraction, pin mapping, peripherals
- `testing.md` — Unit tests, coverage, fuzzing, regression
- `error-handling.md` — Fail-closed, error codes, safe states
- `code-style.md` — Naming, formatting, file organization
- `power-management.md` — Sleep states, battery, peripheral control
- `state-machines.md` — State/event enums, transition tables
- `logging-diagnostics.md` — Log levels, security events, crash data
- `device-provisioning.md` — Identity, credentials, config management
- `vendor-independence.md` — Generic protocols, abstraction layers
- `build-and-ci.md` — CI pipeline, compiler flags, reproducible builds
- `documentation.md` — Code comments, file headers, architecture docs

### ISO 26262 / ASIL D / Automotive
- `iso-compliance.md` — ISO 26262 overview, ASIL matrix, standards index
- `asil-d-software.md` — ASIL D software constraints (Part 6), MC/DC, defensive programming
- `asil-d-hardware.md` — Hardware metrics (Part 5), SPFM >= 99%, LFM >= 90%, PMHF < 10 FIT
- `asil-d-architecture.md` — System architecture (Part 4), FFI, timing, E2E, fail-safe/fail-operational
- `asil-d-verification.md` — Verification methods, independence (I3), fault injection, formal verification
- `misra-c.md` — MISRA C:2012/2023 rules, deviations, enforcement
- `aspice.md` — Automotive SPICE 4.0 process areas, V-model, capability levels
- `safety-lifecycle.md` — FSE role, HARA, safety case, safety plan, DFA
- `tool-qualification.md` — TCL classification, compiler qualification, tool validation
- `traceability.md` — Bidirectional traceability (SG -> FSR -> TSR -> SSR -> code -> tests)
- `asil-decomposition.md` — Decomposition rules, independence proof, constraints

## Skills

- `/plan-feature <name>` — Create structured implementation plan
- `/security-review [path]` — Audit code for security issues
- `/firmware-build [debug|release|test|all]` — Build and validate

## Reference

- @docs/reference/process-playbook.md
- @docs/reference/lessons-learned-security-hardening.md

