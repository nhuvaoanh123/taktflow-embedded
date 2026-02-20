---
paths:
  - "firmware/test/**/*"
  - "firmware/src/**/*"
  - "scripts/**/*"
---

# Testing Standards

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
- Test file location: `firmware/test/test_<module>.c`
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

- Minimum 80% line coverage for application code (`firmware/src/`)
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
