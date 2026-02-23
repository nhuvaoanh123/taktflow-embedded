---
paths:
  - "firmware/src/**/*"
  - "firmware/include/**/*"
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


# Power Management Standards

## Power States

Define and document all power states for the device:

| State | CPU | Peripherals | Wake Sources | Typical Current |
|-------|-----|-------------|--------------|-----------------|
| ACTIVE | Full speed | All enabled | N/A | Highest |
| IDLE | Reduced clock | Active peripherals on | Any interrupt | Moderate |
| LIGHT_SLEEP | Suspended | Selected peripherals on | Timer, GPIO, UART | Low |
| DEEP_SLEEP | Off (RTC only) | All off | RTC timer, ext GPIO | Minimal |
| HIBERNATE | Off | Off (RTC off) | External reset only | Lowest |

## Design Rules

- Document current consumption budget for each operating mode
- Validate power budget against battery capacity / supply limits
- Implement orderly transition between power states:
  1. Save application state
  2. Close network connections gracefully (send LWT if MQTT)
  3. Flush pending data to storage
  4. Disable peripherals in reverse initialization order
  5. Enter low-power mode
  6. On wake: restore in initialization order

## Peripheral Power Control

- Disable unused peripherals (ADC, SPI, UART, BLE radio) when not in use
- Use peripheral power gating when hardware supports it
- Power on sensors only when reading — don't leave powered between samples
- Disable Wi-Fi/BLE radio when not actively communicating

## Battery Monitoring

- Monitor battery voltage/state-of-charge if battery-powered
- Define low-battery threshold — enter power-saving mode
- Define critical-battery threshold — save state and shut down gracefully
- Report battery status in telemetry data
- Warn user/backend before critical shutdown

## Sleep Strategy

- Use the lightest sleep state that meets latency requirements
- Wake periodically to check for pending work, then sleep again
- Use event-driven wake (interrupt) rather than polling when possible
- Calculate and document expected battery life for each usage pattern

## Measurement & Validation

- Measure actual current consumption in each power state (not just datasheet estimates)
- Test wake-up time from each sleep state
- Verify all wake sources function correctly
- Test behavior when multiple wake sources fire simultaneously
- Document measured values in `docs/power-budget.md`

