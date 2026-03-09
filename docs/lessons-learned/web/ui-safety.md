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
