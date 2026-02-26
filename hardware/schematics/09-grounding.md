# 09 — Grounding

**Block**: Star ground, analog/digital separation
**Source**: HWDES Section 8

## Star Ground Topology

```
  [Star Ground Point] (single screw terminal or copper bus bar on base plate)
         |
    +----+----+----+----+----+----+----+
    |    |    |    |    |    |    |    |
   PSU  CVC  FZC  RZC   SC  Motor Servo
   GND  GND  GND  GND  GND  GND   GND
              |
         [CAN GND]
         (common ref)
```

## Ground Plane and Separation Rules

### 1. Star Ground

All ground returns converge at a single star ground point (screw terminal or bus bar). This prevents ground loops and ensures all ECUs share the same ground reference.

### 2. Analog/Digital Ground Separation

On each ECU's protoboard, the analog ground (ADC reference, sensor returns) and digital ground (MCU, CAN transceiver) shall be separated by at least 5 mm and connected at a single point near the MCU's VSS/VSSA pins.

### 3. ACS723 Ground Reference

The ACS723 analog output ground (GND pin) shall be connected directly to the RZC's analog ground, with a short, low-impedance trace. The ACS723 power path ground (IP- pin) connects to the motor power ground, which is a separate high-current path.

### 4. Motor Power Ground

The motor power return (from BTS7960 B- to PSU GND) shall use 16 AWG wire and connect directly to the star ground point. This high-current path shall not pass through any signal ground connections.

### 5. CAN Bus Ground

A CAN reference ground wire (CAN_GND) shall run alongside the CAN_H/CAN_L twisted pair. This provides a common-mode reference for all CAN transceivers. CAN_GND connects to the star ground point.

### 6. Decoupling Ground Returns

All decoupling capacitor grounds shall connect to the local ground plane/wire within 5 mm of the capacitor.

## Key Notes

- **Star ground prevents ground loops** — critical for ADC accuracy on RZC (current sensing, temperature, battery voltage)
- **Motor ground is isolated** from signal ground until the star point — prevents motor noise from coupling into ADC readings
- **CAN GND provides common-mode reference** — required by ISO 11898-2 for proper differential signaling
- **16 AWG for power returns, 22 AWG for signal returns** — prevents voltage drop on high-current paths

## BOM References

| Component | BOM # |
|-----------|-------|
| Screw terminal 2-position (x10) | #62 |
| Screw terminal 3-position (x6) | #63 |
| 16 AWG wire (red + black) | #68 |
| 18 AWG wire (red + black) | #67 |
| 22 AWG hookup wire kit | #66 |
| Base plate | #59 |
