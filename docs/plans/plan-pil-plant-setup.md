# PIL Plant Setup Plan — Processor-in-the-Loop Integration

**Status**: IN PROGRESS
**Created**: 2026-03-08
**Last Updated**: 2026-03-08
**Traces to**: SWE.5, SYS-053, TSR-046

---

## 1. Overview

PIL (Processor-in-the-Loop) bridges the gap between pure software simulation (SIL) and full hardware-in-the-loop (HIL). In PIL mode:

- **4 physical ECUs** (CVC, FZC, RZC on STM32G474RE + SC on TMS570LC4357) run real firmware on real MCUs, communicating over a physical CAN bus at 500 kbps
- **3 simulated ECUs** (BCM, ICU, TCU) run as Docker containers on a Raspberry Pi, connected to the same CAN bus via a USB-CAN adapter or CAN HAT
- **Plant simulator** runs on the Raspberry Pi, providing physics models (motor, steering, brake, battery, lidar) that generate virtual sensor data based on actuator commands from the physical ECUs

```
+----------+    +----------+    +----------+    +----------+
|   CVC    |    |   FZC    |    |   RZC    |    |    SC    |
| STM32    |    | STM32    |    | STM32    |    | TMS570   |
| (Nucleo) |    | (Nucleo) |    | (Nucleo) |    | (Launch) |
+----+-----+    +----+-----+    +----+-----+    +----+-----+
     |               |               |               |
     +-------+-------+-------+-------+-------+-------+
             |           CAN BUS (500 kbps)           |
             |         120R        120R               |
             +----------------+-----------------------+
                              |
                     +--------+--------+
                     | USB-CAN Adapter |
                     | (CANable/PCAN)  |
                     +--------+--------+
                              |
                     +--------+--------+
                     |  Raspberry Pi   |
                     |  (can0 / vcan0) |
                     |                 |
                     |  Docker:        |
                     |   - BCM         |
                     |   - ICU         |
                     |   - TCU         |
                     |   - plant-sim   |
                     |   - can-gateway |
                     |   - mqtt-broker |
                     |   - ml-inference|
                     |   - ws-bridge   |
                     +-----------------+
```

### SIL vs PIL Comparison

| Aspect | SIL (Pure Software) | PIL (Mixed HW/SW) |
|--------|--------------------|--------------------|
| CVC/FZC/RZC/SC | Docker containers on vcan0 | Physical STM32/TMS570 on real CAN |
| BCM/ICU/TCU | Docker containers on vcan0 | Docker containers on RPi (can0) |
| CAN bus | vcan0 (kernel virtual) | Physical CAN + can0 on RPi |
| Plant simulator | Docker on vcan0 | Docker on RPi, reads/writes can0 |
| MCAL backend | POSIX stubs (*_Posix.c) | STM32 HAL drivers (*_Hw_STM32.c) |
| Real-time | Non-real-time (Linux scheduler) | Real firmware timing on MCU |
| Peripheral I/O | Injected via POSIX APIs | Real GPIO/ADC/SPI/PWM/UART |

---

## 2. What Has Been Done (Completed Infrastructure)

### 2.1 AUTOSAR-Like MCAL Architecture (Link-Time Platform Swap)

The entire BSW stack uses a 3-layer abstraction that enables the same application code to run on both POSIX (SIL/Docker) and STM32 (PIL/HIL):

```
Application SWCs (cvc/fzc/rzc)
        |
    RTE / Com / IoHwAb
        |
    MCAL API (.h files — platform-independent)
        |
    extern Hw_*() functions
        |
   +----+----+
   |         |
Posix.c   STM32.c     ← selected by Makefile at link time
```

- `Makefile.posix` links `*_Posix.c` backends → Docker/SIL
- `Makefile.stm32` links `*_Hw_STM32.c` backends → physical ECU

### 2.2 POSIX MCAL Stubs (SIL — Docker Containers)

All POSIX stubs now have **bidirectional injection/readback APIs** enabling the plant simulator and test harness to observe actuator outputs and inject sensor inputs:

| Module | File | Key APIs |
|--------|------|----------|
| CAN | `mcal/posix/Can_Posix.c` | SocketCAN (vcan0/can0), `Can_Posix_GetFd()` |
| PWM | `mcal/posix/Pwm_Posix.c` | `Pwm_Posix_ReadDuty(ch)`, `Pwm_Posix_InjectDuty(ch, duty)` |
| DIO | `mcal/posix/Dio_Posix.c` | `Dio_Posix_ReadPin(id)`, `Dio_Posix_InjectPin(id, lvl)`, `Dio_Posix_ResetAll()` |
| ADC | `mcal/posix/Adc_Posix.c` | Store/inject ADC group results |
| SPI | `mcal/posix/Spi_Posix.c` | Simulated SPI transfers |
| UART | `mcal/posix/Uart_Posix.c` | Ring buffer RX, `Uart_Posix_InjectRx(data, len)`, `Uart_Posix_FlushRx()` |
| GPT | `mcal/posix/Gpt_Posix.c` | `clock_gettime()` based timers |

### 2.3 STM32 MCAL Drivers (Physical ECUs)

All STM32 hardware drivers are implemented for real peripheral I/O:

| Module | File | Implementation |
|--------|------|----------------|
| CAN | `mcal/target/Can_Hw_STM32.c` | FDCAN1 (PA11/PA12), 500 kbps |
| PWM | `mcal/target/Pwm_Hw_STM32.c` | TIM1 (20kHz motor) / TIM2 (50Hz servo) |
| DIO | `mcal/target/Dio_Hw_STM32.c` | GPIO BSRR atomic writes, IDR reads |
| ADC | `mcal/target/Adc_Hw_STM32.c` | ADC1 + DMA circular, 4ch (PA0-PA3) |
| SPI | `mcal/target/Spi_Hw_STM32.c` | SPI1/SPI2, AS5048A (CPOL=0, CPHA=1, 16-bit) |
| UART | `mcal/target/Uart_Hw_STM32.c` | USART2 + DMA circular RX (TFMini-S) |
| GPT | `mcal/target/Gpt_Hw_STM32.c` | HAL SysTick, 4 software timer channels |

### 2.4 Plant Simulator (`gateway/plant_sim/simulator.py`)

100 Hz physics loop with 5 models:

| Model | File | Physics |
|-------|------|---------|
| Motor | `motor_model.py` | Torque → RPM, current draw, thermal, overcurrent/stall faults |
| Steering | `steering_model.py` | Servo angle tracking with slew rate |
| Brake | `brake_model.py` | Servo position tracking, servo current |
| Battery | `battery_model.py` | Voltage droop under load, SOC depletion |
| Lidar | `lidar_model.py` | Distance + signal strength simulation |

**CAN message schedule (plant-sim TX):**

| CAN ID | Message | Rate | E2E |
|--------|---------|------|-----|
| 0x400 | FZC_Virtual_Sensors (steer angle, brake pos, brake current) | 10ms | No |
| 0x401 | RZC_Virtual_Sensors (motor current, motor temp, battery mV) | 10ms | No |
| 0x220 | Lidar_Distance | 10ms | Yes (DataID=0x0D) |
| 0x500 | DTC_Broadcast | On event | No |

**CAN message schedule (plant-sim RX from ECUs):**

| CAN ID | Message | Source |
|--------|---------|--------|
| 0x001 | E-Stop | CVC |
| 0x101 | Torque_Request | CVC |
| 0x102 | Steer_Command | CVC |
| 0x103 | Brake_Command | CVC |
| 0x013 | SC_Relay_Status | SC |

### 2.5 Docker Compose Platform

13-service SIL platform (`docker/docker-compose.yml`):
- 7 ECU containers (cvc, fzc, rzc, sc, bcm, icu, tcu)
- plant-sim, can-gateway, mqtt-broker, ws-bridge, caddy, sap-qm-mock, ml-inference, fault-inject

### 2.6 STM32 Flash Infrastructure

All 3 Nucleo boards identified by ST-LINK serial:

| ECU | ST-LINK Serial | Flash Command |
|-----|----------------|---------------|
| CVC | 0027003C3235510B37333439 | `make -f Makefile.stm32 flash-cvc` |
| FZC | 001A00363235510B37333439 | `make -f Makefile.stm32 flash-fzc` |
| RZC | 0049002D3235510C37333439 | `make -f Makefile.stm32 flash-rzc` |

---

## 3. PIL Plant Setup — Step by Step

### Phase P0: Prerequisites

**Hardware needed:**
- 3× Nucleo-G474RE (CVC, FZC, RZC) — mounted on bench with CAN bus wired (Phase 3 from hardware bringup)
- 1× LAUNCHXL2-570LC43 (SC) — on same CAN bus
- 1× Raspberry Pi 4 (4GB+ RAM recommended)
- 1× USB-CAN adapter (CANable, PCAN-USB, or similar) OR Waveshare RS485/CAN HAT
- 1× MicroSD card (32GB+) flashed with Raspberry Pi OS (64-bit Lite)
- Ethernet cable or Wi-Fi for Pi

**Software needed on Pi:**
- Docker Engine + Docker Compose (v2)
- can-utils (`sudo apt install can-utils`)
- Git (to clone repo)

**Status**: PENDING

### Phase P1: Raspberry Pi OS Setup

1. Flash Raspberry Pi OS Lite (64-bit, Bookworm) to SD card
2. Enable SSH (create empty `ssh` file in boot partition)
3. Configure Wi-Fi or use Ethernet
4. Boot and connect via SSH:
   ```bash
   ssh pi@<pi-ip-address>
   ```
5. Update system:
   ```bash
   sudo apt update && sudo apt upgrade -y
   ```
6. Install Docker:
   ```bash
   curl -fsSL https://get.docker.com | sh
   sudo usermod -aG docker $USER
   # Log out and back in for group change
   ```
7. Install Docker Compose plugin:
   ```bash
   sudo apt install docker-compose-plugin -y
   docker compose version  # verify
   ```
8. Install can-utils:
   ```bash
   sudo apt install can-utils -y
   ```

**Pass criteria**: `docker compose version` shows v2.x, `candump --help` works

**Status**: PENDING

### Phase P2: USB-CAN Adapter Setup

#### Option A: CANable (gs_usb driver — recommended)

1. Plug CANable into Pi USB port
2. Load driver and bring up interface:
   ```bash
   sudo modprobe gs_usb
   sudo ip link set can0 type can bitrate 500000
   sudo ip link set can0 up
   ```
3. Persist across reboots — create `/etc/network/interfaces.d/can0`:
   ```
   auto can0
   iface can0 inet manual
       pre-up /sbin/ip link set can0 type can bitrate 500000
       up /sbin/ip link set can0 up
       down /sbin/ip link set can0 down
   ```

#### Option B: Waveshare RS485/CAN HAT (SPI-based MCP2515)

1. Enable SPI in `/boot/config.txt`:
   ```
   dtparam=spi=on
   dtoverlay=mcp2515-can0,oscillator=12000000,interrupt=25,spimaxfrequency=2000000
   ```
2. Reboot, then:
   ```bash
   sudo ip link set can0 type can bitrate 500000
   sudo ip link set can0 up
   ```

#### Verification (both options):

1. On the bench, ensure at least one physical ECU is running and sending heartbeats
2. On Pi:
   ```bash
   candump can0
   ```
3. You should see CAN frames from the physical ECUs (heartbeats at 0x010-0x013)

**Pass criteria**: `candump can0` shows heartbeat frames from physical ECUs

**Status**: PENDING

### Phase P3: Clone Repo and Build Docker Images

1. Clone the repository on Pi:
   ```bash
   git clone --recursive https://github.com/<org>/taktflow-embedded.git
   cd taktflow-embedded
   ```

2. Create PIL-specific docker-compose override — `docker/docker-compose.pil.yml`:
   ```yaml
   # PIL override — only simulated ECUs + gateway services
   # Physical ECUs (CVC, FZC, RZC, SC) run on real hardware
   #
   # Usage: docker compose -f docker-compose.yml -f docker-compose.pil.yml up --build
   services:
     # Disable physical ECUs (they run on real hardware)
     cvc:
       profiles: ["sil-only"]
     fzc:
       profiles: ["sil-only"]
     rzc:
       profiles: ["sil-only"]
     sc:
       profiles: ["sil-only"]

     # Override CAN channel: use can0 (physical) instead of vcan0
     bcm:
       environment:
         - CAN_INTERFACE=can0
         - ECU_NAME=bcm
     icu:
       environment:
         - CAN_INTERFACE=can0
         - ECU_NAME=icu
     tcu:
       environment:
         - CAN_INTERFACE=can0
         - ECU_NAME=tcu
     plant-sim:
       environment:
         - CAN_CHANNEL=can0
         - DBC_PATH=/app/taktflow.dbc
         - MQTT_HOST=localhost
         - MQTT_PORT=1883
     can-gateway:
       environment:
         - CAN_CHANNEL=can0
         - DBC_PATH=/app/taktflow.dbc
         - MQTT_HOST=localhost
         - MQTT_PORT=1883
         - SAP_QM_API_URL=http://localhost:8090
     fault-inject:
       environment:
         - CAN_CHANNEL=can0
         - FAULT_PORT=8091
         - MQTT_HOST=localhost
         - MQTT_PORT=1883

     # Override can-setup to use real can0 instead of creating vcan0
     can-setup:
       command: >
         /bin/sh -c "
           if ip link show can0 > /dev/null 2>&1; then
             ip link set can0 up;
             echo '[can-setup] can0 already UP (physical CAN adapter)';
           else
             echo '[can-setup] ERROR: can0 not found — is USB-CAN adapter plugged in?';
             echo '[can-setup] Run: sudo ip link set can0 type can bitrate 500000 && sudo ip link set can0 up';
             exit 1;
           fi;
           ip -d link show can0
         "
   ```

3. Build all images (ARM64 native on Pi):
   ```bash
   cd docker
   docker compose -f docker-compose.yml -f docker-compose.pil.yml build
   ```

**Note**: First build on Pi takes 10-20 minutes (ARM64 compilation of C firmware + Python deps). Subsequent builds use cache.

**Pass criteria**: All images build successfully, `docker compose config` shows correct service configuration

**Status**: PENDING

### Phase P4: Flash Physical ECUs

From the development PC (not the Pi), flash all 3 STM32 boards:

```bash
cd firmware
make -f Makefile.stm32 flash-all
```

Or individually:
```bash
make -f Makefile.stm32 flash-cvc
make -f Makefile.stm32 flash-fzc
make -f Makefile.stm32 flash-rzc
```

SC (TMS570) is flashed separately via CCS/UniFlash.

**Pass criteria**: All 4 ECUs boot, heartbeats visible on `candump can0` from Pi

**Status**: PENDING

### Phase P5: Start PIL Platform

1. Ensure CAN adapter is up:
   ```bash
   ip link show can0  # should show UP
   ```

2. Start Docker services:
   ```bash
   cd docker
   docker compose -f docker-compose.yml -f docker-compose.pil.yml up --build -d
   ```

3. Monitor logs:
   ```bash
   docker compose logs -f plant-sim     # Physics loop
   docker compose logs -f bcm           # Body control
   docker compose logs -f can-gateway   # CAN→MQTT bridge
   ```

4. Verify CAN traffic:
   ```bash
   candump can0  # Should show frames from ALL 7 ECUs + plant-sim
   ```

Expected CAN traffic:

| Source | CAN IDs | Rate |
|--------|---------|------|
| CVC (physical) | 0x001, 0x010, 0x100, 0x101, 0x102, 0x103 | 10-100ms |
| FZC (physical) | 0x011, 0x200, 0x201 | 20-100ms |
| RZC (physical) | 0x012, 0x300, 0x301, 0x302, 0x303 | 10-1000ms |
| SC (physical) | 0x013 | 100ms |
| BCM (Docker) | 0x014 | 100ms |
| ICU (Docker) | 0x015 | 100ms |
| TCU (Docker) | 0x016 | 100ms |
| Plant-sim (Docker) | 0x220, 0x400, 0x401 | 10ms |

**Pass criteria**: All 7 ECU heartbeats present, plant-sim sending virtual sensor data, no CAN errors

**Status**: PENDING

### Phase P6: Validate Closed-Loop Operation

This is the critical test — verifying that the physical ECUs and Docker containers form a working closed loop:

#### Test 1: Pedal → Motor Response

1. CVC reads pedal sensor (physical SPI on STM32)
2. CVC sends Torque_Request (0x101) on physical CAN
3. Plant-sim (Docker) receives 0x101, updates motor model
4. Plant-sim sends RZC_Virtual_Sensors (0x401) with motor current/temp
5. RZC (physical) receives 0x401 via sensor feeder → ADC injection
6. RZC reads motor current via IoHwAb, detects if overcurrent
7. RZC sends Motor_Status (0x300) on CAN

**Verification**: `candump can0 | grep -E "101|401|300"` shows the data flow chain

#### Test 2: Safety Shutdown

1. Press E-stop button on CVC (physical GPIO on STM32)
2. CVC sends E-Stop (0x001) on CAN
3. SC (physical TMS570) receives E-stop, opens kill relay
4. SC sends SC_Relay_Status (0x013)
5. Plant-sim receives 0x013, disables motor model
6. All ECUs transition to SAFE_STOP state

**Verification**: Plant-sim log shows "SC relay KILLED", motor RPM drops to 0

#### Test 3: Lidar Distance Feedback

1. Plant-sim sends Lidar_Distance (0x220) with varying distance
2. FZC (physical) receives 0x220 via CAN
3. FZC processes lidar data in obstacle detection SWC
4. If distance < threshold, FZC requests brake via CAN

**Verification**: Monitor FZC log output, check Brake_Command (0x103) appears when lidar distance is low

#### Test 4: BCM Body Functions (Docker → Physical)

1. BCM (Docker) receives Vehicle_State (0x100) from CVC (physical)
2. BCM processes headlight/indicator logic
3. BCM sends BCM_Status CAN messages

**Verification**: BCM container logs show received vehicle state

#### Test 5: TCU UDS Diagnostics (Docker → Physical)

1. TCU (Docker) sends UDS diagnostic request via CAN (CanTp)
2. Target ECU (physical) processes UDS request in Dcm
3. Target ECU sends UDS response via CAN

**Verification**: TCU container logs show UDS session established

**Status**: PENDING

---

## 4. PIL-Specific Considerations

### 4.1 CAN Bus Timing

Physical ECUs run at real-time tick rates (10ms SchM cycle). Docker containers on Pi run at best-effort Linux scheduling. Expected CAN jitter:

| Source | Expected Jitter |
|--------|----------------|
| Physical ECUs (STM32) | < 1ms (hardware timer interrupt) |
| Docker containers (Pi) | 1-5ms (Linux scheduler + Docker overhead) |
| Plant-sim (Pi) | 1-5ms (asyncio sleep accuracy) |

This jitter is acceptable for PIL — the E2E protection (alive counter + CRC) handles occasional late frames via the state machine's timeout tolerance.

### 4.2 Virtual Sensor Path (PIL vs SIL)

In SIL, the plant-sim injects sensor values directly into POSIX MCAL stubs via function calls. In PIL, the path is different:

**SIL path** (all in-process):
```
Plant-sim → Pwm_Posix_InjectDuty() → IoHwAb → SWC
```

**PIL path** (CAN-based):
```
Plant-sim → CAN 0x400/0x401 → Physical ECU CAN RX → Sensor Feeder SWC → IoHwAb → SWC
```

The physical ECUs have **sensor feeder SWCs** that receive virtual sensor CAN messages (0x400, 0x401) and inject values into the local MCAL. This enables closed-loop simulation without physical sensors attached.

### 4.3 What You Don't Need for PIL

PIL mode does NOT require:
- Physical sensors (pedal, lidar, current sensor) — plant-sim provides virtual values
- Physical actuators (motor, servos) — plant-sim models the physics
- Kill relay circuit — SC sends relay status on CAN, plant-sim disables motor model
- Power distribution beyond USB power for each Nucleo board

PIL validates **firmware logic + real-time behavior + CAN communication** without the full hardware bench.

### 4.4 Transitioning from PIL to HIL

Once PIL is validated, transitioning to full HIL means:
1. Connect physical sensors to the STM32 ADC/SPI/UART pins
2. Connect physical actuators to the PWM/DIO outputs
3. Disable sensor feeder SWCs (or compile them out)
4. Plant-sim becomes optional (real physics replaces simulation)
5. Kill relay circuit must be working (Phase 4 of hardware bringup)

---

## 5. Troubleshooting

### No CAN frames from physical ECUs

1. Check CAN adapter: `ip link show can0` — must be UP
2. Check bitrate: must be 500000 on both Pi and ECUs
3. Check termination: 60 ohm between CAN_H and CAN_L (power off)
4. Check wiring: CAN_H to CAN_H, CAN_L to CAN_L (not crossed)
5. Try: `cansend can0 000#DEADBEEF` — should appear on physical ECU candump

### Docker containers can't see CAN

1. Verify `network_mode: host` in compose file
2. Verify `cap_add: NET_ADMIN, NET_RAW`
3. Test from inside container: `docker exec -it <container> candump can0`

### Plant-sim not receiving actuator commands

1. Check plant-sim logs: `docker compose logs plant-sim`
2. Verify CAN IDs match: plant-sim expects 0x101, 0x102, 0x103 from physical ECUs
3. Check CAN_CHANNEL env var: must be `can0` (not `vcan0`) in PIL mode

### High CAN error rate

1. Check `ip -s link show can0` — look for TX/RX errors
2. Reduce bus load: disable non-essential 10ms messages during debug
3. Check for ground loop: all boards should share common ground
4. Check stub lengths: CAN stubs to each board must be < 100mm

---

## 6. Phase Summary

| Phase | Description | Pass Criteria | Status |
|-------|-------------|---------------|--------|
| P0 | Prerequisites | All hardware and software available | PENDING |
| P1 | RPi OS setup | Docker + can-utils installed | PENDING |
| P2 | USB-CAN adapter | `candump can0` shows physical ECU frames | PENDING |
| P3 | Clone + build | Docker images built on Pi | PENDING |
| P4 | Flash ECUs | All 4 ECUs boot and send heartbeats | PENDING |
| P5 | Start platform | All 7 ECUs + plant-sim running | PENDING |
| P6 | Closed-loop test | Pedal→motor, E-stop, lidar feedback working | PENDING |

---

## 7. Files Reference

| File | Purpose |
|------|---------|
| `firmware/Makefile.posix` | POSIX (SIL) build — links *_Posix.c backends |
| `firmware/Makefile.stm32` | STM32 (PIL/HIL) build — links *_Hw_STM32.c backends |
| `firmware/shared/bsw/mcal/posix/` | All POSIX MCAL stubs with injection/readback APIs |
| `firmware/shared/bsw/mcal/target/` | All STM32 HAL-based MCAL drivers |
| `docker/docker-compose.yml` | Full SIL platform (13 services) |
| `docker/docker-compose.pil.yml` | PIL override (disables physical ECU containers) |
| `gateway/plant_sim/simulator.py` | Plant physics simulator (100 Hz) |
| `gateway/can_gateway/main.py` | CAN → MQTT bridge |
| `gateway/taktflow.dbc` | CAN database (message definitions) |
| `docs/plans/plan-hardware-bringup.md` | Hardware assembly plan (Phases 0-7) |
| `docs/aspice/verification/xil/pil-report.md` | PIL test report (to be filled after execution) |
