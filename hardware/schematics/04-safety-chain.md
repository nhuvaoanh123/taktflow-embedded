# 04 — Safety Chain

**Block**: Kill relay, E-stop, fault LEDs
**Source**: HWDES Sections 5.4.2–5.4.3, 5.1.4

## Kill Relay Circuit

<!-- DECISION: ADR-003 — Energize-to-run relay pattern -->

```
  12V Main Rail
    |
    +---[Relay Coil (12V, 70-120 ohm)]---+
    |                                      |
    |  [1N4007 flyback diode]              |
    |  cathode (+) ---- anode (-)          |
    |     |                |               |
    +-----+                +--- Drain      |
                                |          |
                           [IRLZ44N       ]
                           [ N-MOSFET     ]
                                |          |
                           Source --- GND  |
                                |          |
                           Gate            |
                                |          |
                          [100R series]    |
                                |          |
  SC GIO_A0 ----+--- [10k pulldown] --- GND
                |
            (3.3V output)

  Relay Contact (SPST-NO):
  +-----------+         +-----------+
  | 12V Main  |---NO---+---CLOSED--| 12V Actuator Rail
  | Rail      |   (open when       | (to BTS7960, servos)
  +-----------+    de-energized)   +-----------+
                                        |
                                    [30A Fuse]
```

### Operation

- GIO_A0 = HIGH -> MOSFET ON -> Coil energized -> Relay CLOSED -> Actuator power ON
- GIO_A0 = LOW  -> MOSFET OFF -> Coil de-energized -> Relay OPEN -> Actuator power OFF
- SC power loss -> Gate floats -> 10k pulldown -> MOSFET OFF -> **SAFE STATE**
- SC firmware hang -> TPS3823 reset -> GIO_A0 resets to input -> 10k pulldown -> **SAFE STATE**

### Key Specs

- Dropout timing: < 10 ms from GIO_A0 LOW to relay contacts open
- 1N4007 flyback diode limits coil back-EMF to ~0.7V above rail
- IRLZ44N: VGS(th) < 2V (driven directly by 3.3V GIO output)
- 100R gate resistor limits dI/dt during switching
- 10k pulldown ensures OFF state during all fault conditions

## E-Stop Button Circuit

```
  3.3V (internal pull-up on PC13, ~40k)
    |
    +--- PC13 (EXTI, rising-edge) ---[10k series R]---+--- [100nF] --- GND
    |                                                  |    (RC debounce)
    |                                              [TVS 3.3V]
    |                                                  |
    +--- [NC Push Button] --- GND                     GND

  Resting state: button closed -> PC13 = LOW (connected to GND through button)
  Button pressed: button opens -> PC13 = HIGH (pulled up by internal pull-up)
  Broken wire: PC13 = HIGH (E-stop activated) -- FAIL-SAFE

  Rising edge triggers EXTI interrupt.
  RC debounce: tau = 10k * 100nF = 1 ms.
  TVS diode protects PC13 from static on button wiring.
```

## Fault LED Circuit

```
  SC GIO Pins                    LEDs

  GIO_A1 ---[330R]--- LED_CVC (Red, 3mm)    --- GND
  GIO_A2 ---[330R]--- LED_FZC (Red, 3mm)    --- GND
  GIO_A3 ---[330R]--- LED_RZC (Red, 3mm)    --- GND
  GIO_A4 ---[330R]--- LED_SYS (Amber, 3mm)  --- GND

  GIO_B1 ---[330R]--- LED_HB (Green, 3mm)   --- GND  (heartbeat/status)

  LED current: I = (3.3V - 1.8V) / 330R = 4.5 mA (sufficient for visibility).
  TMS570 GIO pins can source up to 8 mA per pin (4.5 mA is within limits).
  All LEDs OFF at power-up (GIO pins initialized as inputs = high-impedance).
  After init, GIO pins configured as outputs, initialized LOW (LEDs OFF).
  Lamp test during startup: all LEDs ON for 500 ms.
```

## Pin References

| Function | ECU | Pin | Direction | Net Name |
|----------|-----|-----|-----------|----------|
| Kill relay control | SC | GIO_A0 | OUT | SC_KILL_RELAY |
| CVC fault LED | SC | GIO_A1 | OUT | SC_LED_CVC |
| FZC fault LED | SC | GIO_A2 | OUT | SC_LED_FZC |
| RZC fault LED | SC | GIO_A3 | OUT | SC_LED_RZC |
| System fault LED | SC | GIO_A4 | OUT | SC_LED_SYS |
| Heartbeat LED | SC | GIO_B1 | OUT | SC_LED_HB |
| E-stop input | CVC | PC13 | IN | CVC_ESTOP |

## BOM References

| Component | BOM # |
|-----------|-------|
| 30A automotive relay | #24 |
| IRLZ44N MOSFET | #25 |
| 1N4007 flyback diodes (x2) | #26 |
| E-stop button (NC, mushroom) | #27 |
| LED 3mm red (x5) | #29 |
| LED 3mm green (x5) | #30 |
| LED 3mm amber (x2) | #31 |
| 330 ohm resistors | #51 |
| 100 ohm resistors | #52 |
| 10k ohm resistors | #49 |
| 100nF capacitors | #43 |
| 3.3V TVS diodes | #56 |
| Relay socket | #72 |
