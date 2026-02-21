---
paths:
  - "firmware/**/*"
  - "scripts/**/*"
---

# Testing Standards

## Test-First Mandate (TDD)

**Hard rule — enforced by hook (`.claude/hooks/test-first.sh`)**

Every firmware source file MUST have its test file written BEFORE the implementation:

1. **Write the test file first**: `test_<module>.c` with Unity test stubs
2. **Each test function MUST have a `@verifies` tag** linking to a requirement ID (SSR, SWR, or HSR)
3. **Test stubs define the expected API** — function signatures, parameter types, return values
4. **Only then write the implementation** — the hook blocks `Edit`/`Write` on any `.c` source file under `firmware/*/src/` or `firmware/shared/bsw/` if the corresponding test file doesn't exist

### Test file locations

| Source pattern | Test file location |
|---|---|
| `firmware/<ecu>/src/<module>.c` | `firmware/<ecu>/test/test_<module>.c` |
| `firmware/shared/bsw/<layer>/<sub>/<module>.c` | `firmware/shared/bsw/test/test_<module>.c` |

### Minimum test stub content

```c
/**
 * @file    test_<module>.c
 * @brief   Unit tests for <module>
 * @verifies SWR-<ECU>-NNN, SSR-<ECU>-NNN
 */
#include "unity.h"
#include "<module>.h"

void setUp(void) { /* reset state before each test */ }
void tearDown(void) { /* cleanup after each test */ }

/** @verifies SWR-<ECU>-NNN */
void test_<module>_<function>_<nominal_scenario>(void) {
    TEST_ASSERT_EQUAL(expected, actual);
}

/** @verifies SWR-<ECU>-NNN */
void test_<module>_<function>_<error_scenario>(void) {
    TEST_ASSERT_EQUAL(ERR_INVALID_PARAM, result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_<module>_<function>_<nominal_scenario>);
    RUN_TEST(test_<module>_<function>_<error_scenario>);
    return UNITY_END();
}
```

### Rationale

- ISO 26262 Part 6, Table 9: Requirements-based testing is highly recommended at ASIL D
- Tests written first guarantee every function traces to a requirement before code exists
- Catches API design issues early — test stubs define the contract
- ASPICE SWE.4: Unit verification must be planned before unit construction

### Exemptions

Files NOT subject to test-first (no hook enforcement):
- Header files (`.h`) — tested via their `.c` implementation
- Config files (`firmware/*/cfg/`) — static configuration, not executable
- Test files themselves (`firmware/*/test/`) — always allowed

## Test Categories

| Category | Scope | Runs On | When |
|----------|-------|---------|------|
| Unit tests | Individual functions/modules | Host machine (x86) | Every commit |
| Integration tests | Module interactions | Host or target hardware | Every PR |
| Hardware-in-the-loop (HIL) | Full system with real peripherals | Target hardware | Before release |
| Regression tests | Previously fixed bugs | Host machine | Every commit |
| Security tests | Attack vectors, fuzzing | Host machine | Every PR |
| Performance tests | Timing, memory usage | Target hardware | Before release |

## Unit Test Rules

- Every module (`module.c`) MUST have a corresponding test file (`test_module.c`)
- Test file location: `firmware/<ecu>/test/test_<module>.c` (ECU modules) or `firmware/shared/bsw/test/test_<module>.c` (BSW modules)
- Use a test framework (Unity, CMock, or project-selected alternative)
- Tests must be runnable on host machine — mock hardware dependencies
- Test the boundary conditions: zero, max, off-by-one, NULL, empty
- Test error paths — not just the happy path
- Each test function tests one behavior — name it: `test_<function>_<scenario>`

## Test Naming Convention

```
test_<module>_<function>_<scenario>

Examples:
  test_uart_parse_valid_command
  test_uart_parse_oversized_input_rejects
  test_mqtt_connect_invalid_cert_fails
  test_ota_verify_bad_signature_rejects
  test_sensor_read_out_of_range_clamps
```

## What MUST Be Tested

- All input validation functions (every boundary point from input-validation rules)
- All state machine transitions
- All error handling paths
- Cryptographic operations (known test vectors)
- Protocol parsers (valid, malformed, oversized, empty input)
- Configuration loading (missing fields, corrupt data, defaults)
- OTA update verification (valid signature, invalid, truncated, rollback attempt)

## What SHOULD Be Tested (when practical)

- Timing-critical code (interrupt handlers, debounce logic)
- Power state transitions
- Peripheral initialization sequences
- Queue full/empty conditions
- Memory allocation failure paths

## Mocking Strategy

- Mock hardware at the HAL boundary — test application logic against mock HAL
- Mock network at the transport layer — test protocol logic without real connections
- Mock time functions for timeout/timer testing
- Do NOT mock the code under test — only its dependencies

## Test Infrastructure

- Tests must compile and run with a single command: `make test` or equivalent
- Test results must output in standard format (TAP, JUnit XML, or similar)
- Failed tests must produce clear error messages with expected vs actual values
- Test suite must complete in under 60 seconds on host machine
- CI must fail the build on any test failure

## Coverage Goals

| ASIL | Statement | Branch | MC/DC |
|------|-----------|--------|-------|
| QM | >= 80% | >= 60% | -- |
| A-B | >= 90% | >= 80% | -- |
| C | >= 95% | >= 90% | >= 80% |
| **D** | **100%** | **100%** | **100%** |

- ASIL D safety paths: 100% MC/DC mandatory (ISO 26262 Part 6, Table 12)
- 100% coverage for security-critical code (crypto, auth, input validation, OTA)
- Coverage reports generated on every CI run
- New code must not decrease overall coverage percentage

## Regression Tests

- Every bug fix MUST include a regression test that reproduces the original bug
- Name regression tests clearly: `test_regression_issue_<number>_<description>`
- Regression tests should never be deleted (even if the code changes)

## Fuzzing (Security)

- Fuzz all protocol parsers (MQTT, BLE, HTTP, serial command parser)
- Fuzz all input validation functions
- Use AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan) during fuzz runs
- Run fuzz campaigns for minimum 1 hour per target before release
- Document and fix all crashes found by fuzzing
