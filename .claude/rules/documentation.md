---
paths:
  - "**/*"
---

# Documentation Standards

## Code Documentation

### Function Comments (mandatory for all public functions)
```c
/**
 * @brief  Parse incoming MQTT message and dispatch to handler
 * @param  topic   Null-terminated topic string (max 128 chars)
 * @param  payload Raw payload buffer
 * @param  len     Payload length in bytes (max 4096)
 * @return ERR_OK on success, ERR_INVALID_PARAM if topic/payload NULL,
 *         ERR_BUFFER_OVERFLOW if len > max
 * @note   Called from MQTT receive callback — must be ISR-safe
 */
error_t mqtt_handle_message(const char *topic, const uint8_t *payload, size_t len);
```

### Required Documentation Elements
- `@brief` — What the function does (one line)
- `@param` — Every parameter: name, type expectation, constraints
- `@return` — All possible return values and their meaning
- `@note` — Thread safety, ISR context, side effects, preconditions

### When to Add Inline Comments
- Explain WHY, not WHAT — the code shows what, comments explain why
- Non-obvious algorithms or mathematical formulas
- Hardware-specific behavior or timing requirements
- Workarounds for hardware bugs or library limitations
- Security-critical decisions ("we validate here because...")

### When NOT to Comment
- Self-explanatory code: `count++; // increment count` is noise
- Standard patterns that any C developer recognizes
- Commented-out code — delete it, version control remembers

## File Headers

Every source file must include:
```c
/**
 * @file    module_name.c
 * @brief   One-line description of this module
 * @author  Author name or team
 * @date    Creation date (YYYY-MM-DD)
 *
 * Longer description if needed: what this module does,
 * what hardware it interfaces with, key design decisions.
 */
```

## Architecture Documentation

- `docs/architecture.md` — High-level system design, data flow, component diagram
- `docs/protocols.md` — Communication protocol specifications (MQTT topics, BLE services, serial commands)
- `docs/hardware-setup.md` — Physical connections, power supply, antenna placement
- `hardware/pin-mapping.md` — Complete pin assignment table

## Plan Documentation

- All implementation plans in `docs/plans/`
- Plan format: phases with status table, checkboxes, DONE criteria
- Update plan BEFORE starting next phase (source of truth)

## Change Documentation

- Breaking changes documented in commit message AND relevant docs
- API changes require updated function comments
- Hardware changes require updated pin mapping and schematic

## Claim Accuracy (from Taktflow lessons learned)

- Audit every public-facing claim against actual behavior
- Use qualifiers: "by default", "configurable", "up to" — avoid absolutes
- Claim validation is a release gate — not optional
- Dynamic stats preferred over hardcoded numbers
