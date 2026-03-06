---
paths:
  - "firmware/**/*"
---
# OTA Updates

Signed (Ed25519/ECDSA) + verified BEFORE flash. A/B partitions. Anti-rollback (version ≥ current).
**Flow**: TLS download → size/signature/version check → write inactive partition → CRC verify → reboot → self-test → confirm or auto-rollback.
Never install partial/unsigned images. Auto-rollback after N failed boots. Log all attempts.

# Vendor Independence

Prefer generic protocols (MQTT, HTTP, TLS) over vendor SDKs. If vendor SDK needed, wrap in abstraction — swap = 1-file change. Vendor code in `firmware/lib/vendor/`.
HAL for all HW access. Pin mappings in config, not source. Support target + host builds.
