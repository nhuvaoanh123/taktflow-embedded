# SC Safety Evidence Gap Log

Date: 2026-03-08
Scope: safety-critical dataflow and state transitions for SC firmware.

Severity scale:
- `Critical`: can directly compromise intended fail-safe behavior
- `High`: substantial verification or diagnosability weakness on safety path
- `Medium`: important but not immediate hazard in current architecture

## Gap Register

| ID | Severity | Gap | Evidence | Risk | Closure Action |
|---|---|---|---|---|---|
| GAP-SC-001 | Critical | E-Stop path is not connected to relay decision logic | E-Stop ID/mailbox exists (`sc_cfg.h:25`, `46`), receive/E2E path exists (`sc_can.c`), but no consumer found in safety decisions (`rg` usage search) | Emergency stop command may be ignored by SC relay path | Implement explicit E-Stop parse and relay trigger path; add ASIL D tests for assert/clear/invalid E2E |
| GAP-SC-002 | High | E2E persistent failure latch is not used for fail-safe actions | `SC_E2E_IsMsgFailed` implemented (`sc_e2e.c:163`) but only referenced in tests | Repeated corrupted frames may not escalate beyond per-message drop | Add policy: critical mailbox E2E fail latch drives fault state/kill or degraded state with timeout |
| GAP-SC-003 | High | Bus-silence monitor is not consumed by relay or watchdog logic | `SC_CAN_IsBusSilent` exists (`sc_can.c:232`) and tested, but no runtime use | Network-wide silence may not trigger safe-stop if bus-off bit never sets | Integrate bus-silence as relay trigger or watchdog gate with debounce rationale |
| GAP-SC-004 | High | POSIX runtime bypasses E2E and plausibility checks | `sc_e2e.c:87`, `sc_plausibility.c:160` compile-time bypasses on POSIX | SIL dynamic tests do not validate key ASIL-D decision logic | Introduce fault-injectable SIL mode (not bypass) or dedicated non-bypass simulation path in CI |
| GAP-SC-005 | High | ESM initialization intentionally skipped in main bring-up | `SC_ESM_Init()` commented out in `sc_main.c` init block | Lockstep fault reaction chain can be inactive on real target builds | Restore ESM init behind feature gate once lockstep root cause fixed; enforce target-level test |
| GAP-SC-006 | Medium | No single explicit runtime state variable despite state enum constants | `SC_STATE_*` defined (`sc_cfg.h:200-203`) but unused | Harder transition auditing, easier drift between mode bits and actual behavior | Introduce authoritative state machine and transition tests |
| GAP-SC-007 | Medium | Diagnostic fault reason encoding collapses multiple kill reasons | monitoring maps ESM/bus-off/readback to self-test-class reason (`sc_monitoring.c`) | Reduced post-incident diagnosability and slower root-cause isolation | Preserve one-to-one reason mapping in SC_Status byte 3 allocation |
| GAP-SC-008 | High | Local dynamic validation blocked by host/tool mismatch for POSIX SC build | `make -f Makefile.posix TARGET=sc` fails with `Syntaxfehler`; direct compile of `sc_hw_posix.c` fails due missing Linux headers (`sys/socket.h`, `linux/can.h`) on current Windows MinGW toolchain | Safety evidence cannot be regenerated reliably on this workstation | Provide enforced WSL/Linux build path (or Windows-compatible SocketCAN abstraction) and a single supported local validation command |

## Coverage Snapshot (Current)

Available:
- Module unit tests exist under `firmware/sc/test/*.c`.
- Static path tracing completed for main loop and trigger chain.

Missing/blocked:
- Reproduced dynamic run evidence for POSIX SC in current Windows shell environment.
- Transition-level evidence matrix linking each critical edge to a passing test artifact.

## Priority Closure Plan

1. Close `GAP-SC-001` and `GAP-SC-003` first (direct safety decision-path omissions).
2. Close `GAP-SC-004` and `GAP-SC-008` together to restore executable evidence generation.
3. Close `GAP-SC-005` on target with hardware-backed verification.
4. Close `GAP-SC-006/007` as architecture hardening and diagnosability improvements.
