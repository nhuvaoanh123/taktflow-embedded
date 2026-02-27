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

## Lessons Learned Rule

Every process topic in this document that undergoes HITL review discussion MUST have its own lessons-learned file in [`docs/reference/lessons-learned/`](lessons-learned/). One file per process topic. File naming: `PROCESS-<topic>.md`.

# Process Playbook — Carried from Taktflow Systems

Proven workflow patterns from the Taktflow web project. Apply these to embedded work.

<!-- HITL-LOCK START:COMMENT-BLOCK-PLAYBOOK-SEC1 -->
**HITL Review (An Dao) — Reviewed: 2026-02-27:** The framing is accurate — these are patterns carried forward from the web project. The intro correctly positions this as a reference for embedded work. The key question for each section below is whether the web-originated pattern applies cleanly to automotive embedded development or needs adaptation.
<!-- HITL-LOCK END:COMMENT-BLOCK-PLAYBOOK-SEC1 -->

## Planning Discipline

1. **Always write a plan before implementing** — write to a plan file, get approval before coding
2. **Always update plans before continuing** — update the plan doc to reflect current state before starting next phase
3. **Ask before implementing** — get approval first, don't just start

<!-- HITL-LOCK START:COMMENT-BLOCK-PLAYBOOK-SEC2 -->
**HITL Review (An Dao) — Reviewed: 2026-02-27:** Planning discipline rules are directly carried from the web project and fully apply to embedded work. These align with ASPICE MAN.3 (project management) and the project's workflow rules in `workflow.md`. The plan-before-implement pattern is especially critical for ISO 26262 where every design decision must be documented and traceable. No adaptation needed for embedded context.
<!-- HITL-LOCK END:COMMENT-BLOCK-PLAYBOOK-SEC2 -->

## Phase-Based Execution

- Break work into numbered phases with clear DONE criteria
- Maintain a master plan with a status table (Phase | Name | Status)
- Each phase gets `IN PROGRESS` → `DONE`
- Mark individual items with `[x]` checkboxes

<!-- HITL-LOCK START:COMMENT-BLOCK-PLAYBOOK-SEC3 -->
**HITL Review (An Dao) — Reviewed: 2026-02-27:** Phase-based execution translates directly to automotive development — the 14-phase master plan already uses this pattern. For embedded, this maps well to V-model gate reviews and ASPICE milestone management. The DONE criteria concept is critical for gate readiness (see `gate-readiness-checklist.md`). No issues, fully applicable to embedded.
<!-- HITL-LOCK END:COMMENT-BLOCK-PLAYBOOK-SEC3 -->

## Security-First Mindset

- **Fail-closed**: If a secret/config is missing, reject — don't silently skip
- **Generic errors**: Never expose internal error messages to external interfaces
- **Input validation at boundaries**: Validate at system edges (serial input, MQTT messages, API calls)
- **Graceful degradation**: Features should degrade safely when dependencies are unavailable
- **No hardcoded secrets**: All credentials via environment/config files, never in source

<!-- HITL-LOCK START:COMMENT-BLOCK-PLAYBOOK-SEC4 -->
**HITL Review (An Dao) — Reviewed: 2026-02-27:** The security-first mindset carries over well to embedded. The principles align with ISO/SAE 21434 (automotive cybersecurity) and the project's `security.md` rules. For embedded specifically, "fail-closed" has a safety dimension beyond security — it maps to ISO 26262 safe states. The "input validation at boundaries" bullet correctly includes embedded-specific interfaces (serial input, MQTT). "Graceful degradation" should be linked to the fail-safe/fail-operational architecture concepts from `asil-d-architecture.md`.
<!-- HITL-LOCK END:COMMENT-BLOCK-PLAYBOOK-SEC4 -->

## Claim Accuracy

- Audit every public-facing claim (datasheet, marketing, README) against actual behavior
- Use qualifiers: "by default", "configurable", "up to" — avoid absolutes
- Make claim validation a release gate

<!-- HITL-LOCK START:COMMENT-BLOCK-PLAYBOOK-SEC5 -->
**HITL Review (An Dao) — Reviewed: 2026-02-27:** Claim accuracy is a valuable lesson from the web project (homepage stats drifting from code). For embedded, this applies to datasheet specifications, performance claims, safety compliance statements, and ASIL ratings. In automotive context, inaccurate claims about safety ratings or compliance have legal liability implications far beyond marketing. This pattern should be elevated to a mandatory release gate for any public-facing embedded documentation.
<!-- HITL-LOCK END:COMMENT-BLOCK-PLAYBOOK-SEC5 -->

## TODO Markers

Use greppable markers for deferred work:

```
// TODO:SCALE — description of what to change for production scale
// TODO:POST-BETA — description of post-launch enhancement
// TODO:HARDWARE — needs hardware validation
```

<!-- HITL-LOCK START:COMMENT-BLOCK-PLAYBOOK-SEC6 -->
**HITL Review (An Dao) — Reviewed: 2026-02-27:** The TODO marker pattern is well-established from the web project. For embedded, the markers listed here are a subset — the full set is defined in `workflow.md` and includes additional embedded-specific markers: `TODO:SECURITY`, `TODO:ISO`, `TODO:TEST`, `TODO:ASPICE`, `TODO:MISRA`. This section should either reference the full list in `workflow.md` or be updated to include the embedded-specific markers.
<!-- HITL-LOCK END:COMMENT-BLOCK-PLAYBOOK-SEC6 -->

## Vendor Independence

- **Lesson from Taktflow**: Started with Resend (vendor SDK), had to rip it out and switch to generic SMTP
- **Rule**: Prefer generic protocols (MQTT, HTTP, SMTP) over vendor SDKs when possible
- **If using a vendor SDK**: Wrap it in an abstraction layer so swapping is a 1-file change

<!-- HITL-LOCK START:COMMENT-BLOCK-PLAYBOOK-SEC7 -->
**HITL Review (An Dao) — Reviewed: 2026-02-27:** Vendor independence is even more critical in embedded than web — hardware vendor lock-in (MCU, RTOS, tool chain) is harder to reverse than a web service SDK swap. The Resend lesson translates directly. For the embedded project, the detailed vendor independence rules are in `vendor-independence.md` including HAL abstraction patterns, cloud service independence, and the abstraction layer code pattern. This section should cross-reference that rule file. The automotive-specific vendor concern (MCU vendor lock-in via proprietary MCAL/BSW) is not mentioned here but is addressed by the AUTOSAR-like BSW architecture.
<!-- HITL-LOCK END:COMMENT-BLOCK-PLAYBOOK-SEC7 -->

