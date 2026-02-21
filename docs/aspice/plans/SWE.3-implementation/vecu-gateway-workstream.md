# SWE.3 Implementation Plan (vECU and Gateway)

Process area: SWE.3 Software Detailed Design and Unit Construction
Scope: phases 10 to 11

## Entry Criteria

- [ ] Physical ECU data contracts are stable
- [ ] POSIX CAN backend architecture approved
- [ ] Cloud and test account prerequisites available

## Detailed Work Breakdown

## VG1 vECU Runtime and Build Pipeline

- [ ] Implement `Can_Posix.c` with parity to STM32 CAN API
- [ ] Build Linux container toolchain and compose orchestration
- [ ] Implement BCM executable path and signal behavior
- [ ] Implement ICU executable path and dashboard behavior
- [ ] Implement TCU UDS and DTC storage behavior
- [ ] Validate CAN bridge for mixed real and simulated mode

## VG2 Edge Gateway Telemetry and ML

- [ ] Implement CAN listener and decoder pipeline
- [ ] Implement MQTT publisher and cloud schema
- [ ] Implement model inference pipeline for health and anomaly models
- [ ] Build alerting thresholds and dashboard visualization

## VG3 Quality Integration Demonstrator

- [ ] Implement SAP QM mock API and persistence layer
- [ ] Implement DTC to notification mapping
- [ ] Generate 8D template payload from DTC context
- [ ] Surface notification status in fault injector GUI

## Outputs

- [ ] 3 simulated ECU binaries and container definitions
- [ ] Operational gateway telemetry and ML services
- [ ] Operational QM mock integration flow

## Review Checklist (Gate G3)

- [ ] One command starts all vECU components
- [ ] Key UDS services respond correctly
- [ ] Faults propagate from CAN to cloud alerts
- [ ] Faults propagate from DTC to QM notification and 8D template
