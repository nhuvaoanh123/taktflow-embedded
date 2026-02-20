---
paths:
  - "hardware/**/*"
  - "firmware/**/*"
---

# ASIL D Hardware Constraints (ISO 26262 Part 5)

## Hardware Architectural Metrics

| Metric | ASIL B | ASIL C | ASIL D |
|--------|--------|--------|--------|
| **SPFM** (Single Point Fault Metric) | >= 90% | >= 97% | **>= 99%** |
| **LFM** (Latent Fault Metric) | >= 60% | >= 80% | **>= 90%** |
| **PMHF** (Probabilistic Metric for HW Failure) | < 100 FIT | < 100 FIT | **< 10 FIT** |

FIT = Failures In Time = 1 failure per 10^9 device-hours

### What These Mean Practically

- **SPFM >= 99%**: No more than 1% of hardware faults can be undetected single-point faults. Requires extensive diagnostic safety mechanisms on every safety-critical component.
- **LFM >= 90%**: No more than 10% of multiple-point faults can remain latent. Requires periodic self-tests (BIST), startup diagnostics, and runtime monitoring.
- **PMHF < 10 FIT**: Random hardware failure rate causing safety goal violation must be below 10 per billion hours. Typically requires redundancy — individual components rarely achieve this alone.

## Diagnostic Coverage Requirements

| DC Level | Coverage Range |
|----------|---------------|
| Low | 60% <= DC < 90% |
| Medium | 90% <= DC < 99% |
| **High** | **DC >= 99%** (expected at ASIL D) |

At ASIL D, high diagnostic coverage (>= 99%) is expected for all safety-critical hardware elements.

## Fault Classification

| Category | Symbol | Description |
|----------|--------|-------------|
| Safe fault | lambda_S | Does not contribute to safety goal violation |
| Single-point fault | lambda_SPF | Directly violates safety goal, no safety mechanism |
| Residual fault | lambda_RF | Part of fault not covered by safety mechanism |
| Detected multi-point | lambda_MPF_det | Multi-point fault detected by safety mechanism |
| Perceived multi-point | lambda_MPF_per | Multi-point fault perceived by driver |
| Latent multi-point | lambda_MPF_lat | Neither detected nor perceived |

## Redundancy Architectures

| Architecture | Description | Use Case |
|-------------|-------------|----------|
| **1oo1D** | Single channel + diagnostics | Lower ASIL; ASIL D only with very high DC |
| **1oo2** | Two independent channels, either sufficient | ASIL D fail-safe systems |
| **1oo2D** | Two channels + diagnostics each | ASIL D standard architecture |
| **2oo3** | Three channels, majority voting | ASIL D fail-operational (steer/brake-by-wire) |

## Safety Mechanism Requirements

### Processor Monitoring
- Lockstep CPU (dual-core running same code, outputs compared)
- OR: independent monitoring processor (checker)
- Program counter monitoring
- Instruction execution checking

### Memory Protection
- ECC (Error Correcting Code) on all safety-critical RAM
- ECC or CRC on flash/ROM
- Memory BIST (Built-In Self Test) at startup
- Periodic memory integrity checks during operation
- MPU to enforce partition isolation

### Communication Monitoring
- CRC on all bus communications (CAN, SPI, etc.)
- Message alive counters / sequence numbers
- Timeout monitoring for periodic messages
- Bus-off detection and recovery

### Power Supply Monitoring
- Voltage monitoring with independent comparator
- Brown-out detection and reset
- Over-voltage protection
- Power supply sequencing verification at startup

### Clock Monitoring
- Independent clock source for watchdog
- Clock frequency monitoring (cross-check main vs reference clock)
- Clock loss detection

### ADC/Sensor Monitoring
- ADC self-test (known reference voltage check)
- Sensor plausibility checking (range, rate-of-change, cross-correlation)
- Open/short circuit detection on analog inputs

## FMEA / FMEDA Requirements

- **FMEA** (qualitative): Identify failure modes, effects, causes for every component
- **FMEDA** (quantitative): Classify each failure mode, assign failure rates, calculate SPFM/LFM/PMHF
- Failure rates from recognized sources: IEC TR 62380, SN 29500, MIL-HDBK-217, or field data
- Update FMEDA when design changes

## Common Cause Failure (CCF) Analysis

Evaluate whether redundant channels can fail simultaneously due to:
- Shared power supply
- Physical proximity (temperature, vibration, EMC)
- Common design methodology
- Shared manufacturing process
- Shared software/firmware
- Environmental stress (ESD, humidity)

ISO 26262 Annex F provides a checklist approach. Mitigations include:
- Diverse hardware (different manufacturers, technologies)
- Physical separation
- Independent power supplies
- Environmental shielding

## Hardware-Software Interface (HSI) Specification

The HSI document MUST cover:

| Item | Description |
|------|-------------|
| RAM mapping | Safety-critical vs non-safety memory regions |
| Non-volatile memory | Flash/EEPROM layout, wear leveling |
| Bus interfaces | CAN, LIN, SPI, I2C — speeds, protocols, error handling |
| ADC/DAC | Channels, resolution, reference voltage, sampling rates |
| PWM | Channels, frequency, duty cycle ranges |
| GPIO | Pin assignments, directions, pull-up/pull-down, interrupt config |
| Interrupts | Assignments, priorities, nesting rules |
| Clock sources | Main, RTC, PLL configuration |
| Diagnostic resources | Watchdog, ECC, MPU, temperature sensor |
| Power modes | Sleep states, wake sources, transition timing |

The HSI is the last system-level artifact before parallel HW/SW development begins.
