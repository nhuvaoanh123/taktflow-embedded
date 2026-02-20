---
paths:
  - "firmware/src/**/*"
  - "firmware/include/**/*"
---

# Logging & Diagnostics Standards

## Log Levels

| Level | Use For | Production |
|-------|---------|------------|
| FATAL | System cannot continue, entering safe state | Always logged |
| ERROR | Operation failed, requires attention | Always logged |
| WARN | Recoverable issue, unusual condition | Always logged |
| INFO | Normal operation milestones (boot, connect, OTA) | Configurable |
| DEBUG | Development diagnostics, variable dumps | Stripped from release builds |
| TRACE | Detailed execution flow, packet-level data | Stripped from release builds |

## Logging Rules

- Use structured log format: `[LEVEL] [timestamp] [module] message`
- Include context in every log: module name, function, relevant IDs
- NEVER log sensitive data: credentials, keys, tokens, PII
- NEVER log raw user input without sanitization
- Use log rotation or circular buffer — prevent storage exhaustion
- Log to non-volatile storage for post-mortem analysis (when possible)
- Rate-limit repetitive log messages — don't flood the log with the same error

## Security Event Logging (mandatory)

These events MUST be logged in production:

| Event | Severity | Details to Include |
|-------|----------|--------------------|
| Boot / reset | INFO | Reset reason, firmware version, hardware ID |
| Authentication attempt | INFO/WARN | Success/failure, method, source |
| Authentication failure | WARN | Failure count, lockout status |
| Connection established | INFO | Protocol, remote endpoint, TLS version |
| Connection lost | WARN | Reason, duration connected |
| OTA update attempt | INFO | Version from/to, source |
| OTA verification failure | ERROR | Signature result, expected vs actual |
| Invalid input rejected | WARN | Interface, validation rule triggered |
| Watchdog reset | ERROR | Last known state, uptime before reset |
| Tamper detection | FATAL | Sensor ID, type of tamper event |
| Configuration change | INFO | What changed, previous value (sanitized) |

## Diagnostics Endpoint

- Implement health/diagnostics reporting capability
- Include: firmware version, uptime, free memory, connection status, error counts
- DO NOT include secrets or sensitive configuration values
- Make diagnostics accessible via secure channel only (authenticated MQTT, BLE with pairing)

## Crash Diagnostics

- Capture crash information: stack pointer, fault address, register dump
- Store crash data in reserved memory region (survives soft reset)
- Report crash data to backend on next successful connection
- Include firmware version and uptime at crash time
- Implement core dump to flash if storage permits

## Runtime Metrics

Track and report periodically:
- Free heap memory (detect memory leaks over time)
- Stack high-water mark per task (detect near-overflow)
- CPU utilization per task
- Message queue depths
- Network latency and error rates
- Sensor reading statistics (min/max/avg over reporting period)
