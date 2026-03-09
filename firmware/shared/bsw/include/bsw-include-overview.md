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

# BSW Shared Headers

Platform-independent type definitions and common headers.

| Header | Purpose |
|--------|---------|
| Platform_Types.h | uint8, uint16, uint32, boolean, etc. |
| Std_Types.h | Std_ReturnType, E_OK, E_NOT_OK |
| ComStack_Types.h | PduIdType, PduInfoType, PduLengthType |

Phase 5 deliverable.

