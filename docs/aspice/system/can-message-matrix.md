---
document_id: CAN-MATRIX
title: "CAN Message Matrix"
version: "0.1"
status: planned
aspice_process: SYS.3
---

# CAN Message Matrix

<!-- Phase 4 deliverable — see docs/plans/master-plan.md Phase 4 -->

## Purpose

Complete CAN bus message definition: ID, sender, receiver, DLC, cycle time, signals, E2E protection.

## Message Table

| CAN ID | Name | Sender | Receiver(s) | DLC | Cycle (ms) | E2E | Signals |
|--------|------|--------|-------------|-----|-----------|-----|---------|
| 0x100 | — | CVC | FZC, RZC | — | 10 | Yes | — |

<!-- Target: 15+ CAN messages — see master-plan.md Phase 4 -->

## Signal Definitions

### 0x100: {Message Name}

| Signal | Start Bit | Length | Factor | Offset | Unit | Range |
|--------|-----------|--------|--------|--------|------|-------|
| — | — | — | — | — | — | — |

## E2E Protection Strategy

- CRC-8 per message
- Alive counter (4-bit, wrap at 15)
- Data ID per message
