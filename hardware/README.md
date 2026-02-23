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

# Hardware Folder Structure

## Files

- `hardware/bom-list.md` - procurement-ready BOM checklist (quick buy/use)
- `hardware/bom.md` - detailed BOM with alternatives and phased procurement
- `hardware/pin-mapping.md` - board and signal pin mappings

## Subfolders

- `hardware/schematics/` - schematics and related notes

## Recommended Workflow

1. Buy against `hardware/bom-list.md`.
2. Track substitutions and alternatives in `hardware/bom.md`.
3. Update `hardware/pin-mapping.md` if component variants change.

