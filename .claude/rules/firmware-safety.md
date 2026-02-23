---
paths:
  - "firmware/**/*.c"
  - "firmware/**/*.h"
  - "firmware/**/*.cpp"
  - "firmware/**/*.hpp"
  - "firmware/**/*.ino"
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


# Firmware Safety Standards

## Memory Safety

- NEVER use `gets()` — use `fgets()` with explicit buffer size
- NEVER use `strcpy()` — use `strncpy()` or `strlcpy()` with bounds
- NEVER use `sprintf()` — use `snprintf()` with buffer size limit
- NEVER use `strcat()` — use `strncat()` with remaining space calculation
- ALWAYS check buffer sizes before write operations
- ALWAYS validate array indices before access
- ALWAYS initialize variables before use — no uninitialized reads
- ALWAYS free dynamically allocated memory — no leaks
- NEVER use `malloc()` in interrupt context
- Prefer stack allocation over heap allocation where possible
- Use static allocation for fixed-size buffers in embedded contexts

## Integer Safety

- Check for integer overflow before arithmetic on user-supplied values
- Use fixed-width types: `uint8_t`, `uint16_t`, `uint32_t`, `int32_t`
- Do not assume `int` size — it varies by platform
- Cast explicitly when mixing signed and unsigned types
- Validate range before narrowing conversions (e.g., `uint32_t` to `uint8_t`)

## Pointer Safety

- ALWAYS check pointers for NULL before dereferencing
- Set pointers to NULL after `free()`
- Never return pointers to local (stack) variables
- Validate function pointer targets before calling
- Use `const` pointers when data should not be modified

## Concurrency & Interrupts

- Protect shared data with mutexes or critical sections
- Use `volatile` for hardware registers and ISR-shared variables
- Keep ISR handlers short — set flags, defer processing to main loop
- Never call blocking functions from ISR context
- Document which variables are shared between ISR and main context
- Use atomic operations for single-word shared counters

## Watchdog & Recovery

- Enable hardware watchdog timer in production builds
- Feed watchdog only from the main loop — proves the system is running
- Implement safe-state fallback if watchdog triggers
- Log reset reason on boot (watchdog, brownout, software reset, power-on)

## Preprocessor & Build

- Use header guards (`#ifndef HEADER_H`) or `#pragma once`
- Minimize use of `#define` macros — prefer `const` or `enum`
- Never use `#define` for complex expressions without parentheses
- Keep `#ifdef` nesting shallow — refactor if deeper than 2 levels
- Use `-Wall -Wextra -Werror` compiler flags (treat warnings as errors)

## Prohibited Patterns

```c
// BANNED — use safe alternatives:
gets(buf);                    // -> fgets(buf, sizeof(buf), stdin);
strcpy(dst, src);             // -> strncpy(dst, src, sizeof(dst)-1); dst[sizeof(dst)-1]='\0';
sprintf(buf, fmt, ...);      // -> snprintf(buf, sizeof(buf), fmt, ...);
strcat(dst, src);             // -> strncat(dst, src, sizeof(dst)-strlen(dst)-1);
atoi(str);                    // -> strtol(str, &end, 10); with error checking
system(cmd);                  // -> NEVER in production firmware
```

