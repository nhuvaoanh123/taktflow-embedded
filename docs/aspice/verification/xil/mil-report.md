---
document_id: MIL-RPT
title: "MIL Test Report"
version: "0.1"
status: planned
aspice_process: SWE.5
---

# Model-in-the-Loop (MIL) Test Report

<!-- Phase 12 deliverable -->

## Purpose

Validate control algorithms using Python plant models (no firmware, no CAN).

## Plant Models

| Model | File | Parameters |
|-------|------|-----------|
| DC Motor | test/mil/plant_motor.py | J, B, Ke, Kt, R, L |
| Steering Servo | test/mil/plant_steering.py | Rate limit, saturation |
| Vehicle Dynamics | test/mil/plant_vehicle.py | Mass, drag, tire radius |

## Test Scenarios

| Scenario | Expected | Actual | Status |
|----------|----------|--------|--------|
| Normal operation | — | — | — |
| Overcurrent | — | — | — |
| Thermal runaway | — | — | — |
| Emergency brake | — | — | — |

## Results

<!-- To be completed in Phase 12 -->
