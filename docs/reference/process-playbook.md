# Process Playbook — Carried from Taktflow Systems

Proven workflow patterns from the Taktflow web project. Apply these to embedded work.

## Planning Discipline

1. **Always write a plan before implementing** — write to a plan file, get approval before coding
2. **Always update plans before continuing** — update the plan doc to reflect current state before starting next phase
3. **Ask before implementing** — get approval first, don't just start

## Phase-Based Execution

- Break work into numbered phases with clear DONE criteria
- Maintain a master plan with a status table (Phase | Name | Status)
- Each phase gets `IN PROGRESS` → `DONE`
- Mark individual items with `[x]` checkboxes

## Security-First Mindset

- **Fail-closed**: If a secret/config is missing, reject — don't silently skip
- **Generic errors**: Never expose internal error messages to external interfaces
- **Input validation at boundaries**: Validate at system edges (serial input, MQTT messages, API calls)
- **Graceful degradation**: Features should degrade safely when dependencies are unavailable
- **No hardcoded secrets**: All credentials via environment/config files, never in source

## Claim Accuracy

- Audit every public-facing claim (datasheet, marketing, README) against actual behavior
- Use qualifiers: "by default", "configurable", "up to" — avoid absolutes
- Make claim validation a release gate

## TODO Markers

Use greppable markers for deferred work:

```
// TODO:SCALE — description of what to change for production scale
// TODO:POST-BETA — description of post-launch enhancement
// TODO:HARDWARE — needs hardware validation
```

## Vendor Independence

- **Lesson from Taktflow**: Started with Resend (vendor SDK), had to rip it out and switch to generic SMTP
- **Rule**: Prefer generic protocols (MQTT, HTTP, SMTP) over vendor SDKs when possible
- **If using a vendor SDK**: Wrap it in an abstraction layer so swapping is a 1-file change
