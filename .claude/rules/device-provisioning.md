---
paths:
  - "firmware/src/**/*"
  - "scripts/**/*"
---

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


# Device Provisioning Standards

## Unique Device Identity

- Every device MUST have a unique identity (serial number, UUID, or hardware ID)
- Prefer hardware-derived identity (MAC address, chip UID, secure element serial)
- Store device identity in protected, non-erasable memory when possible
- Device identity must be readable by firmware and reportable to backend

## Credential Provisioning

- Provision per-device credentials during manufacturing or first-boot setup
- NEVER use shared credentials across devices (no fleet-wide master password)
- Support credential rotation without physical access (remote key update)
- Store credentials in encrypted flash or secure element
- Provisioning channel must be secure (HTTPS, authenticated BLE, physical serial)

## Provisioning Flow

1. **Factory provisioning** (manufacturing line):
   - Flash firmware
   - Write device identity
   - Generate and store device credentials (keys, certificates)
   - Write initial configuration (product type, region, fleet ID)
   - Run factory test suite
   - Record provisioning data in manufacturing database

2. **Field provisioning** (user setup):
   - Device advertises BLE or creates AP for configuration
   - User provides Wi-Fi credentials, cloud endpoint, user account binding
   - Device validates connection to backend
   - Backend confirms device registration
   - Device enters normal operation

## Configuration Management

- Separate configuration from firmware — config should survive firmware updates
- Define configuration schema with defaults for every field
- Validate configuration on load — reject corrupt or incomplete config
- Support configuration reset to factory defaults (physical button or remote command)
- Version the configuration schema — handle migration between versions
- Log configuration changes

## Zero-Touch Provisioning (when possible)

- Device should be able to self-register with backend on first boot
- Use hardware identity + attestation for initial trust
- Backend provisions full credentials after identity verification
- Fallback to manual provisioning if zero-touch fails

## Decommissioning

- Implement secure wipe: erase all credentials, keys, user data
- Decommission must be triggerable locally AND remotely
- Log decommission event before erasing
- After decommission, device should enter a clean factory state
- Notify backend of decommission event

