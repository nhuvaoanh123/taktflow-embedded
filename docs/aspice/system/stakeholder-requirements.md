---
document_id: STKR
title: "Stakeholder Requirements"
version: "1.0"
status: draft
aspice_process: SYS.1
date: 2026-02-21
---

# Stakeholder Requirements

## 1. Purpose

This document captures the stakeholder needs and expectations for the Taktflow Zonal Vehicle Platform per Automotive SPICE 4.0 SYS.1 (Requirements Elicitation). It translates stakeholder intentions into structured, verifiable requirements that serve as the basis for system requirements analysis (SYS.2).

The stakeholder requirements define WHAT the stakeholders want the system to achieve, without prescribing HOW. They are expressed in stakeholder language and are traceable to the system requirements that refine them.

## 2. Stakeholders

| ID | Stakeholder | Role | Interest |
|----|-------------|------|----------|
| SH-01 | Portfolio Reviewer | Evaluator | Assesses ISO 26262, ASPICE, and AUTOSAR competence through delivered work products |
| SH-02 | Automotive Hiring Manager | Decision Maker | Evaluates candidate's ability to perform as a Functional Safety Engineer or Embedded Systems Engineer |
| SH-03 | System Integrator | Technical | Requires clear interfaces, testable requirements, and reproducible build and demo processes |
| SH-04 | Vehicle Operator (Demo) | End User | Operates the bench platform during demonstrations; expects predictable behavior and clear feedback |
| SH-05 | Safety Assessor | Auditor | Reviews safety work products for completeness, consistency, and ISO 26262 compliance |
| SH-06 | Service Technician | Maintenance | Diagnoses faults using UDS tools, reads/clears DTCs, and validates system health |

## 3. Referenced Documents

| Document ID | Title | Version |
|-------------|-------|---------|
| ITEM-DEF | Item Definition | 1.0 |
| HARA | Hazard Analysis and Risk Assessment | 1.0 |
| SG | Safety Goals | 1.0 |
| FSR | Functional Safety Requirements | 1.0 |
| FSC | Functional Safety Concept | 1.0 |
| SYSREQ | System Requirements Specification | 1.0 |

## 4. Requirement Conventions

Each requirement follows this structure:

- **ID**: STK-NNN (sequential)
- **Title**: Descriptive name
- **Priority**: Must (mandatory for core demo), Should (expected for completeness), May (desirable enhancement)
- **Source**: Stakeholder ID (SH-01 through SH-06)
- **Status**: draft | reviewed | approved
- **Description**: "Shall" language defining the need

---

## 5. Portfolio Demonstration Requirements

### STK-001: ISO 26262 Full Safety Lifecycle Demonstration

- **Priority**: Must
- **Source**: SH-01, SH-02
- **Status**: draft

The platform shall demonstrate competence with the full ISO 26262 safety lifecycle from item definition (Part 3) through safety validation (Part 4), including HARA, safety goals, functional safety concept, technical safety concept, software and hardware safety requirements, safety analyses (FMEA, DFA), and a structured safety case. All work products shall be structured to the level of completeness expected in a production ASIL D project.

---

### STK-002: ASPICE Level 2 Process Maturity Demonstration

- **Priority**: Must
- **Source**: SH-01, SH-05
- **Status**: draft

The platform shall demonstrate Automotive SPICE Level 2 process maturity across the system engineering (SYS.1 through SYS.5), software engineering (SWE.1 through SWE.6), and support (SUP.1, SUP.8) process areas. Each process area shall produce the defined work products with bidirectional traceability between requirements, architecture, implementation, and verification.

---

### STK-003: AUTOSAR-like BSW Architecture Demonstration

- **Priority**: Must
- **Source**: SH-01, SH-02
- **Status**: draft

The platform shall implement an AUTOSAR Classic-like layered basic software (BSW) architecture with the following modules: MCAL (CAN, SPI, ADC, PWM, Dio, Gpt), ECU Abstraction (CanIf, PduR, IoHwAb), Services (Com, Dcm, Dem, WdgM, BswM, E2E), and RTE. The architecture shall demonstrate the separation of concerns between application software components and BSW modules.

---

### STK-004: Diverse Redundancy and Multi-Vendor Safety Architecture

- **Priority**: Must
- **Source**: SH-01, SH-02, SH-05
- **Status**: draft

The platform shall demonstrate a diverse redundancy architecture using at least two different MCU vendors for safety-critical and zone control functions. The independent safety monitoring function shall use a lockstep CPU from a different vendor than the zone controllers to provide hardware-diverse fault detection.

---

## 6. Functional Requirements

### STK-005: Drive-by-Wire Function

- **Priority**: Must
- **Source**: SH-04, SH-03
- **Status**: draft

The platform shall provide a drive-by-wire function that converts operator pedal input into proportional motor torque. The pedal input shall be sensed using dual redundant sensors, and the system shall detect disagreement between the two sensors and transition to a safe state when plausibility checks fail.

---

### STK-006: Steering Control Function

- **Priority**: Must
- **Source**: SH-04, SH-03
- **Status**: draft

The platform shall provide a steering control function that translates steering commands into proportional steering servo angle. The system shall provide position feedback, rate limiting, and angle limiting. On detection of a steering fault, the system shall return the steering to the center position in a controlled manner.

---

### STK-007: Braking Control Function

- **Priority**: Must
- **Source**: SH-04, SH-03
- **Status**: draft

The platform shall provide a braking control function that applies proportional braking force via a brake servo actuator. The system shall autonomously apply braking in the event of CAN communication loss (fail-safe default). Emergency braking shall be triggered automatically by the obstacle detection system.

---

### STK-008: Forward Obstacle Detection Function

- **Priority**: Must
- **Source**: SH-04, SH-03
- **Status**: draft

The platform shall detect obstacles in the forward path using a lidar sensor and implement a graduated response with at least three distance thresholds: audible warning, automatic speed reduction, and emergency braking. The system shall detect lidar sensor faults and substitute a safe default (obstacle assumed present).

---

### STK-009: Independent Safety Monitoring Function

- **Priority**: Must
- **Source**: SH-04, SH-05, SH-01
- **Status**: draft

The platform shall include an independent safety monitoring function, implemented on a separate ECU from the zone controllers, that monitors all zone ECU heartbeats, performs cross-plausibility checks on safety-critical signals, and controls a hardware kill relay to force the system into a safe state when a safety violation is detected. The monitoring ECU shall operate in CAN listen-only mode.

---

### STK-010: Emergency Stop Function

- **Priority**: Must
- **Source**: SH-04, SH-03
- **Status**: draft

The platform shall provide a hardware emergency stop button that, when activated, immediately commands all actuators to their safe state (motor off, brakes applied, steering centered) and latches the system in a safe stop condition until a manual restart is performed.

---

### STK-011: Body Control Function

- **Priority**: Should
- **Source**: SH-01, SH-04
- **Status**: draft

The platform shall provide body control functions including automatic headlight activation based on driving state, turn indicator control, and hazard light activation during fault or emergency conditions. Body control functions shall be QM-rated and shall not interfere with safety-critical functions.

---

### STK-012: UDS Diagnostic Services

- **Priority**: Must
- **Source**: SH-06, SH-01
- **Status**: draft

The platform shall implement Unified Diagnostic Services (UDS) per ISO 14229, supporting at least the following services: Diagnostic Session Control (0x10), Clear Diagnostic Information (0x14), Read DTC Information (0x19), Read Data By Identifier (0x22), Write Data By Identifier (0x2E), and Security Access (0x27). The diagnostic function shall enable fault reading, clearing, and data inspection by a service technician using standard UDS tooling.

---

### STK-013: Diagnostic Trouble Code Management

- **Priority**: Must
- **Source**: SH-06, SH-01
- **Status**: draft

The platform shall store diagnostic trouble codes (DTCs) with associated freeze-frame data for all detected faults. DTCs shall be persistent across power cycles (stored in non-volatile memory). The system shall support reading, clearing, and aging of DTCs per standard diagnostic conventions.

---

### STK-014: Cloud Telemetry and Dashboarding

- **Priority**: Should
- **Source**: SH-01, SH-03
- **Status**: draft

The platform shall transmit vehicle telemetry data (speed, torque, temperature, current, sensor health, DTCs) to a cloud endpoint over a secure (TLS-encrypted) connection. A cloud dashboard shall provide real-time and historical visualization of vehicle operating data and fault events.

---

### STK-015: Edge ML Anomaly Detection

- **Priority**: Should
- **Source**: SH-01, SH-03
- **Status**: draft

The platform shall include an edge machine learning inference capability that detects anomalous patterns in motor current, temperature, and CAN bus traffic. Anomaly alerts shall be forwarded to the cloud dashboard and shall not be used for safety-critical decisions.

---

## 7. Safety Requirements

### STK-016: ASIL D Compliance for Drive-by-Wire, Steering, and Braking

- **Priority**: Must
- **Source**: SH-05, SH-01
- **Status**: draft

The drive-by-wire (acceleration control), steering, and braking functions shall be developed to ASIL D integrity per ISO 26262. All safety mechanisms, diagnostic coverage, and verification activities for these functions shall meet ASIL D requirements including MC/DC code coverage, independent verification, and formal methods where applicable.

---

### STK-017: Defined Safe States for All Safety Goals

- **Priority**: Must
- **Source**: SH-05, SH-01
- **Status**: draft

The platform shall define and implement at least one safe state for each identified safety goal. Safe states shall include, at minimum: motor off with brakes applied, steering return to center, controlled stop with gradual deceleration, and full system shutdown via hardware kill relay. Each safe state shall be achievable within the defined Fault Tolerant Time Interval.

---

### STK-018: Fault Tolerant Time Interval Compliance

- **Priority**: Must
- **Source**: SH-05, SH-01
- **Status**: draft

The platform shall detect safety-relevant faults and achieve the corresponding safe state within the Fault Tolerant Time Interval defined for each safety goal. The FTTI budget shall account for detection time, reaction time, actuation time, and a safety margin. FTTI values shall be justified by physical analysis of the hazard progression.

---

### STK-019: Independent Safety Monitoring with Kill Relay

- **Priority**: Must
- **Source**: SH-05, SH-01, SH-04
- **Status**: draft

The platform shall include a hardware-enforced safety boundary (kill relay) controlled by the independent Safety Controller. The relay shall use an energize-to-run configuration such that any failure of the Safety Controller inherently results in power removal from all safety-critical actuators. The kill relay shall be the ultimate safety enforcement mechanism, independent of the zone controller software.

---

### STK-020: E2E Protection on Safety-Critical CAN Messages

- **Priority**: Must
- **Source**: SH-05, SH-03
- **Status**: draft

All safety-critical CAN messages shall be protected with end-to-end (E2E) data protection including CRC, alive counter, and data identification. The E2E mechanism shall detect message corruption, repetition, loss, delay, insertion, and masquerading. On E2E verification failure, the receiver shall substitute safe default values.

---

## 8. Performance Requirements

### STK-021: Control Loop Cycle Time

- **Priority**: Must
- **Source**: SH-03, SH-05
- **Status**: draft

The platform shall execute the main safety-critical control loop (pedal reading, torque calculation, CAN transmission) at a fixed period of 10 ms or faster. The control loop execution time shall not exceed 80% of the cycle period to provide scheduling margin.

---

### STK-022: CAN Bus Timing and Throughput

- **Priority**: Must
- **Source**: SH-03, SH-05
- **Status**: draft

The CAN bus shall operate at 500 kbps and shall support the transmission of all defined messages within their specified cycle times. The bus utilization shall not exceed 60% under normal operation to provide margin for burst traffic and retransmissions. All safety-critical messages shall have CAN IDs assigned by priority (lower ID = higher priority = safety-critical).

---

### STK-023: Sensor Update Rates

- **Priority**: Must
- **Source**: SH-03
- **Status**: draft

The platform shall read all safety-critical sensors at their specified update rates: pedal position sensors at 100 Hz or faster, steering angle sensor at 100 Hz or faster, lidar sensor at 100 Hz (sensor native rate), motor current at 1 kHz or faster, and motor temperature at 10 Hz or faster. Sensor data shall be available to the application within one control cycle of acquisition.

---

### STK-024: Safe State Transition Times

- **Priority**: Must
- **Source**: SH-05, SH-03
- **Status**: draft

The platform shall achieve safe state transitions within the following maximum times from fault detection: motor off within 15 ms (for ASIL D pedal and brake faults), steering return to center within 100 ms (for ASIL D steering faults), controlled stop initiation within 100 ms (for ASIL B/C faults), and system shutdown (kill relay open) within 10 ms of Safety Controller decision.

---

## 9. Usability Requirements

### STK-025: Demonstration Scenario Coverage

- **Priority**: Must
- **Source**: SH-01, SH-04
- **Status**: draft

The platform shall support at least 12 distinct demonstration scenarios covering normal operation, sensor faults (pedal disagreement, pedal failure, steering fault, lidar detection), actuator faults (motor overcurrent, motor overtemperature), communication faults (CAN bus loss), safety monitoring (ECU hang detection, SC intervention), and operator interaction (E-stop, ML anomaly alert, CVC vs SC disagreement). Each scenario shall be triggerable and observable without requiring code changes.

---

### STK-026: Visual and Audible Operator Feedback

- **Priority**: Must
- **Source**: SH-04, SH-03
- **Status**: draft

The platform shall provide operator feedback through at least three independent channels: a display showing current operating state and fault information, an audible warning device with distinct patterns for different severity levels, and fault indicator LEDs that are independent of the CAN bus. At least one feedback channel shall remain operational even during total CAN bus failure.

---

### STK-027: Reproducible Build and Demo Process

- **Priority**: Should
- **Source**: SH-03, SH-01
- **Status**: draft

The platform shall be buildable from source using a documented, reproducible build process (Makefile or equivalent). The simulated ECUs shall be deployable via Docker containers. The demo setup shall be documented with step-by-step instructions covering hardware connections, firmware flashing, Docker startup, and scenario execution.

---

## 10. Regulatory and Standards Requirements

### STK-028: ISO 26262:2018 Compliance

- **Priority**: Must
- **Source**: SH-05, SH-01
- **Status**: draft

The platform development shall comply with ISO 26262:2018 across all applicable parts (Parts 2 through 9). All safety work products shall be structured per the standard's requirements for ASIL D. While formal third-party certification is not pursued (portfolio project), all deliverables shall be assessor-ready.

---

### STK-029: ASPICE Level 2 Minimum Process Maturity

- **Priority**: Must
- **Source**: SH-01, SH-05
- **Status**: draft

The platform development process shall meet Automotive SPICE 4.0 Capability Level 2 (Managed) as a minimum for all assessed process areas. Safety-critical processes (SYS, SWE) shall target Level 3 (Established). Each process area shall produce its defined information items (work products) with evidence of planning, monitoring, and control.

---

### STK-030: MISRA C:2012/2023 Coding Compliance

- **Priority**: Must
- **Source**: SH-05, SH-01
- **Status**: draft

All firmware source code shall comply with MISRA C:2012 (with Amendment 2 / 2023 updates) as required by ISO 26262-6 for ASIL D software. Deviations from mandatory MISRA rules shall follow a formal deviation process including justification, risk assessment, and approval. Static analysis shall enforce MISRA compliance as part of the build process.

---

### STK-031: Bidirectional Traceability

- **Priority**: Must
- **Source**: SH-05, SH-01
- **Status**: draft

The platform shall maintain bidirectional traceability across the full V-model: stakeholder requirements to system requirements, system requirements to architecture elements, architecture to software requirements, software requirements to source code and unit tests, and up through integration and system verification. Every requirement shall trace down to implementation and test. Every test shall trace back to a requirement.

---

### STK-032: SAP QM Integration Demonstration

- **Priority**: May
- **Source**: SH-01
- **Status**: draft

The platform shall demonstrate integration with SAP Quality Management (QM) processes by routing diagnostic trouble codes from the vehicle to a mock SAP QM endpoint, triggering the creation of a quality notification (Q-Meldung) and an automated 8D report. This demonstrates automotive industry awareness of quality management workflows.

---

## 6. Traceability

### 6.1 Stakeholder to Source Mapping

| Requirement | SH-01 | SH-02 | SH-03 | SH-04 | SH-05 | SH-06 |
|-------------|-------|-------|-------|-------|-------|-------|
| STK-001 | X | X | | | | |
| STK-002 | X | | | | X | |
| STK-003 | X | X | | | | |
| STK-004 | X | X | | | X | |
| STK-005 | | | X | X | | |
| STK-006 | | | X | X | | |
| STK-007 | | | X | X | | |
| STK-008 | | | X | X | | |
| STK-009 | X | | | X | X | |
| STK-010 | | | X | X | | |
| STK-011 | X | | | X | | |
| STK-012 | X | | | | | X |
| STK-013 | X | | | | | X |
| STK-014 | X | | X | | | |
| STK-015 | X | | X | | | |
| STK-016 | X | | | | X | |
| STK-017 | X | | | | X | |
| STK-018 | X | | | | X | |
| STK-019 | X | | | X | X | |
| STK-020 | | | X | | X | |
| STK-021 | | | X | | X | |
| STK-022 | | | X | | X | |
| STK-023 | | | X | | | |
| STK-024 | | | X | | X | |
| STK-025 | X | | | X | | |
| STK-026 | | | X | X | | |
| STK-027 | X | | X | | | |
| STK-028 | X | | | | X | |
| STK-029 | X | | | | X | |
| STK-030 | X | | | | X | |
| STK-031 | X | | | | X | |
| STK-032 | X | | | | | |

### 6.2 Downstream Traceability (STK to SYS)

Bidirectional traceability from stakeholder requirements to system requirements is maintained in the System Requirements Specification (SYSREQ) and summarized in the traceability matrix at the end of that document.

## 7. Open Items

| ID | Item | Owner | Target |
|----|------|-------|--------|
| STK-O-001 | Validate all STK requirements with full system requirements derivation (SYS.2) | System Engineer | SYS.2 phase |
| STK-O-002 | Review STK-025 demo scenario list against master plan scenario definitions | Project Manager | Before SYS.2 |
| STK-O-003 | Confirm STK-032 SAP QM scope with project stakeholders | Project Manager | Phase 0 |

## 8. Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 0.1 | 2026-02-21 | System | Initial stub (planned status) |
| 1.0 | 2026-02-21 | System | Complete stakeholder requirements: 32 requirements (STK-001 to STK-032), 6 stakeholders, traceability matrix |
