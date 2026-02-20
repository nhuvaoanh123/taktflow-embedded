# Taktflow Embedded

IoT / microcontroller embedded system. Part of the Taktflow ecosystem.

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
firmware/src/       — Application code
firmware/include/   — Headers
firmware/lib/       — Third-party libraries
firmware/test/      — Unit tests (test_<module>.c)
hardware/           — Schematics, PCB, pin mappings
scripts/            — Build/flash/utility scripts
docs/plans/         — Implementation plans
docs/reference/     — Process playbook, lessons learned
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
