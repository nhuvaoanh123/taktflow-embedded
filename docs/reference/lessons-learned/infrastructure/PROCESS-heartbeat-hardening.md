# Lessons Learned — Heartbeat Monitoring Hardening

**Project:** Taktflow Embedded — Zonal Vehicle Platform
**Period:** 2026-03-02
**Scope:** 7-phase hardening of heartbeat monitoring system (CVC, FZC, RZC, SC) to professional AUTOSAR patterns
**Result:** All 5 safety gaps closed, 134 tests green (38 CVC + 24 E2E SM + 27 FZC + 21 RZC + 24 SC), FTTI compliance achieved for CVC primary path

---

## 1. Derived Constants Eliminate a Class of Bug

**Context:** Three consecutive heartbeat bugs (commits `430f3ba`, `e4acb09`, `52b029d`) all stemmed from manually-set `HB_PERIOD_CYCLES` that didn't match the actual RTE period.

**Fix:** Replace all manual constants with derived macros:

```c
#define HB_TX_CYCLES  (CVC_HB_TX_PERIOD_MS / CVC_RTE_PERIOD_MS)
_Static_assert(CVC_HB_TX_PERIOD_MS % CVC_RTE_PERIOD_MS == 0u,
               "HB TX period must be exact multiple of RTE period");
```

**Lesson:** When timing constant A depends on timing constant B, derive A from B with a compile-time assertion. Manual synchronization between independent `#define` values is a bug factory. The `_Static_assert` catches the drift at compile time, not at runtime on the vehicle.

---

## 2. E2E_Protect with NULL Config/State is Silent No-Op

**Context:** CVC heartbeat TX called `E2E_Protect(NULL_PTR, NULL_PTR, ...)` — the E2E module early-returned on NULL, so heartbeats had zero E2E protection despite the code appearing to use E2E.

**Fix:** Added static `E2E_ConfigType` and `E2E_StateType` with proper `DataId`, initialized at startup.

**Lesson:** NULL pointer guards in AUTOSAR BSW modules make bugs silent. When integrating E2E, always verify with a test that captures the config pointer passed to `E2E_Protect` — a non-NULL assertion catches this immediately. Code review alone missed this because the function call "looked right."

---

## 3. Recovery Debounce Prevents Comm Status Oscillation

**Context:** Before hardening, a single valid heartbeat after a timeout would immediately restore OK status. On a noisy CAN bus, this causes rapid TIMEOUT→OK→TIMEOUT oscillation, which triggers cascading state machine transitions and DTC spam.

**Fix (Phase 4):** Counter-based recovery — require 3 consecutive heartbeats before declaring OK. Phase 5 replaced this with E2E SM window-based evaluation (subsumes the counter approach with more robust sliding window logic).

**Lesson:** Any binary status (OK/FAIL) derived from periodic messages needs hysteresis in both directions. Detection hysteresis (miss count) was already present; recovery hysteresis (consecutive-OK count) was missing. The E2E SM pattern handles both directions with a single configurable mechanism.

---

## 4. WdgM Supervised Entity with No Checkpoint = Dead Monitoring

**Context:** WdgM SE 3 was configured for heartbeat alive supervision (`{3u, 1u, 1u, 3u}`), but `WdgM_CheckpointReached(3u)` was never called in the heartbeat code. The watchdog was monitoring a checkpoint that never fired — it would eventually trigger a watchdog violation, but for the wrong reason.

**Fix:** Added `WdgM_CheckpointReached(3u)` at the heartbeat TX boundary (every 50ms), updated SE 3 config to expect 1-3 checkpoints per 100ms WdgM cycle.

**Lesson:** When adding a WdgM supervised entity, always verify with a test that the checkpoint actually fires. A supervised entity with no checkpoint is worse than no supervision — it creates a false sense of coverage and can trigger false watchdog violations.

---

## 5. FTTI Budget Must Be Documented, Not Assumed

**Context:** The original heartbeat timeouts (CVC: 150ms, SC: 200ms) were chosen for robustness but never formally checked against FTTI. Both exceeded the 100ms FTTI for SG-008 — a 2x violation that went unnoticed because no FTTI budget document existed.

**Fix:** Created `docs/safety/analysis/heartbeat-ftti-budget.md` with per-SG timing breakdown, proving which path is primary vs complementary for each safety goal. Tuned timeouts to achieve compliance: CVC FZC detection 100ms (exactly at FTTI), SC total 140ms (backup path, documented gap).

**Lesson:** Every safety mechanism's timeout chain must be formally documented against its FTTI before the first safety review. The budget doc should answer: "What is the worst-case detection time, and is it within FTTI?" If the answer involves "complementary mechanism" or "backup path," that argument must be explicit and traceable to the safety concept.

---

## 6. E2E State Machine vs Simple Miss Counter

**Context:** Simple miss counters (Phase 4) work for basic detection but have limitations:
- No distinction between startup (unknown state) and runtime monitoring
- Recovery is counter-based (fragile to message ordering)
- No formal "window" concept for evaluating communication quality

**Fix:** Implemented AUTOSAR-inspired E2E State Machine (`E2E_Sm.c`) with:
- Three states: INIT → VALID → INVALID
- Sliding window circular buffer
- Configurable per-ECU: `WindowSize`, `MinOkStateInit`, `MaxErrorStateValid`, `MinOkStateInvalid`

**Lesson:** For ASIL D heartbeat monitoring, a state machine with formal state transitions is more defensible in a safety audit than ad-hoc counters. The window-based approach naturally handles jitter, burst reception, and startup synchronization. The additional complexity (~120 LOC for E2E_Sm.c) pays for itself in auditability and robustness.

---

## 7. Per-ECU Thresholds Reflect Actual Safety Architecture

**Context:** Originally, CVC used a single `CVC_HB_MAX_MISS 3u` for both FZC and RZC. But FZC (steering/braking) has tighter safety requirements than RZC (motor), and RZC has local motor cutoff as primary protection.

**Fix:** Per-ECU thresholds: `CVC_HB_FZC_MAX_MISS 2u` (100ms, SG-008 FTTI compliant), `CVC_HB_RZC_MAX_MISS 3u` (150ms, justified by local motor cutoff primary path).

**Lesson:** One-size-fits-all timeout values waste safety margin on some ECUs while being too tight on others. Map each timeout to its safety goal and document why that specific value was chosen. The FTTI budget doc makes this reasoning auditable.

---

## Key Takeaways

| Topic | Lesson |
|-------|--------|
| Derived constants | Derive timing constants from base values + `_Static_assert` — eliminates manual sync bugs |
| E2E NULL config | Test that E2E_Protect receives non-NULL config — silent no-op is worse than crash |
| Recovery debounce | Every binary status needs hysteresis in both detection AND recovery directions |
| WdgM checkpoints | A supervised entity with no checkpoint = dead monitoring — always verify in tests |
| FTTI documentation | Budget doc must exist before safety review — "it's probably fine" is not compliant |
| E2E State Machine | Window-based SM > ad-hoc counters for ASIL D auditability |
| Per-ECU thresholds | Map each timeout to its specific safety goal and document the justification |
| SC E2E DataID mismatch | SC E2E DataID mismatch silently rejects all ECU heartbeats → SC declares all ECUs dead → SC_Status=FAULT cascades through all ECUs that depend on it — verify DataIDs with CAN capture before enabling SC_CAN_Receive |

---

## 8. SC Heartbeat Monitoring Disrupts STM32 ECU Heartbeats

**Date:** 2026-03-07

**Context:** After adding `SC_Heartbeat_Init()`, `SC_CAN_Receive()`, and `SC_Heartbeat_Monitor()` to `sc_main.c`, CAN captures showed `SC_Status=FAULT` and disrupted heartbeat frames from STM32 ECUs (CVC/FZC/RZC). The SC was actively receiving and processing heartbeat frames via DCAN.

**Root cause:** E2E DataID mismatch. The SC's `mb_data_ids[]` table held DataIDs that did not match what the STM32 ECUs embed in their heartbeat frames (CAN capture showed `0x0E` in frames that SC expected to have a different DataID). Every `SC_E2E_Check()` call returned `FALSE` → `SC_Heartbeat_NotifyRx()` never fired → `SC_Heartbeat_Monitor()` decremented all three ECU timeout counters to zero → declared all ECUs dead → `SC_Monitoring_Update()` broadcast `SC_Status=FAULT` on 0x013.

CVC reads 0x013, saw `SC_Status=FAULT`, and entered its own fault mode — which disrupted or halted its heartbeat TX. FZC/RZC then saw CVC heartbeat go silent and cascaded into fault. The STM32 heartbeat frames on the wire were physically correct the whole time; the disruption was entirely at the application layer.

**Fix:** Commented out `SC_Heartbeat_Init()`, `SC_CAN_Receive()`, and `SC_Heartbeat_Monitor()` in `sc_main.c`. SC now only transmits its own status (0x013) and does not evaluate ECU heartbeats. Cascade eliminated immediately.

**Principle:** When a monitoring node (SC) evaluates received frames with E2E, the DataIDs must be verified end-to-end on the physical bus before enabling the monitor in production. An E2E mismatch is silent at the wire level but catastrophic at the application level — every frame is silently rejected, the monitor declares all senders dead, and the resulting FAULT broadcast cascades through all ECUs that depend on SC_Status. Verify DataID alignment with a CAN capture before enabling SC_CAN_Receive in hardware bringup.


---

## 9. Three SC Bringup Bugs Found Back-to-Back on Physical Hardware

**Date:** 2026-03-07

**Context:** First full SC hardware bringup on TMS570LC4357 LaunchPad with physical STM32 ECUs on the same CAN bus. Symptoms diagnosed incrementally by re-enabling one function at a time (SC_Heartbeat_Init → SC_CAN_Receive → SC_Heartbeat_Monitor).

### Bug 1 — DCAN NewDat Write-Back Re-Sets the Bit It Just Cleared

**Mistake:** `dcan1_get_mailbox_data()` cleared NewDat with a read (WR=0 + DCAN_IFCMD_NEWDAT), then immediately wrote back with `WR=1 | DCAN_IFCMD_NEWDAT`, which re-sets NewDat=1. The comment said "clear NewDat by writing back with NewDat=0" but the DCAN protocol is opposite: WR=1 + NEWDAT **sets** the bit.

**Effect:** DCAN hardware saw NewDat permanently=1 in the message object. Every subsequent frame triggered MsgLst (overwrite of unread buffer) → bus disruption visible on CAN analyzer immediately after SC_CAN_Receive() was re-enabled.

**Fix:** Removed the write-back entirely. The read command (WR=0 + DCAN_IFCMD_NEWDAT in IF2CMD) already clears NewDat atomically. No write-back needed.

**Principle:** On DCAN (Bosch), WR=0 reads → clears NewDat atomically. WR=1 writes → sets NewDat. They are opposite. The IFCMD NEWDAT bit is a transfer trigger, not a copy of the MCTL state. Never write back MCTL after a read without verifying which direction WR controls.

### Bug 2 — sc_monitoring.c Not in Makefile

**Mistake:** sc_monitoring.c was implemented but never added to SC_CSRC. Linker error on SC_Monitoring_Init and SC_Monitoring_Update.

**Fix:** Added the file to SC_CSRC. One line.

**Principle:** Add every new .c file to the build system at creation time. A CI build after each new file would have caught this at the same commit.

### Bug 3 — SC_E2E_Init() Never Called, Alive Counter Fails on First Frame

**Mistake:** SC_E2E_Init() sets e2e_first_rx[i]=TRUE (skip alive check on first reception) but was never called. Static arrays zero-initialize to FALSE. SC stored e2e_last_alive=0 → expected counter=1, but ECUs had been running for seconds with counter at 7, 12, etc. Every frame failed E2E → SC_Heartbeat_NotifyRx never called → all ECUs timed out → SC_Status=FAULT.

**Fix:** Added SC_E2E_Init() after SC_CAN_Init() in sc_main.c.

**Principle:** At integration, grep for every _Init() in each module and verify it appears in the startup sequence. Zero-initialized statics produce a plausible-but-wrong default (FALSE instead of TRUE for e2e_first_rx) — invisible until hardware bringup.

### Overall Lesson

Hardware bringup exposed three bugs in sequence that unit tests did not catch because: (1) DCAN WR bit direction requires the physical peripheral to observe, (2) missing Makefile entry requires a full link, (3) missing Init call requires the real startup sequence. Incremental re-enable (one function at a time) gave a clean before/after for each bug.
