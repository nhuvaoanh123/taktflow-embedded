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

# MCAL — Microcontroller Abstraction Layer

Wraps MCU peripherals with hardware-independent API.

| Module | Wraps | Platforms |
|--------|-------|-----------|
| Can | FDCAN (STM32), DCAN (TMS570), SocketCAN (POSIX) | All |
| Spi | SPI HAL | STM32 |
| Adc | ADC DMA | STM32 |
| Pwm | Timer PWM | STM32 |
| Dio | GPIO | STM32, TMS570 |
| Gpt | SysTick | STM32, TMS570 |

Phase 5 deliverable.

