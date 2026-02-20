---
paths:
  - "firmware/**/*"
  - "hardware/**/*"
---

# ASIL D System Architecture Constraints (ISO 26262 Part 4)

## Freedom from Interference (FFI)

FFI = absence of cascading failures between elements that could violate a safety goal.

### Three Types of Interference

| Type | Description | Mitigation |
|------|-------------|------------|
| **Spatial (memory)** | Low-ASIL component corrupts high-ASIL memory | MPU/MMU hardware isolation, separate address spaces |
| **Temporal (timing)** | Low-priority task steals CPU from safety-critical task | Time-partitioned RTOS, WCET budgets, execution monitoring |
| **Communication** | Corrupt/delayed messages between ASIL levels | Data validation, CRC, sequence counters, E2E protection |

### FFI Demonstration Methods

1. **Hardware separation** (strongest): Separate microcontrollers for different ASIL levels
2. **Hardware partitioning**: MPU/MMU enforced memory regions on same MCU
3. **Temporal partitioning**: Safety-qualified RTOS with time-partitioned scheduling
4. **Monitoring/validation**: Higher-ASIL element validates ALL data from lower-ASIL before use
5. **Static analysis**: Prove no control/data flow from low-ASIL to high-ASIL without protection
6. **Fault injection testing**: Inject faults in low-ASIL element, verify high-ASIL unaffected

**ASIL D rule**: If full separation cannot be achieved, the lower-ASIL element MUST be developed to the higher ASIL level (ASIL escalation).

## Timing Architecture

### Fault Tolerant Time Interval (FTTI)

```
FTTI = FDTI + FRTI

FDTI = Fault Detection Time Interval (occurrence -> detection)
FRTI = Fault Reaction Time Interval (detection -> safe state)
```

**Constraint**: FDTI + FRTI MUST be less than FTTI (system-specific, from HARA).

Typical values:
- Processor faults: FTTI in low milliseconds
- Steering/braking: FTTI 50-200ms depending on speed/scenario
- NXP ASIL D reference: Fault Reaction Time <= 50ms

### Timing Requirements

- All safety functions must have defined execution time budgets
- WCET analysis mandatory for every safety-critical task
- Jitter must be bounded and accounted for
- Interrupt latency must be analyzed and bounded
- Task scheduling must be deterministic (no priority inversion without analysis)

## Communication Safety (AUTOSAR E2E)

End-to-End protection detects these communication faults:

| Fault Type | Description | Detection Method |
|-----------|-------------|------------------|
| Repetition | Same message received twice | Sequence counter |
| Loss | Message not received | Sequence counter + timeout |
| Delay | Message received too late | Timeout monitoring |
| Insertion | Spurious message injected | Data ID + CRC |
| Masquerade | Message from wrong sender | Data ID + CRC |
| Corruption | Data content altered | CRC (8/32/64-bit) |
| Asymmetric info | Different receivers get different data | CRC + sequence counter |

### E2E Protection Mechanisms

- **CRC**: Over payload — 8-bit (CAN 2.0), 32-bit or 64-bit (CAN FD, Ethernet)
- **Sequence counter**: 4-bit (CAN) or 8-bit (CAN FD) alive counter
- **Data ID**: Unique identifier per data element
- **Timeout**: Watchdog on message arrival period

Apply E2E protection to ALL safety-critical data transfers, including:
- Inter-ECU communication (CAN, LIN, Ethernet)
- Intra-ECU communication (inter-partition, shared memory)
- Sensor readings
- Actuator commands

## Fail-Safe vs Fail-Operational

### Fail-Safe (standard ASIL D pattern)
- System detects fault and transitions to defined safe state
- Safe state: motor de-energized, brakes applied, output disabled
- Used when "off" is an acceptable safe state
- Architecture: 1oo2D minimum

### Fail-Operational (required for autonomous driving L3+)
- System continues operating in degraded mode after fault
- No single fault causes complete loss of function
- Used when "off" is NOT a safe state (steer-by-wire, brake-by-wire)
- Architecture: 2oo3 minimum with majority voting
- Requires at least two independent processing paths

## Safe State Requirements

Every safety goal MUST have at least one defined safe state:

- **Precisely defined**: "De-energize steering assist motor" not "enter safe mode"
- **Reachable**: From any operating mode within the FTTI
- **Independent**: Achievable without failed component's cooperation
- **Verified**: Tested via fault injection that safe state is actually reached

## System Design Verification at ASIL D

| Method | ASIL D |
|--------|--------|
| Walk-through | o (not sufficient alone) |
| Inspection | ++ (highly recommended) |
| Semi-formal verification | ++ (highly recommended) |
| **Formal verification** | **++ (highly recommended)** |
| Simulation/prototyping | ++ (highly recommended) |

**ASIL D is the only level where formal verification of system design is highly recommended.**

## Mixed-Criticality Systems

When components of different ASIL levels coexist on the same hardware:

1. All software treated at the highest ASIL UNLESS freedom from interference is demonstrated
2. FFI must address all three interference types (spatial, temporal, communication)
3. Safety-qualified hypervisor or RTOS required for partitioning
4. Each partition's ASIL level documented in the safety architecture
5. Interface between partitions is a safety-critical element — analyze and test independently
