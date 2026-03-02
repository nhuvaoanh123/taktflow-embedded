# Lessons Learned — Simulated Kill Relay for SIL Demo

**Date**: 2026-03-02
**Scope**: 4-phase implementation — SC CAN broadcast → plant-sim → CVC BSW routing → fault scenarios
**Plan**: `docs/plans/plan-simulated-relay.md`

---

## 1. Adding a New CAN→RTE Signal Requires 5 Files Minimum

To route a single new CAN message through the AUTOSAR-like BSW into an SWC:

| # | File | What |
|---|------|------|
| 1 | `Cvc_Cfg.h` | New signal ID (`CVC_SIG_xxx`), bump `CVC_SIG_COUNT`, new PDU ID (`CVC_COM_RX_xxx`) |
| 2 | `main.c` | Add to `canif_rx_config[]` (CAN ID → PDU ID) and `cvc_pdur_routing[]` (PDU → Com) |
| 3 | `Com_Cfg_Cvc.c` | Shadow buffer variable, signal config entry (ID, bitPos, bitSize, type, PDU, buffer), RX PDU config entry |
| 4 | `Swc_CvcCom.c` | `Com_ReceiveSignal()` + `Rte_Write()` in `BridgeRxToRte()` |
| 5 | `Swc_VehicleState.c` (or consumer SWC) | `Rte_Read()` + business logic |

**Takeaway**: This is the AUTOSAR tax — 5 files for 1 signal. But it's also what makes the architecture traceable and testable. Don't shortcut it.

---

## 2. PLATFORM_POSIX_TEST Guard for Unit Tests on Windows

**Problem**: SC firmware uses SocketCAN (`sys/socket.h`, `linux/can.h`) in its POSIX HAL. Unit tests run on Windows where these headers don't exist. But the test needs `PLATFORM_POSIX` defined to compile the broadcast function under test.

**Solution**: Two-level guard:
```c
#ifdef PLATFORM_POSIX
#ifndef PLATFORM_POSIX_TEST   /* skip in unit tests on Windows */
#include <sys/socket.h>
#include <linux/can.h>
#endif
#endif
```

Compile tests with: `-DPLATFORM_POSIX -DPLATFORM_POSIX_TEST -DUNIT_TEST`

**Takeaway**: When SIL-only code has OS-specific includes, add a `_TEST` sub-guard so the source-inclusion test pattern still works on the dev machine.

---

## 3. CVC Test Include Paths Need shared/bsw/services/ (Not Just include/)

**Problem**: CVC test build failed with `BswM.h: No such file or directory` even though the header guard `BSWM_H` was pre-defined in the test file.

**Root cause**: GCC resolves `#include "BswM.h"` from the source file *before* checking the guard. If the file isn't on the include path, compilation fails — the guard never gets a chance to prevent inclusion.

**Fix**: Add `-I shared/bsw/services` to the GCC command (alongside `-I shared/bsw/services/include`).

**Takeaway**: Source-inclusion test pattern + header guards only works if ALL directories containing referenced headers are on the include path, even for headers you intend to skip.

---

## 4. SIL-Only vs Production Code Separation

The relay feature splits cleanly:

| Code | Where it runs | Guard |
|------|--------------|-------|
| `kill_reason` tracking | Everywhere (SIL + HIL + production) | None — always compiled |
| `SC_Relay_BroadcastSil()` | SIL Docker only | `#ifdef PLATFORM_POSIX` |
| `sc_posix_can_send()` | SIL Docker only | `#ifndef PLATFORM_POSIX_TEST` within `#ifdef PLATFORM_POSIX` |
| CVC reads CAN 0x013 → SAFE_STOP | Everywhere | None — standard BSW path |
| Plant-sim reads CAN 0x013 | SIL Docker only | Python (only runs in Docker) |

**Takeaway**: The `kill_reason` state variable and the CVC BSW routing are production-quality code that will work on real hardware. Only the CAN TX mechanism is SIL-specific. Good ROI — ~80% of the code is reusable for HIL/target.

---

## 5. Docker Container Stop as a Legitimate Fault Injection Mechanism

**Before**: `heartbeat_loss` scenario was documentation-only ("run `docker stop fzc` manually").

**After**: Scenario calls `docker.from_env().containers.get("docker-fzc-1").stop()` directly — one-click from the dashboard.

**Insight**: For SIL, stopping a container is the *exact equivalent* of an ECU losing power. It's not a workaround — it's the correct simulation. The SC sees the same effect (heartbeat ceases) as it would with a real ECU failure.

**Takeaway**: Don't be afraid to use Docker lifecycle as fault injection. Container stop = ECU power loss. Container restart = ECU reboot. These are first-class test actions, not hacks.

---

## 6. Recovery Guard Must Include New Signals

When adding a new fault signal that triggers SAFE_STOP (like `sc_relay_kill`), **always** add its clear-condition to the SAFE_STOP recovery guard. Otherwise the system can "recover" while the fault is still active.

The recovery guard now checks 7 conditions:
```c
if ((estop_active == 0u) &&
    (motor_cutoff == 0u) &&
    (brake_fault == 0u) &&
    (steering_fault == 0u) &&
    (pedal_fault == 0u) &&
    (sc_relay_kill == 0u) &&    /* NEW — easy to forget */
    (fzc_comm == CVC_COMM_OK) &&
    (rzc_comm == CVC_COMM_OK))
```

**Takeaway**: Every new SAFE_STOP trigger must have a corresponding recovery guard entry. Make this a checklist item for any state machine extension.

---

## 7. Test Count Growth as a Health Metric

| Module | Before | After | Delta |
|--------|--------|-------|-------|
| SC relay | 17 | 23 | +6 (broadcast tests) |
| CVC VehicleState | 49 | 51 | +2 (signal-path tests) |

Total project tests remain healthy. The +8 tests verify the new feature end-to-end at the unit level.

---

## 8. SocketCAN Single-Socket Consumes All Frames — Buffer First, Distribute Second

**Problem**: SC's `dcan1_get_mailbox_data()` reads from a single SocketCAN socket. When checking mailbox 0 (E-Stop 0x001), the function reads and discards all non-matching frames — including heartbeats (0x010, 0x011, 0x012). By the time mailbox 1/2/3 are queried, their frames are gone.

**Root cause**: Real DCAN hardware has per-mailbox hardware filters that run in parallel. SocketCAN is a serial stream — reading for one ID consumes frames for all IDs.

**Fix**: Drain all pending frames into a per-mailbox buffer once per tick, then serve from the buffer for individual mailbox queries:
```c
/* Drain on first call per tick */
if (rx_drained == FALSE) {
    while (recv(dcan_fd, &frame, ...) > 0) {
        /* Sort into rx_slot[mailbox] by CAN ID */
    }
    rx_drained = TRUE;
}
/* Serve from buffer */
return rx_slot[mbIndex].valid ? copy_and_return() : FALSE;
```
Reset `rx_drained = FALSE` in `rtiClearTick()`.

**Takeaway**: When emulating hardware mailboxes over SocketCAN, always buffer-then-distribute. A per-ID serial scan will starve later mailboxes.

---

## 9. E2E Format Mismatch Is Silent — Heartbeats Arrive But Are Rejected

**Problem**: SC's E2E module expects `alive` in upper nibble of byte 0 and CRC-8 in byte 1. BSW senders (CVC/FZC/RZC) use a different E2E layout. All E2E checks fail silently, so `SC_Heartbeat_NotifyRx()` is never called.

**Root cause**: The SC (TMS570, bare-metal) and the AUTOSAR BSW ECUs use independently developed E2E implementations with different byte layouts.

**Fix for SIL**: Bypass E2E in Docker builds where vcan0 provides perfect data integrity:
```c
#if defined(PLATFORM_POSIX) && !defined(UNIT_TEST)
    return TRUE;  /* SIL bypass — vcan0 has no bit-flips */
#else
    /* Full E2E check for production + unit tests */
#endif
```

**Lesson**: This bug was invisible until the relay broadcast feature — SC was always killing the relay in SIL, but nobody knew because there was no observable effect. **Adding observability (CAN 0x013 broadcast) exposed a pre-existing bug.**

**Takeaway**: When adding observability to a system, expect to discover bugs that were always there. Also: E2E format must be documented in a shared CAN matrix, not just in each ECU's source code.

---

## 10. Rte_Cfg Array Size Must Match SIG_COUNT — The 6th File

**Problem**: After bumping `CVC_SIG_COUNT` to 32 for the new signal, the Docker build (`-Werror`) failed because `Rte_Cfg_Cvc.c` had only 31 initializers for a 32-element array.

**Fix**: Add `{ CVC_SIG_SC_RELAY_KILL, 0u }` to the RTE signal config array.

**Takeaway**: The AUTOSAR routing tax is actually **6 files**, not 5. `Rte_Cfg_Cvc.c` is the silent 6th that only bites you if SIG_COUNT changes.

---

## Key Takeaways

1. **AUTOSAR routing is verbose but traceable** — 6 files for 1 signal is the cost of layered architecture
2. **Two-level POSIX guards** (`PLATFORM_POSIX` + `PLATFORM_POSIX_TEST`) solve the Windows unit test problem cleanly
3. **Docker stop = ECU failure** — use container lifecycle as first-class fault injection
4. **Every SAFE_STOP trigger needs a recovery guard entry** — otherwise the system "recovers" while the fault persists
5. **~80% of SIL feature code is production-reusable** — only the CAN TX mechanism is SIL-specific
6. **Buffer-then-distribute for SocketCAN mailbox emulation** — serial recv() starves later mailboxes
7. **Adding observability exposes pre-existing bugs** — the relay broadcast revealed E2E and frame consumption bugs that existed from day one
8. **E2E format must be a shared specification** — not independently developed per ECU
