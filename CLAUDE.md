# Taktflow Embedded — Claude Code Instructions

## Project

IoT / microcontroller embedded system project. Part of the Taktflow ecosystem.

## Workflow Rules

- **Always write a plan before implementing** — write to `docs/plans/` and get approval before coding
- **Always update plans before continuing** — update the plan doc before starting next phase
- **Ask before implementing** — get user approval first

## Project Structure

```
firmware/
  src/          # Application source code
  include/      # Header files
  lib/          # Third-party libraries
  test/         # Unit tests
hardware/       # Schematics, PCB files, pin mappings
scripts/        # Build scripts, flash scripts, utilities
docs/
  plans/        # Implementation plans (write plans here)
  reference/    # Lessons learned, process playbook
```

## Reference Docs

- `docs/reference/process-playbook.md` — workflow patterns from Taktflow web project
- `docs/reference/lessons-learned-security-hardening.md` — security patterns (applicable to IoT security)

## Security Considerations (from web project, adapted for embedded)

- Fail-closed: missing config = reject, don't skip
- No hardcoded credentials in firmware
- Validate all external input (serial, MQTT, BLE, HTTP)
- Use TLS for all network communication
- OTA updates must be signed
