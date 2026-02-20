---
paths:
  - "scripts/**/*"
  - "firmware/**/*"
  - ".github/**/*"
---

# Build & CI/CD Standards

## Build System

- Single command to build: `make build` or equivalent
- Single command to test: `make test`
- Single command to flash: `make flash`
- Single command to clean: `make clean`
- Build output directory separate from source: `build/` (gitignored)
- Support both debug and release builds: `make build-debug`, `make build-release`

## Compiler Flags

### Required for all builds:
```
-Wall              # All standard warnings
-Wextra            # Extra warnings
-Wshadow           # Variable shadowing
-Wdouble-promotion # Implicit float-to-double (expensive on MCU)
-Wformat=2         # Format string validation
-Wundef            # Undefined macro usage
```

### Required for release builds:
```
-Werror            # Treat warnings as errors
-Os or -O2         # Optimize for size or speed (project decision)
-DNDEBUG           # Disable assert()
-fstack-protector  # Stack smashing detection
```

### Required for debug builds:
```
-Og                # Optimize for debugging
-g3                # Full debug symbols
-DDEBUG            # Enable debug code paths
-fsanitize=address,undefined  # Sanitizers (host builds only)
```

## CI Pipeline

### On every commit:
1. Compile firmware (debug + release) — fail on warnings
2. Run unit tests on host
3. Run static analysis (cppcheck or clang-tidy)
4. Check code formatting
5. Verify no secrets in code (`detect-secrets` or similar)

### On every PR:
1. All commit checks above
2. Run integration tests
3. Run security tests / fuzzing (if configured)
4. Generate coverage report
5. Size comparison vs main branch (detect firmware bloat)

### Before release:
1. All PR checks above
2. Hardware-in-the-loop tests
3. Full fuzz campaign
4. Sign firmware binary
5. Generate release notes from commit history
6. Claim accuracy audit

## Firmware Size Tracking

- Track firmware binary size in CI — alert on unexpected growth
- Set size budget per flash partition
- Log component-level size breakdown (text, data, bss, rodata)
- Block merge if firmware exceeds partition size

## Reproducible Builds

- Pin all toolchain versions (compiler, linker, libraries)
- Use lock files for package managers
- Document exact toolchain setup in `docs/toolchain-setup.md`
- CI must use the same toolchain version as developers
- Build output should be deterministic (same source = same binary)

## Flash Scripts

- Flash scripts in `scripts/` directory
- Support both USB and OTA flash methods
- Verify flash succeeded (read back and compare)
- Flash script must check firmware signature before flashing
- Document required hardware setup for flashing
