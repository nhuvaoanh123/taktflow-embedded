# Plan: MISRA C:2012 Compliance Checking Pipeline

**Project:** Taktflow Embedded — Zonal Vehicle Platform
**Created:** 2026-02-24
**Standard:** MISRA C:2012 / MISRA C:2023
**ASIL:** D (ISO 26262 Part 6)
**Tool:** cppcheck 2.17.1 + MISRA addon

---

## Status Table

| Phase | Name | Status |
|-------|------|--------|
| 1 | MISRA rule texts + cppcheck config | DONE |
| 2 | Makefile targets | DONE |
| 3 | Deviation register + suppression file | DONE |
| 4 | GitHub Actions CI workflow | DONE |
| 5 | First run + triage | RUN DONE, TRIAGE PENDING |

---

## Why This Exists

The firmware claims ISO 26262 ASIL D and MISRA C:2012 compliance. Without actual
tooling to check it, that claim is unsubstantiated. ISO 26262 Part 6 Section 8.4.6
requires automated MISRA checking as part of the development process.

**Tool choice**: cppcheck (free, open source) with MISRA addon. Covers ~70-85% of
the 143 statically-checkable MISRA C:2012 rules. MISRA officially provides rule
headline texts on their public GitLab (Feb 2025), which cppcheck needs for
human-readable output.

---

## Phase 1: Rule Texts + Cppcheck Config (DONE)

### What was done

1. **Created `scripts/setup-misra-rules.sh`** — downloads official MISRA rule
   headline texts from `https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/tools/`
   into `tools/misra/`. Supports both curl and wget. Idempotent (skips if already
   downloaded).

2. **Created `tools/misra/misra.json`** — cppcheck addon config that tells the
   MISRA checker where to find rule texts.

### Key detail: relative path

The `misra.json` rule-texts path is **relative to where cppcheck runs** (the CWD),
not relative to where the JSON file lives. Since the Makefile runs from `firmware/`,
the path must be `../tools/misra/misra_c_2023__headlines_for_cppcheck.txt`.

```json
{
    "script": "misra.py",
    "args": [
        "--rule-texts=../tools/misra/misra_c_2023__headlines_for_cppcheck.txt"
    ]
}
```

### Gotcha: rule texts are copyrighted

MISRA headline texts are provided for tool use but are NOT redistributable. They
must be downloaded per-developer and per-CI-run, never committed to Git. Added
`tools/misra/*.txt` to `.gitignore`.

---

## Phase 2: Makefile Targets (DONE)

### What was done

Added `misra` and `misra-report` targets to `firmware/Makefile.posix`.

**Usage:**
```bash
cd firmware
make -f Makefile.posix misra          # Output to terminal
make -f Makefile.posix misra-report   # Output to build/misra-report.txt
```

### Design decisions

1. **TARGET validation skip** — The Makefile normally requires `TARGET=bcm|icu|...`.
   MISRA targets check ALL ECUs at once, so they need to bypass that validation.
   Solved with:
   ```makefile
   MISRA_GOALS = misra misra-report
   ifneq ($(filter $(MISRA_GOALS),$(MAKECMDGOALS)),)
     _SKIP_TARGET_CHECK = 1
   endif
   ```

2. **Separate directory variables** — MISRA targets use `_`-prefixed variables
   (`_BSW_DIR`, `_MCAL_DIR`, etc.) to avoid collision with the build variables
   that depend on `TARGET`.

3. **BSW checked once, each ECU separately** — BSW sources get one cppcheck pass
   with all BSW include paths. Each ECU gets its own pass with BSW includes + its
   own `-I<ecu>/include`. SC is separate (no BSW, just `-Isc/include`).

4. **POSIX backends excluded from BSW check** — `*_Posix.c` files are POSIX
   simulation only, not target firmware. They use POSIX APIs that violate MISRA
   by design.

### Cppcheck flags explained

```
--addon=../tools/misra/misra.json   # MISRA rule checker
--enable=style,warning              # Enable style and warning checks
--std=c99                           # C99 standard (matches -std=c99 in build)
--platform=unix32                   # 32-bit platform model (MCU target)
--suppress=missingIncludeSystem     # Don't warn about <system.h> not found
--suppressions-list=../tools/misra/suppressions.txt  # Project suppressions
--inline-suppr                      # Allow // cppcheck-suppress in code
--error-exitcode=0                  # Non-blocking (switch to 1 after triage)
```

### Gotcha: stderr redirect order

cppcheck sends violations to **stderr** and progress to stdout. The redirect
order matters:

```bash
# WRONG — stderr goes to terminal, only stdout goes to file
cppcheck ... 2>&1 >> file

# CORRECT — stdout goes to file first, then stderr follows
cppcheck ... >> file 2>&1
```

This cost a debugging round during first run.

---

## Phase 3: Deviation Register + Suppressions (DONE)

### What was done

1. **Created `tools/misra/suppressions.txt`** — starts empty, populated during
   triage for false positives and known cppcheck limitations.

2. **Created `docs/safety/analysis/misra-deviation-register.md`** — formal ISO 26262
   deviation register. Each deviation from a required rule gets:
   - Rule number + headline
   - File:line location
   - Technical justification
   - Risk assessment
   - Compensating measure
   - Independent reviewer sign-off

### MISRA rule categories

| Category | ASIL D compliance | Deviations allowed |
|----------|-------------------|-------------------|
| Mandatory | Zero tolerance | NEVER |
| Required | Full compliance | Yes, with formal documented deviation |
| Advisory | Tracked | No formal deviation needed, but compliance expected |

---

## Phase 4: GitHub Actions CI (DONE)

### What was done

Created `.github/workflows/misra.yml`:
- Triggers on push/PR to `firmware/**`, `tools/misra/**`, or the workflow itself
- Installs cppcheck via apt-get
- Downloads MISRA rule texts via setup script
- Runs `make -f Makefile.posix misra-report`
- Uploads `build/misra-report.txt` as artifact (30-day retention)
- Prints summary (violation count + first 50 violations)

### Currently non-blocking

`error-exitcode=0` means the CI step always passes. After triage is complete,
switch to `error-exitcode=1` so new violations block PRs.

---

## Phase 5: First Run Results (2026-02-24)

### Summary

- **Total violations: 1,536**
- **Unique rules triggered: 29**
- **Files checked: ~80 .c files** across BSW + 7 ECUs

### Violations by Rule (top 10)

| Rule | Count | Category | Description |
|------|-------|----------|-------------|
| 2.5 | 470 | Advisory | Unused macro declarations |
| 15.5 | 386 | Advisory | Multiple return points (single exit) |
| 8.5 | 269 | Required | External object/function declared in source |
| 8.7 | 91 | Advisory | Object with external linkage should be block scope |
| 8.4 | 89 | Required | Compatible declaration not visible |
| 2.3 | 54 | Advisory | Unused type declarations |
| 8.9 | 51 | Advisory | Object with automatic storage could be block scope |
| 5.9 | 22 | Advisory | Internal linkage object identifier reused |
| 2.4 | 21 | Advisory | Unused tag declarations |
| 17.8 | 12 | Advisory | Function parameter modified |

### Violations by Component

| Component | Count | Notes |
|-----------|-------|-------|
| BSW | 307 | Shared AUTOSAR-like BSW modules |
| bcm | 98 | Body Control Module |
| icu | 91 | Instrument Cluster Unit |
| tcu | 170 | Telematics Control Unit (most complex UDS) |
| cvc | 236 | Central Vehicle Computer |
| fzc | 239 | Front Zone Controller |
| rzc | 245 | Rear Zone Controller |
| sc | 150 | Safety Controller (no BSW) |

### Analysis of top violations

**Rule 2.5 (470 — Advisory: unused macros)**
- Root cause: shared headers (`Std_Types.h`, `Platform_Types.h`) define macros for
  all ECUs, but each ECU only uses a subset.
- Action: Suppress. This is inherent to shared-header embedded architecture.

**Rule 15.5 (386 — Advisory: multiple returns)**
- Root cause: defensive programming pattern — early return on parameter validation.
  This is actually RECOMMENDED at ASIL D for defensive coding.
- Action: Suppress with deviation rationale. ISO 26262 Part 6 defensive programming
  requirements take precedence over this advisory MISRA rule.

**Rule 8.5 (269 — Required: external declaration in source)**
- Root cause: functions/objects with external linkage declared in .c files instead
  of headers.
- Action: Fix. Add missing `extern` declarations to header files.

**Rule 8.4 (89 — Required: compatible declaration not visible)**
- Root cause: function defined with external linkage but no prior declaration
  visible (missing prototype in header).
- Action: Fix. Add missing prototypes to header files.

**Rule 17.7 (4 — Required: return value discarded)**
- Root cause: calling functions without checking return value.
- Action: Fix. This is a real safety concern at ASIL D.

### Triage categories

| Category | Action | Estimated count |
|----------|--------|-----------------|
| True violation — fix | Modify code | ~400 (8.5, 8.4, 17.7, 15.7, etc.) |
| Advisory — suppress | Add to suppressions.txt | ~1,000 (2.5, 15.5, 8.7, 2.3, etc.) |
| Intentional deviation | Add to deviation register | ~50 (15.5 in safety-critical paths) |
| POSIX backend only | Already excluded | 0 (excluded from check) |

---

## Files Created/Modified

| File | Action | Purpose |
|------|--------|---------|
| `scripts/setup-misra-rules.sh` | CREATE | Download MISRA rule texts |
| `tools/misra/misra.json` | CREATE | Cppcheck MISRA addon config |
| `tools/misra/suppressions.txt` | CREATE | Suppression file (starts empty) |
| `docs/safety/analysis/misra-deviation-register.md` | CREATE | ISO 26262 deviation register |
| `.github/workflows/misra.yml` | CREATE | CI workflow |
| `firmware/Makefile.posix` | MODIFY | Added misra + misra-report targets |
| `.gitignore` | MODIFY | Added tools/misra/*.txt |

---

## Local Dev Setup

### Prerequisites
- cppcheck (via `pip install cppcheck` on Windows, `apt-get install cppcheck` on Linux)
- make (via choco/apt — needs admin on Windows)
- bash (Git Bash / MSYS2 on Windows)

### First-time setup
```bash
# 1. Download rule texts (one-time)
bash scripts/setup-misra-rules.sh

# 2. Run MISRA check
cd firmware
make -f Makefile.posix misra          # terminal output
make -f Makefile.posix misra-report   # file output → build/misra-report.txt
```

### Without make (Windows without admin)
```bash
export PATH="$PATH:/c/Users/<user>/AppData/Roaming/Python/Python314/Scripts"
cd firmware
# Run cppcheck directly with the same flags as the Makefile
cppcheck --addon=../tools/misra/misra.json --enable=style,warning --std=c99 \
  --platform=unix32 --suppress=missingIncludeSystem \
  --suppressions-list=../tools/misra/suppressions.txt --inline-suppr \
  --error-exitcode=0 \
  -Ishared/bsw/include -Ishared/bsw/mcal -Ishared/bsw/ecual \
  -Ishared/bsw/services -Ishared/bsw/rte \
  shared/bsw/mcal/Can.c shared/bsw/mcal/Spi.c ...
```

---

## Next Steps

1. **Triage** — categorize each of the 1,536 violations
2. **Fix required-rule violations** — 8.5, 8.4, 17.7, 15.7 (add missing headers/prototypes)
3. **Populate suppressions.txt** — advisory rules with justification comments
4. **Populate deviation register** — any required-rule violations that can't be fixed
5. **Switch CI to blocking** — change `error-exitcode=0` → `1` after triage
6. **Establish baseline** — zero new violations on future PRs
