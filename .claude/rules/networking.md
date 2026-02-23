---
paths:
  - "firmware/src/**/*"
  - "firmware/include/**/*"
---

## Human-in-the-Loop (HITL) Comment Lock

`HITL` means human-reviewer-owned comment content.

**Marker standard (code-friendly):**
- Markdown: `<!-- HITL-LOCK START:<id> -->` ... `<!-- HITL-LOCK END:<id> -->`
- C/C++/Java/JS/TS: `// HITL-LOCK START:<id>` ... `// HITL-LOCK END:<id>`
- Python/Shell/YAML/TOML: `# HITL-LOCK START:<id>` ... `# HITL-LOCK END:<id>`

**Rules:**
- AI must never edit, reformat, move, or delete text inside any `HITL-LOCK` block.
- Append-only: AI may add new comments/changes only; prior HITL comments stay unchanged.
- If a locked comment needs revision, add a new note outside the lock or ask the human reviewer to unlock it.


# Networking Standards (IoT Communications)

## Transport Security

- ALL network communication MUST use TLS — no plaintext HTTP, MQTT, or WebSocket
- TLS 1.2 minimum, prefer TLS 1.3
- Verify server certificates — NEVER disable verification (even for testing)
- Pin certificates or public keys for known backend servers
- Use certificate rotation strategy — don't rely on a single long-lived cert

## MQTT

- Use MQTT over TLS (port 8883), never plain MQTT (port 1883)
- Authenticate with client certificates or unique per-device credentials
- Use QoS 1 or QoS 2 for critical messages (not QoS 0)
- Implement Last Will and Testament (LWT) for disconnect detection
- Subscribe only to topics the device needs — principle of least privilege
- Validate all incoming MQTT payload content before processing
- Enforce maximum payload size
- Use clean session = false for persistent subscriptions when appropriate
- Implement exponential backoff on reconnection

## BLE (Bluetooth Low Energy)

- Require pairing and bonding for sensitive characteristics
- Use Secure Connections (LE Secure Connections, not Legacy Pairing)
- Validate all characteristic write values (length, range, type)
- Implement MTU negotiation — don't assume default 23-byte MTU
- Rate-limit connection attempts to prevent DoS
- Disable BLE advertising when not needed (reduce attack surface)
- Use random resolvable private addresses (RPA) to prevent tracking

## HTTP / REST APIs

- HTTPS only — redirect HTTP to HTTPS or reject
- Validate Content-Type header
- Enforce maximum request body size
- Use authentication on all endpoints (API key, JWT, mTLS)
- Implement rate limiting per device identity
- Set appropriate timeouts for HTTP client requests
- Validate response status codes — don't assume 200

## Wi-Fi

- Use WPA3 when available, WPA2 minimum
- Store Wi-Fi credentials in encrypted flash, never in plaintext
- Implement Wi-Fi provisioning through secure channel (BLE, AP mode with HTTPS)
- Monitor for deauthentication attacks and log anomalies
- Implement reconnection with exponential backoff

## DNS & NTP

- Validate DNS responses — check for unexpected redirects
- Use DNSSEC if available
- Use authenticated NTP (NTS) when possible
- Validate time synchronization — reject grossly incorrect times
- Protect against time-based replay attacks

## Connection Resilience

- Implement connection health monitoring (ping/heartbeat)
- Use exponential backoff with jitter for all reconnection attempts
- Queue messages during disconnection — flush on reconnect
- Set maximum queue depth to prevent memory exhaustion
- Log connection state transitions for diagnostics
- Implement circuit breaker pattern for repeated failures

