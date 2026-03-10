# Lessons Learned

## 2026-03-09 — DCAN IFCMD bit position regression breaks CAN TX on TMS570

**Context**: SC (Safety Controller) CAN TX stopped working after flashing firmware built from HEAD. CAN ID 0x013 frames were absent from the bus. DCAN error status showed TxOk=0 and LEC=6 (CRC error).

**Mistake**: Commit `865d7ce` ("refactor: move CubeMX/HALCoGen configs under firmware ECU dirs") rewrote DCAN_IFCMD_* bit definitions using little-endian [7:0] positions instead of the correct big-endian [23:16] positions required by TMS570LC43x. Also changed DCAN register offsets (BTR: 0x0C→0x08, TEST: 0x14→0x10) and replaced the atomic NewDat read-clear with an explicit write-back using the wrong command bits.

**Fix**: Reverted DCAN_IFCMD_* defines to shifts [16..23], restored BTR/TEST offsets, removed redundant NewDat write-back (read with NEWDAT flag already clears atomically when WR=0).

**Principle**: TMS570 is big-endian ARM Cortex-R5F — DCAN IFxCMD command mask byte maps to bits [23:16], not [7:0]. Never assume little-endian register layout on TMS570. When refactoring files that move between directories, verify register-level constants against the TRM, not against ARM Cortex-M conventions. Always do a CAN smoke test (candump) after any firmware rebuild that touches DCAN code.

## 2026-03-09 — CAN signal byte-position mismatches masked by DTC belt-and-suspenders

**Context**: SIL overcurrent test (SG-001) failed — vehicle stayed in RUN instead of transitioning to SAFE_STOP. Additionally, motor temperature spiked to 146°C after overcurrent injection.

**Mistake**: Three independent encoding bugs were present simultaneously:
1. Plant-sim `_tx_motor_status()` packed MotorFaultStatus at byte 4 instead of byte 7 (DBC defines bit 56). CVC signal 21 reads byte 7 → always saw 0 → never confirmed motor fault.
2. RZC firmware `Swc_RzcCom.c` wrote `pdu[4] = (uint8)overcurrent` for Motor_Current (0x301), clobbering direction/enable bits at the same byte. DBC defines OvercurrentFlag at bit 34 (byte 4, bit 2) — should be `pdu[4] |= overcurrent << 2`.
3. Motor thermal model kept using latched 28A current for heat calculation even after `_hw_disabled = True`, producing unrealistic temperature runaway.

All signal-based fault detection was broken, but tests appeared to "mostly work" because fault scenarios inject DTC directly on CAN 0x500 as a fallback — masking the root cause for weeks.

**Fix**:
- Rewrote Motor_Status encoding to match DBC byte layout (byte 7 for MotorFaultStatus).
- Fixed RZC Motor_Current encoding to pack dir|enable|overcurrent as individual bits per DBC.
- Thermal model uses `thermal_current = 0.0 if _hw_disabled` to stop heating when motor driver is off.
- Added DBC byte-layout docstrings to all plant-sim TX functions to prevent future drift.

**Principle**: DBC is the single source of truth for CAN signal layout. Every TX function — firmware and plant-sim — must have an explicit byte-layout comment referencing DBC bit positions. When a belt-and-suspenders mechanism (direct DTC injection) masks signal-path failures, bugs hide for weeks. Always test the primary detection path in isolation before adding fallbacks. Treat "test passes but via wrong path" as a failure.

## 2026-03-10 — Dual TX path on same CAN ID breaks E2E protection

**Context**: FZC brake fault detection worked internally (fault=1, latch=1), but CVC never transitioned to SAFE_STOP. CAN 0x210 frames from FZC had all-zero payload despite active fault.

**Mistake**: `Swc_Brake.c` called `Com_SendSignal(FZC_COM_SIG_TX_BRAKE_FAULT)` every 10ms, which packed the fault value into the Com PDU buffer and set `com_tx_pending`. `Com_MainFunction_Tx` then sent this frame via `PduR_Transmit` — without E2E protection (no CRC, no alive counter). Meanwhile, `Swc_FzcCom_TransmitSchedule` correctly sent an E2E-protected frame on the same CAN ID (0x210) every 100ms. The Com layer frame ran at lower priority and fired 10x more often, so CVC received mostly invalid-E2E frames and rejected them.

**Fix**: Removed `Com_SendSignal` from `Swc_Brake.c`. The manual TX path in `Swc_FzcCom_TransmitSchedule` already reads brake fault from RTE and sends with proper E2E. One CAN ID = one TX path.

**Principle**: Never have two TX paths (Com layer + manual PduR_Transmit) for the same CAN ID. The Com layer doesn't add E2E protection — it just packs signals into PDU buffers. If a message needs E2E, use the manual path exclusively. Grep for `Com_SendSignal` and `PduR_Transmit` on the same PDU ID to detect conflicts.

## 2026-03-10 — SIL timing differences require platform-specific debounce

**Context**: runaway_accel scenario triggered SAFE_STOP instead of DEGRADED. SC plausibility check killed the relay before CVC could limit torque.

**Mistake**: SC plausibility debounce was 5 ticks (50ms) — designed for bare-metal where FOC inverter responds in <1ms. In SIL, CAN round-trip between CVC torque command and plant-sim current response is ~20-30ms. The 50ms debounce window was too tight — SC saw 2-3 "implausible" ticks during the CAN latency window and killed prematurely.

**Fix**: `#ifdef PLATFORM_POSIX` debounce = 10 ticks (100ms) for SIL, 5 ticks for HW. Same pattern already used for FZC steering/brake debounce.

**Principle**: Any debounce or timeout constant that depends on signal latency needs a PLATFORM_POSIX override. SIL CAN adds 20-30ms per hop vs <1ms on real hardware. Document the latency assumption next to every debounce constant. Use the existing `#ifdef PLATFORM_POSIX` pattern consistently across all ECUs.
