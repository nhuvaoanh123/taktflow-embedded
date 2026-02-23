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

# BSW Services Layer

Application-level BSW services.

| Module | Purpose | Est. LOC |
|--------|---------|----------|
| Com | Signal packing/unpacking, timeouts | ~400 |
| Dcm | UDS diagnostic service dispatch | ~500 |
| Dem | DTC storage, status bits, debouncing | ~300 |
| WdgM | Supervised entity alive monitoring | ~200 |
| BswM | ECU mode management | ~150 |
| E2E | CRC-8, alive counter, data ID | ~100 |

Phase 5 deliverable.

