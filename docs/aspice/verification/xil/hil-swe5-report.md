---
document_id: HIL-SWE5-RPT
title: "HIL Integration Test Report (SWE.5)"
version: "0.2"
status: planned
aspice_process: SWE.5
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


# HIL Integration Test Report (SWE.5)

<!-- Phase 12 deliverable -->
<!-- Note: Our setup (4 physical ECUs + 3 Docker ECUs + plant-sim on Pi)
     is HIL. See hil-report.md (SYS.4) for full system validation. -->

## Purpose

Validate real-time behavior on target MCU with simulated plant via CAN bridge.

## Configuration

- 4 physical ECUs (CVC, FZC, RZC on STM32G474RE + SC on TMS570) on real CAN bus
- 3 Docker ECUs (BCM, ICU, TCU) on Raspberry Pi via USB-CAN adapter
- Plant-sim provides lidar simulation (0x220); virtual sensor TX disabled in HIL mode
- `docker-compose.hil.yml` override switches all services to can0

## Test Scenarios

| Scenario | Target MCU | Real-Time Met | CAN Jitter |
|----------|-----------|---------------|------------|
| — | — | — | — |

## SIL vs HIL Comparison

| Metric | SIL | HIL | Delta |
|--------|-----|-----|-------|
| Loop time | — | — | — |
| CAN latency | — | — | — |

## Results

<!-- To be completed in Phase 12 -->

