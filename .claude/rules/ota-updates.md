---
paths:
  - "firmware/**/*"
---

# OTA (Over-The-Air) Update Standards

## Core Requirement

ALL OTA updates MUST be cryptographically signed and verified before installation.

## Signing & Verification

- Sign firmware images with Ed25519 or ECDSA (P-256 minimum)
- Store public verification key in protected memory (OTP, eFuse, secure element)
- Verify signature BEFORE writing to flash — reject unsigned or invalid images
- Use a separate signing key for each product line
- Signing keys must never exist on the device — only the public key
- Keep signing private key in HSM or air-gapped machine

## Update Process

1. **Download**: Fetch update over TLS-secured channel
2. **Verify size**: Check against expected size, reject if exceeds flash partition
3. **Verify signature**: Cryptographic signature check — fail = reject entirely
4. **Verify version**: Prevent rollback — new version must be >= current version
5. **Write to inactive partition**: Dual-partition (A/B) scheme for safe updates
6. **Verify written image**: Read back and verify CRC/hash matches
7. **Set boot flag**: Mark new partition as pending-boot
8. **Reboot**: Boot into new firmware
9. **Self-test**: New firmware validates itself on first boot
10. **Confirm**: Mark update as confirmed — or rollback to previous partition

## Rollback Protection

- Use A/B partition scheme — always have a known-good fallback
- If new firmware fails self-test, automatically rollback
- Set maximum boot attempts before rollback (e.g., 3 attempts)
- Never erase the previous working firmware until new one is confirmed
- Implement anti-rollback counter to prevent downgrade attacks

## Version Management

- Use semantic versioning: MAJOR.MINOR.PATCH
- Embed version in firmware binary (readable at runtime)
- Anti-rollback: reject updates with version < current installed version
- Store version metadata alongside firmware image (build date, commit hash)

## Delivery

- Use TLS for update download — verify server certificate
- Support resume for interrupted downloads (range requests)
- Validate total download size before starting
- Use delta updates when possible to reduce bandwidth
- Implement download progress reporting

## Failure Modes

- Download interrupted: Resume or restart, never install partial image
- Signature invalid: Reject entirely, log security event, keep current firmware
- Flash write failure: Abort update, verify current firmware integrity
- Boot failure after update: Automatic rollback after N failed boot attempts
- No network: Queue update check, retry with exponential backoff

## Audit Trail

- Log all update attempts (success and failure) with timestamp
- Log firmware version transitions
- Log signature verification results
- Report update status to backend server when possible
