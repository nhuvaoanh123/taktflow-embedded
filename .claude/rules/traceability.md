---
paths:
  - "firmware/**/*"
  - "hardware/**/*"
  - "docs/safety/**/*"
  - "docs/aspice/**/*"
---

# Traceability Requirements (ISO 26262 + ASPICE)

**Bidirectional traceability** across the full V-model. Every requirement traces to implementation and tests. Every test traces back to a requirement.

## Traceability Chain

```
Stakeholder Reqs (SYS.1) ↔ System Reqs (SYS.2) ↔ System Architecture (SYS.3)
  ↔ SW Reqs (SWE.1) ↔ SW Architecture (SWE.2) ↔ Detailed Design (SWE.3)
  ↔ Source Code ↔ Unit Tests (SWE.4) ↔ Integration Tests (SWE.5)
  ↔ SW Verification (SWE.6) ↔ System Tests (SYS.4/5)
```

## Code Annotations

```c
// Source files:
/** @safety_req SSR-042  @traces_to TSR-018, SG-003  @verified_by TC-MQTT-012 */

// Test files:
/** @test_case TC-MQTT-012  @verifies SSR-042  @method Boundary value analysis  @coverage MC/DC */
```

## ASIL D Rules

- **Complete**: No orphan requirements, no untested code
- **Verified**: Independently reviewed
- **Maintained**: Updated with every change
- **Tool-supported**: DOORS, Polarion, Jama, or similar

## Traceability Matrix

| Safety Goal | FSR | TSR | SSR/HSR | Source File:Function | Test Case | Result | Status |
|-------------|-----|-----|---------|---------------------|-----------|--------|--------|

Every row must be complete for release. Any gap = compliance finding.
