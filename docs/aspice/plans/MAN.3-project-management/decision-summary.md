---
document_id: MAN3-DECISION-SUMMARY
title: "Decision Summary — Quick Reference"
version: "1.0"
status: active
updated: "2026-02-22"
---

# Decision Summary

Quick reference for all approved decisions. For full rationale, alternatives, and effort analysis, see [decision-log.md](decision-log.md).

---

## Hardware

| ADR | Decision | Cost | Time | Key Reason |
|-----|----------|------|------|------------|
| ADR-005 | **STM32G474RE Nucleo-64** for 3 zone ECUs (CVC, FZC, RZC) | $60 (3x $20) | 10-14 days BSW | 3x FDCAN, 5x ADC, CORDIC/FMAC, dominant automotive MCU family |
| ADR-011 | **CAN 2.0B at 500 kbps** (no CAN FD) | $0 | 0 days | TMS570 DCAN only supports classic CAN; 35% bus utilization — no bandwidth pressure |

## Software Architecture

| ADR | Decision | Cost | Time | Key Reason |
|-----|----------|------|------|------------|
| ADR-006 | **AUTOSAR Classic layered BSW** (MCAL, EAL, Services, RTE) | $0 | 10-14 days | 16+ automotive resume keywords, ASPICE SWE.2/SWE.3 compliant, 10/10 resume impact |
| ADR-007 | **POSIX SocketCAN** for simulated ECU MCAL | $0 | 5-8 days | 100% code reuse between physical and simulated ECUs |
| ADR-008 | **BMW vsomeip** for SOME/IP demo | $0 | 4-7 days | Industry-standard (1,400 stars), deployed in millions of BMW vehicles, direct API shows protocol knowledge |
| ADR-009 | **Docker containers** for simulated ECU runtime | $0 | 1-2 days | Isolation + reproducibility + CI/CD at same cost as native processes |

## Testing

| ADR | Decision | Cost | Time | Key Reason |
|-----|----------|------|------|------------|
| ADR-010 | **Unity + CCS Test + pytest** | $0 | 2-4 hours + 1-2 days | Pure C, MIT license, 5K stars, actively maintained, designed for embedded |

## Cloud & Infrastructure

| ADR | Decision | Cost | Time | Key Reason |
|-----|----------|------|------|------------|
| ADR-012 | **AWS IoT Core + Timestream + Grafana** | $4-7/month | 3-5 days | Highest automotive OEM adoption (Mercedes, VW, Toyota), free tier covers 500K msg/month |

## Process & Documentation

| ADR | Decision | Cost | Time | Key Reason |
|-----|----------|------|------|------------|
| ADR-001 | **ASPICE process area folder structure** | $0 | 2 hours | Assessor-friendly navigation, scales with project |
| ADR-002 | **master-plan.md as source baseline** | $0 | 0 hours | Single strategic source of truth; ASPICE plans reference it |
| ADR-003 | **Central docs/research/ repository** | $0 | 1 hour | Git-versioned research provenance |
| ADR-004 | **MAN.3 live tracking set** (dashboard, logs, gates) | $0 | 3 hours | ASPICE MAN.3 evidence requirements met in-repo |

---

## Totals

| Category | Total Cost | Total Dev Time |
|----------|-----------|----------------|
| Hardware (boards) | ~$60 | — |
| Cloud (monthly) | ~$4-7/month | — |
| Software/tools | $0 | — |
| Development effort | — | ~36-50 days |

## Rejected Alternatives (Top Reasons)

| Rejected | Why |
|----------|-----|
| STM32F446RE | No CAN-FD — legacy technology signal |
| Arduino Mega 2560 | 8-bit, 8KB RAM, no CAN — resume damage for ASIL D |
| Bare-metal (no AUTOSAR) | ASPICE SWE.2 non-compliant, 3/10 resume impact |
| FreeRTOS without AUTOSAR layers | No AUTOSAR keywords, 5/10 resume impact |
| Python-can for simulated ECUs | Zero code reuse with physical ECU firmware |
| Vector CANoe | $10K-$15K+ — prohibitive for portfolio project |
| Custom SOME/IP from scratch | 4-8 weeks wasted reimplementing existing solution |
| CommonAPI + vsomeip | 6-repo toolchain, doubles effort, hides protocol details |
| QEMU/Renode emulation | 3-15x more setup, STM32G474 not supported upstream |
| Google Test (gtest) | C++17 required, C/C++ boundary boilerplate unnecessary for pure C |
| CUnit | Abandoned since 2018, no mock generation |
| CAN FD (via TCAN4550) | 8-13 extra days for unneeded bandwidth, complicates Safety Controller |
| Ethernet + CAN hybrid | 6-10 weeks, requires replacing all MCU boards |
| Azure IoT Hub | $25/month minimum for 500K messages (5x AWS cost) |
| Self-hosted Mosquitto + InfluxDB | Loses "AWS IoT Core" automotive resume keyword |
