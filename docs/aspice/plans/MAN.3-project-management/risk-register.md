---
document_id: RISK
title: "Risk Register"
version: "0.1"
status: draft
aspice_process: MAN.3
---

# Risk Register

## Purpose

Track project risks, their impact, and mitigation actions per ASPICE MAN.3.

## Risk Table

| Risk ID | Description | Probability | Impact | Mitigation | Status |
|---------|-------------|------------|--------|------------|--------|
| R-001 | TMS570 toolchain learning curve | Medium | 1-2 days lost | Start with LED toggle + CAN loopback. SC firmware is ~400 LOC. | Open |
| R-002 | CAN bit timing mismatch | Low | Bus doesn't work | Use online calculator. Test 2-node first. | Open |
| R-003 | AWS free tier exceeded | Low | Unexpected cost | Batch to 1 msg/5sec. Budget $3/month. | Open |
| R-004 | Docker + SocketCAN on Windows | Medium | Dev environment issues | Test WSL2 early. Fallback: develop on Pi. | Open |
| R-005 | vsomeip build complexity | Medium | Schedule | Pin version. Fallback: raw UDP SOME/IP. | Open |
| R-006 | FMEDA failure rates unavailable for STM32G474 | Medium | Completeness | Use generic Cortex-M4 rates. Flag assumption. | Open |
| R-007 | MIL plant model parameters | Medium | Accuracy | Buy motor early, measure if no datasheet. | Open |
| R-008 | 19.5 days is optimistic | High | Schedule | Track velocity after Phase 5. Accept stretch to 25-30 days. | Open |
| R-009 | AUTOSAR BSW over-engineering | Medium | Schedule | Keep to ~2,500 LOC. Demonstrate architecture, not production completeness. | Open |
| R-010 | Sensor wiring errors | Low | Debug time | Follow HSI spec. Test each sensor standalone. | Open |

## Risk Review

Last reviewed: —
Next review: —
