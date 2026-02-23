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

# RTE — Runtime Environment

Signal buffer and port connections between SWCs and BSW.

| Feature | Description |
|---------|-------------|
| Rte_Read | SWC reads signal from buffer |
| Rte_Write | SWC writes signal to buffer |
| Runnable scheduling | Which SWC runs at which tick rate |
| Port connections | Compile-time per-ECU configuration |

Per-ECU configs: firmware/{ecu}/cfg/Rte_Cfg_{Ecu}.c

Phase 5 deliverable.

