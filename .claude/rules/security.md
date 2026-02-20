---
paths:
  - "firmware/**/*"
  - "scripts/**/*"
---

# Security Standards (Embedded / IoT)

## Core Principle: Fail-Closed

- Missing configuration = REJECT operation, never silently skip
- Missing credentials = REJECT connection, never allow anonymous
- Failed validation = REJECT input, never pass through
- Unknown state = safe state, never undefined behavior

## Credential Management

- NEVER hardcode passwords, API keys, tokens, or certificates in source code
- NEVER commit secrets to version control (even in comments or tests)
- Store credentials in secure storage (flash encryption, secure element, TPM)
- Use unique per-device credentials — no shared secrets across fleet
- Rotate credentials on suspected compromise
- Derive session keys from master keys — never use master keys directly

## Cryptography

- Use established libraries (mbedTLS, wolfSSL, libsodium) — never roll your own crypto
- TLS 1.2 minimum for all network communication — prefer TLS 1.3
- Minimum key sizes: RSA 2048-bit, ECC 256-bit (P-256 or Curve25519)
- Use AEAD ciphers (AES-GCM, ChaCha20-Poly1305) — not CBC mode
- Use cryptographically secure random number generators (hardware RNG when available)
- Validate certificate chains — never disable certificate verification
- Pin certificates or public keys for known servers when possible

## Secure Boot & Firmware Integrity

- Enable secure boot chain if hardware supports it
- Verify firmware signature before execution
- Store root-of-trust in hardware (eFuse, OTP, secure element)
- Protect bootloader from modification — lock flash regions
- Log boot integrity check results

## Debug & Production Separation

- Disable JTAG/SWD debug ports in production firmware
- Remove all debug print statements from production builds
- Do not ship with default/development credentials
- Disable serial console in production unless explicitly needed
- Use `#ifdef DEBUG` guards for all debug-only code paths
- Strip debug symbols from production binaries

## Anti-Tampering

- Detect and log physical tampering attempts if hardware supports it
- Implement secure erase of sensitive data on tamper detection
- Validate flash integrity periodically (CRC/hash checks)
- Monitor for unexpected resets and log patterns

## Error Messages

- Never expose internal state, memory addresses, or stack traces in external responses
- Use generic error codes for external interfaces
- Log detailed errors internally only
- Sanitize all data before including in log messages

## Supply Chain

- Verify integrity of all third-party libraries (checksums, signed releases)
- Pin dependency versions — no floating/latest
- Audit dependencies for known vulnerabilities before inclusion
- Minimize third-party code surface area — only include what you need
