---
document_id: VECU-ARCH
title: "Virtual ECU Architecture"
version: "0.1"
status: planned
aspice_process: SWE.2
---

# Virtual ECU Architecture

<!-- Phase 10 deliverable -->

## Purpose

Architecture for simulated ECUs (BCM, ICU, TCU) running in Docker containers.

## Design Principle

Same C source code as physical ECUs, compiled for Linux with POSIX MCAL backend.

## Platform Abstraction

```
Application SWCs (same code)
    ↕
RTE (same code)
    ↕
BSW Services (same code)
    ↕
MCAL — Can_Posix.c (SocketCAN backend)
    ↕
Linux SocketCAN (vcan0 or real CAN via CANable)
```

## Runtime Modes

| Mode | CAN Interface | Use Case |
|------|--------------|----------|
| vcan | Virtual CAN (vcan0) | CI/CD, SIL testing |
| bridge | Real CAN via CANable | Mixed HW+SW testing |

## Docker Architecture

- Base image: Ubuntu + build-essential + can-utils
- One container per simulated ECU
- docker-compose orchestration
