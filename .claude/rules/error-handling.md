---
paths:
  - "firmware/src/**/*.c"
  - "firmware/src/**/*.cpp"
  - "firmware/src/**/*.h"
---

# Error Handling Standards

## Core Principle: Fail-Closed, Not Fail-Open

Every error condition must result in a safe, defined state. Never silently continue.

## Return Value Discipline

- Every function that can fail MUST return an error code or status
- Check return values of EVERY function call — no exceptions
- Use a project-wide error code enum, not magic numbers
- Document possible error codes in function header comments

```c
// Project error code pattern:
typedef enum {
    ERR_OK = 0,
    ERR_INVALID_PARAM,
    ERR_TIMEOUT,
    ERR_AUTH_FAILED,
    ERR_BUFFER_OVERFLOW,
    ERR_HARDWARE_FAULT,
    ERR_OUT_OF_MEMORY,
    ERR_NOT_INITIALIZED,
    ERR_BUSY,
    ERR_UNKNOWN
} error_t;
```

## Error Propagation

- Propagate errors UP the call stack — don't swallow them
- Each layer may translate errors (HAL error -> module error -> app error)
- Log the original error at the point of detection
- Return a meaningful error to the caller

## Never Do This

```c
// BANNED: silently ignoring errors
result = send_data(buf, len);
// continues without checking result

// BANNED: catch-all with no action
if (result != ERR_OK) {
    // TODO: handle error
}

// BANNED: empty error handler
if (result != ERR_OK) {}
```

## Always Do This

```c
// REQUIRED: check and handle every error
result = send_data(buf, len);
if (result != ERR_OK) {
    log_error("send_data failed: %d", result);
    return result;  // propagate to caller
}
```

## Logging Standards

- Log at point of error detection (not at every propagation level)
- Include context: function name, error code, relevant parameters
- Use severity levels: ERROR, WARN, INFO, DEBUG
- ERROR = requires intervention or indicates failure
- WARN = recoverable issue, but unusual
- INFO = normal operation milestones
- DEBUG = development diagnostics (stripped from production)
- Never log sensitive data (credentials, keys, PII)

## Safe States

Define a safe state for every subsystem:

| Subsystem | Safe State |
|-----------|------------|
| Motor/actuator | STOP / de-energize |
| Network | Disconnect, retry with backoff |
| Sensor | Last known good value + stale flag |
| Storage | Read-only mode, report degraded |
| OTA | Abort update, keep current firmware |
| Authentication | Deny access, require re-auth |

## Watchdog Integration

- Feed the watchdog ONLY from the main loop — proves system is alive
- If a subsystem fails, stop feeding watchdog -> triggers system reset
- On reset, log the reason and attempt recovery
- After N consecutive watchdog resets, enter safe mode

## Assert vs Runtime Check

- Use `assert()` for programming errors that should never happen (debug only)
- Use runtime error checks for conditions that CAN happen (bad input, hardware fault)
- `assert()` is stripped from production builds — never use it for input validation
- Runtime checks MUST remain in production builds
