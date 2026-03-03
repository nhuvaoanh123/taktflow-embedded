# Lessons Learned — STM32 CubeMX Bringup

**Date**: 2026-03-03
**Scope**: CubeMX project creation for STM32G474RETx (NUCLEO-G474RE), clock tree, FDCAN1 peripheral configuration
**Status**: Open (bringup in progress)

---

## 2026-03-03 — CubeMX board selection: use Board Selector, not MCU Selector

**Context**: Creating CubeMX project for the NUCLEO-G474RE board.
**Mistake**: Searched for "STM32G474RE" in the MCU Selector — board not found by that name.
**Fix**: Use the **Board Selector** tab and search for `NUCLEO-G474RE`. CubeMX pre-configures Nucleo-specific pins (LD2=PA5, B1=PC13, VCP=PA2/PA3, SWD=PA13/PA14, HSE/LSE oscillators).
**Principle**: Always start from the board (not the bare MCU) when using a development kit. The board preset saves manual pin configuration and avoids missing oscillator/debug settings.

---

## 2026-03-03 — CubeMX firmware package must be installed first

**Context**: Board Selector showed no results for NUCLEO-G474RE.
**Mistake**: The STM32G4 firmware package was not installed in CubeMX.
**Fix**: Help → Manage embedded software packages → install the **STM32G4** package → restart CubeMX. Board then appears in the Board Selector.
**Principle**: CubeMX requires the MCU family firmware package before boards of that family appear. Always install the package first.

---

## 2026-03-03 — Nucleo-G474RE HSE is 24 MHz from ST-LINK MCO, not 8 MHz

**Context**: Configuring the PLL in CubeMX Clock Configuration tab.
**Mistake**: The hardware bringup plan assumed HSE = 8 MHz (common on older Nucleo boards with a dedicated crystal). The NUCLEO-G474RE feeds HSE from the **ST-LINK MCO output at 24 MHz** — there is no discrete 8 MHz crystal on the board.
**Fix**: CubeMX correctly shows HSE input = 24 MHz when the NUCLEO-G474RE board preset is used. PLL config: HSE → PLLM=/4 → 6 MHz → PLLN=×85 → 510 MHz → PLLR=/2 → **170 MHz SYSCLK**.
**Principle**: Always verify the actual HSE source from the board schematic or CubeMX board preset. Nucleo boards use ST-LINK MCO (typically 8 or 24 MHz), not a standalone crystal. The PLL divisors depend on this exact input frequency.

---

## 2026-03-03 — Clock tree: run everything at 170 MHz for simplicity

**Context**: Configuring AHB/APB prescalers and peripheral clock muxes.
**Decision**: All bus clocks set to 170 MHz (AHB=/1, APB1=/1, APB2=/1). All peripheral clock muxes (ADC, I2C, USART, FDCAN) use PLLP/PLLQ/SYSCLK at 170 MHz.
**Rationale**: STM32G474 supports all peripherals at 170 MHz. No need to divide down for this application. Simplifies bit-timing calculations (one clock frequency everywhere). Power consumption is not a concern (Nucleo is USB-powered).
**Principle**: For development/demo boards, maximize clock speed everywhere. Only reduce clocks when power budget requires it.

---

## 2026-03-03 — FDCAN1 default bit timing is wrong: 3.5 Mbps instead of 500 kbps

**Context**: CubeMX FDCAN1 activated with default parameters.
**Mistake**: Default CubeMX values were Prescaler=16, TimeSeg1=1, TimeSeg2=1, which yields:
```
170 MHz / (16 × (1 + 1 + 1)) = 170 MHz / 48 = 3,541,666 bit/s
```
This is ~3.5 Mbps — way above the project's 500 kbps CAN bus specification.
**Fix**: Correct settings for 500 kbps at 80% sample point with 170 MHz FDCAN clock:
```
Prescaler  = 17
TimeSeg1   = 15
TimeSeg2   = 4
SJW        = 4

Baud = 170 MHz / (17 × (1 + 15 + 4)) = 170 MHz / 340 = 500,000 bit/s  ✓
Sample point = (1 + 15) / 20 = 80%  ✓
```
**Principle**: CubeMX defaults for FDCAN are not usable — always manually calculate bit timing from the kernel clock. Use the formula: `Baud = f_CAN / (Prescaler × (1 + Seg1 + Seg2))`. Target 75-80% sample point for automotive CAN. Verify the calculated baud rate matches the displayed value in CubeMX before generating code.

---

## 2026-03-03 — FDCAN1 Auto Retransmission should be enabled

**Context**: CubeMX FDCAN1 Basic Parameters.
**Mistake**: Auto Retransmission defaults to **Disable** in CubeMX.
**Fix**: Set Auto Retransmission to **Enable**. Standard CAN protocol expects automatic retransmission on arbitration loss or error. Disabling it causes frames to be silently dropped.
**Principle**: Always enable Auto Retransmission for normal CAN operation. Only disable for special cases like CAN-based time-triggered protocols (TTCAN) or one-shot diagnostic frames.

---

## 2026-03-03 — FDCAN1 needs Std Filters configured, not left at 0

**Context**: CubeMX FDCAN1 Std Filters Nbr = 0, Ext Filters Nbr = 0.
**Mistake**: With 0 filters, no messages will pass through to the RX FIFO — FDCAN hardware requires at least one acceptance filter to receive any frame.
**Fix**: Set **Std Filters Nbr = 4** (or more) to allow configuring acceptance filters for heartbeat IDs (0x010-0x012), vehicle state (0x100), torque request (0x101), motor status (0x301), etc. The actual filter values are configured in firmware code, but CubeMX must allocate the filter RAM.
**Principle**: FDCAN hardware uses a message RAM with configurable filter elements. Setting filters to 0 in CubeMX means no filter RAM allocated = no reception possible. Always allocate enough filter slots for your CAN ID set.

---

## 2026-03-03 — FDCAN Classic Mode is correct for this project

**Context**: Choosing between Classic CAN and CAN FD frame format.
**Decision**: Frame Format = **Classic mode**. The project uses standard CAN 2.0B (8-byte data, 500 kbps). CAN FD (flexible data rate with up to 64 bytes and faster data phase) is not needed.
**Rationale**: All CAN frames in the project are ≤ 8 bytes. The TJA1051T/3 transceiver supports CAN FD, but the SN65HVD230 on the SC (TMS570) does not. Classic mode ensures bus compatibility across all 4 ECUs.
**Principle**: Use the simplest protocol that meets requirements. CAN FD adds complexity (dual bit-rate timing, larger message RAM) with no benefit when all payloads fit in 8 bytes. Also verify transceiver compatibility across all bus nodes before enabling FD.

---

## Key Takeaways

1. **Board Selector > MCU Selector** for development kits — presets save time and prevent mistakes
2. **HSE frequency varies by board** — never hardcode assumptions; check schematic or CubeMX preset
3. **CubeMX FDCAN defaults are unusable** — always manually calculate bit timing
4. **FDCAN filter count must be > 0** to receive any frames
5. **Auto Retransmission = Enable** for standard CAN operation
6. **Classic CAN unless all transceivers support FD** — bus compatibility across ECUs is non-negotiable
