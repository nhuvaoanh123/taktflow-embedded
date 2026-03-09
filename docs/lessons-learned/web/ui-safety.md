# Lessons Learned

## 2026-03-09 — DCAN IFCMD bit position regression breaks CAN TX on TMS570

**Context**: SC (Safety Controller) CAN TX stopped working after flashing firmware built from HEAD. CAN ID 0x013 frames were absent from the bus. DCAN error status showed TxOk=0 and LEC=6 (CRC error).

**Mistake**: Commit `865d7ce` ("refactor: move CubeMX/HALCoGen configs under firmware ECU dirs") rewrote DCAN_IFCMD_* bit definitions using little-endian [7:0] positions instead of the correct big-endian [23:16] positions required by TMS570LC43x. Also changed DCAN register offsets (BTR: 0x0C→0x08, TEST: 0x14→0x10) and replaced the atomic NewDat read-clear with an explicit write-back using the wrong command bits.

**Fix**: Reverted DCAN_IFCMD_* defines to shifts [16..23], restored BTR/TEST offsets, removed redundant NewDat write-back (read with NEWDAT flag already clears atomically when WR=0).

**Principle**: TMS570 is big-endian ARM Cortex-R5F — DCAN IFxCMD command mask byte maps to bits [23:16], not [7:0]. Never assume little-endian register layout on TMS570. When refactoring files that move between directories, verify register-level constants against the TRM, not against ARM Cortex-M conventions. Always do a CAN smoke test (candump) after any firmware rebuild that touches DCAN code.
