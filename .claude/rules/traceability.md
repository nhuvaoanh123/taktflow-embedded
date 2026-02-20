---
paths:
  - "**/*"
---

# Traceability Requirements (ISO 26262 + ASPICE)

## Core Requirement

**Bidirectional traceability** must be maintained across the entire V-model development lifecycle. Every requirement must trace down to implementation and tests. Every test must trace back to a requirement.

## Full Traceability Chain

```
Stakeholder Requirements (SYS.1)
    | bidirectional
    v
System Requirements (SYS.2)
    | bidirectional
    v
System Architecture Elements (SYS.3)
    | bidirectional
    v
Software Requirements (SWE.1)
    | bidirectional
    v
Software Architecture Components (SWE.2)
    | bidirectional
    v
Software Detailed Design (SWE.3)
    | bidirectional
    v
Source Code / Software Units (SWE.3)
    | bidirectional
    v
Unit Test Results (SWE.4)
    | bidirectional
    v
Integration Test Results (SWE.5)
    | bidirectional
    v
SW Verification Test Results (SWE.6)
    | bidirectional
    v
System Integration Test Results (SYS.4)
    | bidirectional
    v
System Verification Test Results (SYS.5)
```

## Traceability per ASPICE Process

| Process | Traceability Required |
|---------|----------------------|
| SWE.2 | SW requirements <-> SW architecture |
| SWE.3 | Architecture <-> detailed design <-> source code; SW requirements <-> source code |
| SWE.4 | Source code <-> static analysis; detailed design <-> unit test specs/results |
| SWE.5 | Architecture <-> integration test specs/results |
| SWE.6 | SW requirements <-> verification test specs/results |

## Safety-Specific Traceability (ISO 26262)

| From | To | Direction |
|------|----|-----------|
| Safety goals | Functional safety requirements | Bidirectional |
| Functional safety requirements | Technical safety requirements | Bidirectional |
| Technical safety requirements | HW safety requirements | Bidirectional |
| Technical safety requirements | SW safety requirements | Bidirectional |
| SW safety requirements | SW architecture | Bidirectional |
| SW architecture | SW units (source code) | Bidirectional |
| SW safety requirements | Test cases | Bidirectional |
| Test cases | Test results | Bidirectional |

## What Traceability Ensures

- **Complete coverage**: Every requirement is implemented and tested
- **No superfluous code**: Every unit traces back to a requirement
- **Impact analysis**: When a requirement changes, all affected artifacts identified
- **Consistency**: Requirements, design, code, and tests stay aligned
- **Audit readiness**: Assessors can verify completeness at any time

## Traceability Gap = Compliance Failure

At ASIL D, traceability must be:
- **Complete**: No orphan requirements, no untested code
- **Verified**: Independently reviewed for correctness
- **Maintained**: Updated with every change (not just at milestones)
- **Tool-supported**: Use requirements management tool (DOORS, Polarion, Jama, or similar)

## Implementation in This Project

### For Code Files
```c
/**
 * @safety_req SSR-042: Validate MQTT payload before processing
 * @traces_to TSR-018, SG-003
 * @verified_by TC-MQTT-012, TC-MQTT-013
 */
```

### For Test Files
```c
/**
 * @test_case TC-MQTT-012
 * @verifies SSR-042
 * @method Requirements-based testing, boundary value analysis
 * @coverage Branch, MC/DC
 */
void test_mqtt_validate_payload_oversized(void) { ... }
```

### For Requirements (docs/requirements/)
```
SSR-042: MQTT payload validation
  ASIL: D
  Traces up: TSR-018
  Traces down: mqtt_handler.c:mqtt_validate_payload()
  Verified by: TC-MQTT-012, TC-MQTT-013
  Status: Implemented, Verified
```

## Traceability Matrix

Maintain a traceability matrix (spreadsheet, tool export, or generated report) showing:

| Safety Goal | FSR | TSR | SSR/HSR | Source File:Function | Test Case | Test Result | Status |
|-------------|-----|-----|---------|---------------------|-----------|-------------|--------|
| SG-001 | FSR-001 | TSR-001 | SSR-001 | module.c:func() | TC-001 | PASS | Complete |

Every row must be complete for a release. Any gap = compliance finding.
