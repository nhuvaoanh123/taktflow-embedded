---
paths:
  - "firmware/src/**/*.c"
  - "firmware/src/**/*.h"
  - "firmware/src/**/*.cpp"
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


# Input Validation Standards

## Core Rule

Validate ALL data at system boundaries BEFORE processing. Never trust external input.

## Boundary Points (where validation is mandatory)

| Interface | Threat | Validation Required |
|-----------|--------|---------------------|
| Serial/UART | Malformed commands, buffer overflow | Length limits, format validation, command whitelist |
| MQTT messages | Injection, oversized payloads, spoofing | Topic validation, payload size limits, schema check |
| BLE characteristics | Oversized writes, malformed data | Length check, value range, type validation |
| HTTP requests | Injection, overflow, auth bypass | Input sanitization, auth check, size limits |
| GPIO/ADC readings | Out-of-range, noise, glitches | Range clamping, debounce, moving average |
| Flash/EEPROM reads | Corruption, tampering | CRC/checksum verification, magic number check |
| OTA payloads | Malicious firmware, corruption | Signature verification, size validation, version check |

## Validation Patterns

### String Input
```
- Check length BEFORE copying (reject if > max)
- Null-terminate after every copy operation
- Reject or escape special characters (null bytes, control chars)
- Validate encoding (ASCII-only unless UTF-8 explicitly supported)
```

### Numeric Input
```
- Validate range: MIN <= value <= MAX
- Check for overflow before arithmetic
- Reject NaN, Infinity for floating point
- Use strtol/strtoul with error checking, not atoi
```

### Structured Data (JSON, protobuf, CBOR)
```
- Validate schema before processing fields
- Enforce maximum nesting depth (prevent stack overflow)
- Enforce maximum total message size
- Validate all required fields present
- Reject unknown fields in strict mode
```

### Command Parsing
```
- Use command whitelist — reject unknown commands
- Validate argument count and types per command
- Enforce maximum command length
- Rate-limit command processing (prevent flooding)
```

## Sanitization Order

1. Check total input size (reject if over limit)
2. Validate encoding / character set
3. Parse structure (JSON, protocol buffer, etc.)
4. Validate schema / required fields
5. Validate individual field values (range, format, length)
6. THEN process the validated data

## Anti-Patterns (NEVER do this)

- Never parse input and validate later — validate first, parse validated data
- Never log raw input without sanitization — could contain control characters
- Never use input directly in format strings — always use parameterized formats
- Never assume input encoding — explicitly validate or reject

