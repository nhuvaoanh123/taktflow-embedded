---
paths:
  - "hardware/**/*"
  - "firmware/src/**/*"
  - "firmware/include/**/*"
---

# Hardware Design & Abstraction Standards

## Hardware Abstraction Layer (HAL)

- ALL hardware access MUST go through an abstraction layer
- Direct register manipulation only inside HAL drivers — never in application code
- One HAL module per peripheral: `hal_uart.c`, `hal_spi.c`, `hal_gpio.c`, etc.
- HAL interface defined in headers (`hal_uart.h`) — implementation is platform-specific
- Swapping hardware platform = rewrite HAL, not application code

## Pin Mapping

- Define ALL pin assignments in a single file: `include/pin_map.h` or equivalent
- Use descriptive names: `PIN_LED_STATUS`, `PIN_SENSOR_CS`, `PIN_MOTOR_PWM`
- Never use raw pin numbers in application code
- Document pin assignments in `hardware/pin-mapping.md`
- Include physical connector/header reference for each pin

## GPIO

- Configure unused pins as inputs with pull-down (reduce power, prevent floating)
- Debounce all mechanical switch/button inputs (minimum 20ms)
- Document active-high vs active-low for every digital signal
- Use interrupts for event-driven signals, polling for periodic reads
- Protect outputs with current limiting — don't exceed pin source/sink rating

## ADC / Analog

- Calibrate ADC on boot or use factory calibration values
- Apply averaging/filtering to reduce noise (moving average, median filter)
- Define valid range for each analog input — reject out-of-range readings
- Account for voltage divider ratios in conversion functions
- Document reference voltage and resolution assumptions

## Communication Buses

### UART/Serial
- Define baud rate, parity, stop bits as configuration — not magic numbers
- Implement receive timeout — don't block forever
- Use DMA for high-throughput serial when available
- Implement framing protocol (start byte, length, CRC, end byte)

### SPI
- Document clock polarity (CPOL) and phase (CPHA) for each device
- Use chip select (CS) properly — assert before, deassert after
- Verify SPI clock speed is within device specification
- Handle bus contention when multiple devices share SPI bus

### I2C
- Implement bus recovery (clock stretching timeout, bus reset)
- Check for ACK/NACK on every transaction
- Use appropriate pull-up resistor values for bus speed and capacitance
- Document device addresses in central registry

## Power Management

- Define power states: ACTIVE, IDLE, LIGHT_SLEEP, DEEP_SLEEP, HIBERNATE
- Document current consumption for each power state
- Implement orderly shutdown sequence (save state, close connections, then sleep)
- Wake sources must be documented and tested
- Validate power budget against battery/supply specifications

## Schematic & PCB Standards

- All schematics must include revision number and date
- Document bill of materials (BOM) with manufacturer part numbers
- Include test points for critical signals
- Label all connectors and headers clearly
- Include power supply decoupling capacitors (follow manufacturer recommendations)
- Document board-level protections (ESD, reverse polarity, overcurrent)
