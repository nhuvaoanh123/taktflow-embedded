---
paths:
  - "firmware/**/*"
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


# Vendor Independence Standards

## Core Lesson (from Taktflow web project)

> Started with Resend (vendor SDK), had to rip it out and switch to generic SMTP.
> Cost: multiple phases of migration work, leftover references in plans/env/health checks.

## Rules

### Prefer Generic Protocols Over Vendor SDKs

| Prefer | Over |
|--------|------|
| MQTT (generic) | AWS IoT SDK, Azure IoT SDK |
| HTTP/REST (generic) | Vendor-specific API clients |
| SMTP (generic) | SendGrid SDK, Resend SDK |
| BLE standard profiles | Vendor-specific BLE extensions |
| Standard TLS (mbedTLS) | Vendor-specific crypto libraries |

### When Using a Vendor SDK (sometimes unavoidable)

- Wrap it in an abstraction layer — swapping vendor = 1-file change
- Define your own interface (header file) that the app code uses
- Vendor SDK implements your interface, not the other way around
- Keep vendor-specific code in `firmware/lib/vendor/` — isolated from app code
- Document the vendor dependency and migration path

### Abstraction Pattern

```c
// YOUR interface (vendor-agnostic):
// firmware/include/cloud_client.h
typedef struct {
    error_t (*connect)(const char *host, uint16_t port);
    error_t (*publish)(const char *topic, const uint8_t *data, size_t len);
    error_t (*subscribe)(const char *topic, message_callback_t cb);
    error_t (*disconnect)(void);
} cloud_client_t;

// Vendor implementation (swappable):
// firmware/lib/vendor/aws_cloud_client.c  — implements cloud_client_t
// firmware/lib/vendor/azure_cloud_client.c — implements cloud_client_t
// firmware/lib/vendor/generic_mqtt_client.c — implements cloud_client_t
```

### Cleanup After Vendor Swap

- Grep entire codebase for old vendor name (plans, env files, configs, comments)
- Update health checks and diagnostics
- Update documentation and README
- Verify no orphaned credentials or configuration

### Hardware Vendor Independence

- Use HAL abstraction for all hardware access
- Target hardware = configuration, not hard-coded in app logic
- Support building for at least 2 platforms when practical (target + host for testing)
- Pin mappings in configuration files, not scattered in source

### Cloud Service Independence

- Define your own telemetry/command protocol — not vendor's wire format
- Use standard protocols (MQTT, HTTPS) — not vendor-specific transports
- Store device identity independently of cloud platform
- Design for cloud provider migration — connection config, not code rewrite

