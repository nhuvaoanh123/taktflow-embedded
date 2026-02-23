---
document_id: VECU-ARCH
title: "Virtual ECU Architecture"
version: "1.0"
status: draft
aspice_process: SWE.2
date: 2026-02-21
---

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


# Virtual ECU Architecture

## 1. Purpose

This document defines the architecture for the simulated ECU platform (vECU) used in the Taktflow Zonal Vehicle Platform. The vECU platform enables three ECUs (BCM, ICU, TCU) to run as Docker containers on a development PC or CI/CD server, using the same C source code as physical ECUs with a POSIX-based MCAL backend. This enables Software-in-the-Loop (SIL) testing and continuous integration without physical hardware.

## 2. Referenced Documents

| Document ID | Title | Version |
|-------------|-------|---------|
| SYSARCH | System Architecture | 1.0 |
| SW-ARCH | Software Architecture | 0.1 |
| BSW-ARCH | BSW Architecture | 0.1 |
| SYSREQ | System Requirements Specification | 1.0 |

## 3. Scope

### 3.1 In Scope

- BCM (Body Control Module): Automatic headlights, turn indicators, hazard lights, door lock simulation
- ICU (Instrument Cluster Unit): ncurses terminal dashboard displaying speed, temperature, voltage, warnings, DTC status
- TCU (Telematics Control Unit): UDS diagnostic server, DTC storage, OBD-II PID handler
- POSIX MCAL backend: SocketCAN, timers, GPIO stubs, ADC stubs
- Docker containerization: per-ECU containers, docker-compose orchestration
- CAN bridge: vcan0 for SIL testing, real CAN bridge via CANable 2.0 for mixed testing

### 3.2 Out of Scope

- Real sensor/actuator simulation (vECUs do not model physical plant behavior)
- Timing-accurate scheduling (vECU scheduling uses Linux timers, not RTOS-precise)
- Safety-critical execution (all vECUs are QM; no ASIL allocation)
- AUTOSAR Adaptive / SOME/IP (supplementary demo feature, not primary interface)

### 3.3 ASIL Classification

All simulated ECUs are classified as QM (Quality Management). They do not contribute to any safety goal. Safety-critical functions (pedal sensing, motor control, steering, braking, safety monitoring) are exclusively implemented on the physical ECUs.

---

## 4. Architecture Overview

### 4.1 Platform Abstraction Stack

```
+--------------------------------------------------+
|           Application SWCs (SAME C CODE)          |
|  Swc_Lights, Swc_Indicators, Swc_DoorLock  (BCM) |
|  Swc_Dashboard, Swc_DtcDisplay             (ICU) |
|  UDS Server, DTC Store, OBD-II Handler      (TCU) |
+--------------------------------------------------+
|                  RTE (SAME C CODE)                |
|  Rte_Read(), Rte_Write(), Rte_Call()              |
+--------------------------------------------------+
|            BSW Services (SAME C CODE)             |
|  Com, Dcm, Dem, BswM, E2E                        |
+--------------------------------------------------+
|             EcuAL (SAME C CODE)                   |
|  CanIf, PduR, IoHwAb                              |
+--------------------------------------------------+
|         MCAL (PLATFORM-SPECIFIC)                  |
|  +-------------------------------------------+   |
|  | STM32 Target:    | POSIX Target:           |   |
|  | Can_STM32.c      | Can_Posix.c             |   |
|  | Spi_STM32.c      | Spi_Posix.c (stub)      |   |
|  | Adc_STM32.c      | Adc_Posix.c (stub)      |   |
|  | Pwm_STM32.c      | Pwm_Posix.c (stub)      |   |
|  | Dio_STM32.c      | Dio_Posix.c (stub)      |   |
|  | Gpt_STM32.c      | Gpt_Posix.c             |   |
|  +-------------------------------------------+   |
+--------------------------------------------------+
|              Hardware / OS Layer                   |
|  +-------------------------------------------+   |
|  | STM32 Target:    | POSIX Target:           |   |
|  | STM32G474RE      | Linux (Ubuntu 22.04)    |   |
|  | HAL Registers    | SocketCAN (vcan0/can0)  |   |
|  | FDCAN Peripheral | POSIX Timers            |   |
|  +-------------------------------------------+   |
+--------------------------------------------------+
```

### 4.2 Key Design Principle

**Same application code, different MCAL**. The application SWCs, RTE, BSW services, and EcuAL layers are identical between physical and simulated ECUs. Only the MCAL layer differs:

- Physical ECUs use STM32 HAL-based MCAL drivers that access hardware registers
- Simulated ECUs use POSIX-based MCAL drivers that use Linux system calls

This enables:
1. Application code developed and tested on Linux (fast iteration)
2. Same binary logic validated on physical hardware (confidence)
3. SIL tests that exercise the full BSW stack without hardware

---

## 5. MCAL Abstraction Layer (POSIX Backend)

### 5.1 Can_Posix.c -- SocketCAN Backend

The CAN MCAL for POSIX targets implements the AUTOSAR CAN driver API using Linux SocketCAN:

```c
/* Can_Posix.c -- SocketCAN implementation of AUTOSAR CAN driver API */

#include "Can.h"
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

static int can_socket = -1;

/**
 * @brief  Initialize CAN driver using SocketCAN
 * @param  ConfigPtr  Configuration (contains CAN interface name)
 */
void Can_Init(const Can_ConfigType* ConfigPtr) {
    struct sockaddr_can addr;
    struct ifreq ifr;

    can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    /* Error handling omitted for brevity -- see actual implementation */

    strncpy(ifr.ifr_name, ConfigPtr->interface_name, IFNAMSIZ - 1);
    /* Default: "vcan0" for SIL testing, "can0" for real CAN bridge */
    ioctl(can_socket, SIOCGIFINDEX, &ifr);

    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    bind(can_socket, (struct sockaddr *)&addr, sizeof(addr));
}

/**
 * @brief  Transmit a CAN frame via SocketCAN
 * @param  Hth       Hardware transmit handle (mapped to CAN ID)
 * @param  PduInfo   PDU containing CAN ID, DLC, and data pointer
 * @return CAN_OK on success, CAN_BUSY if socket not ready
 */
Can_ReturnType Can_Write(Can_HwHandleType Hth, const Can_PduType* PduInfo) {
    struct can_frame frame;
    frame.can_id = PduInfo->id;
    frame.can_dlc = PduInfo->length;
    memcpy(frame.data, PduInfo->sdu, PduInfo->length);

    ssize_t nbytes = write(can_socket, &frame, sizeof(frame));
    return (nbytes == sizeof(frame)) ? CAN_OK : CAN_BUSY;
}

/**
 * @brief  Main function -- poll for received CAN frames
 *         Called periodically from the scheduler (e.g., every 1 ms)
 */
void Can_MainFunction_Read(void) {
    struct can_frame frame;
    /* Non-blocking read using MSG_DONTWAIT */
    ssize_t nbytes = recv(can_socket, &frame, sizeof(frame), MSG_DONTWAIT);
    if (nbytes == sizeof(frame)) {
        /* Forward to CanIf via the standard callback */
        CanIf_RxIndication(/* mailbox */ 0, &frame);
    }
}
```

### 5.2 Gpt_Posix.c -- POSIX Timer Backend

The general-purpose timer MCAL for POSIX targets uses `clock_nanosleep` and `timer_create` for periodic scheduling:

```c
/* Gpt_Posix.c -- POSIX timer implementation */

#include "Gpt.h"
#include <time.h>
#include <signal.h>

static timer_t gpt_timer;
static Gpt_NotificationType gpt_callback = NULL;

static void gpt_signal_handler(int sig, siginfo_t *si, void *uc) {
    (void)sig; (void)si; (void)uc;
    if (gpt_callback != NULL) {
        gpt_callback();
    }
}

/**
 * @brief  Initialize GPT using POSIX timer
 */
void Gpt_Init(const Gpt_ConfigType* ConfigPtr) {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = gpt_signal_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGRTMIN, &sa, NULL);

    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    timer_create(CLOCK_MONOTONIC, &sev, &gpt_timer);
}

/**
 * @brief  Start periodic timer
 * @param  Channel  Timer channel (unused in POSIX -- single timer)
 * @param  Value    Period in microseconds
 */
void Gpt_StartTimer(Gpt_ChannelType Channel, Gpt_ValueType Value) {
    struct itimerspec its;
    its.it_value.tv_sec = Value / 1000000;
    its.it_value.tv_nsec = (Value % 1000000) * 1000;
    its.it_interval = its.it_value;
    timer_settime(gpt_timer, 0, &its, NULL);
}

/**
 * @brief  Register periodic callback
 */
void Gpt_EnableNotification(Gpt_ChannelType Channel) {
    /* Callback is set during configuration */
}
```

### 5.3 Dio_Posix.c -- GPIO Stub

The GPIO MCAL for POSIX targets provides a memory-mapped stub. GPIO reads/writes operate on a static array, allowing SWCs to set/read simulated GPIO values:

```c
/* Dio_Posix.c -- GPIO stub for simulated ECUs */

#include "Dio.h"

/* Simulated GPIO state: 256 channels, default all LOW */
static Dio_LevelType dio_state[256] = {0};

Dio_LevelType Dio_ReadChannel(Dio_ChannelType ChannelId) {
    return dio_state[ChannelId];
}

void Dio_WriteChannel(Dio_ChannelType ChannelId, Dio_LevelType Level) {
    dio_state[ChannelId] = Level;
}
```

### 5.4 Adc_Posix.c -- ADC Stub

The ADC MCAL for POSIX targets returns configurable simulated values, allowing test scripts to inject sensor data:

```c
/* Adc_Posix.c -- ADC stub for simulated ECUs */

#include "Adc.h"

/* Simulated ADC values: 16 channels, default midscale (2048 for 12-bit) */
static Adc_ValueGroupType adc_values[16] = {
    2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
    2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048
};

void Adc_Init(const Adc_ConfigType* ConfigPtr) {
    /* No hardware initialization needed */
}

Std_ReturnType Adc_ReadGroup(Adc_GroupType Group, Adc_ValueGroupType* DataBufferPtr) {
    *DataBufferPtr = adc_values[Group];
    return E_OK;
}

/* Test injection function -- not part of AUTOSAR API */
void Adc_Posix_InjectValue(Adc_GroupType Channel, Adc_ValueGroupType Value) {
    adc_values[Channel] = Value;
}
```

### 5.5 Pwm_Posix.c and Spi_Posix.c

These are minimal stubs that log PWM duty cycle changes and SPI transactions to stdout/syslog. Simulated ECUs (BCM, ICU, TCU) do not use PWM or SPI in their application logic, so these stubs exist only for API completeness.

---

## 6. Docker Container Structure

### 6.1 Base Image

```dockerfile
# docker/Dockerfile.base
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    make \
    can-utils \
    libsocketcan-dev \
    iproute2 \
    net-tools \
    && rm -rf /var/lib/apt/lists/*

# Create application directory
WORKDIR /app
```

### 6.2 Per-ECU Dockerfile (BCM Example)

```dockerfile
# docker/Dockerfile.bcm
FROM taktflow/vecu-base:latest

# Copy shared BSW source
COPY firmware/shared/bsw/ /app/bsw/

# Copy BCM application source
COPY firmware/bcm/src/ /app/src/
COPY firmware/bcm/include/ /app/include/
COPY firmware/bcm/cfg/ /app/cfg/

# Copy POSIX MCAL implementations
COPY firmware/shared/bsw/mcal/posix/ /app/bsw/mcal/posix/

# Build BCM for POSIX target
RUN make -C /app PLATFORM=posix TARGET=bcm

# Run BCM application
ENTRYPOINT ["/app/build/bcm_posix"]
```

### 6.3 docker-compose.yml

```yaml
# docker/docker-compose.yml
version: "3.8"

services:
  bcm:
    build:
      context: ..
      dockerfile: docker/Dockerfile.bcm
    container_name: taktflow-bcm
    network_mode: host
    cap_add:
      - NET_ADMIN
    volumes:
      - /tmp/.X11-unix:/tmp/.X11-unix  # For GUI access if needed
    environment:
      - CAN_INTERFACE=vcan0
      - ECU_ID=BCM
    depends_on:
      - can-setup
    restart: unless-stopped

  icu:
    build:
      context: ..
      dockerfile: docker/Dockerfile.icu
    container_name: taktflow-icu
    network_mode: host
    cap_add:
      - NET_ADMIN
    environment:
      - CAN_INTERFACE=vcan0
      - ECU_ID=ICU
      - TERM=xterm-256color
    depends_on:
      - can-setup
    restart: unless-stopped

  tcu:
    build:
      context: ..
      dockerfile: docker/Dockerfile.tcu
    container_name: taktflow-tcu
    network_mode: host
    cap_add:
      - NET_ADMIN
    environment:
      - CAN_INTERFACE=vcan0
      - ECU_ID=TCU
    depends_on:
      - can-setup
    restart: unless-stopped

  can-setup:
    image: ubuntu:22.04
    container_name: taktflow-can-setup
    network_mode: host
    cap_add:
      - NET_ADMIN
    command: >
      sh -c "
        apt-get update && apt-get install -y iproute2 can-utils &&
        ip link add vcan0 type vcan 2>/dev/null || true &&
        ip link set vcan0 up &&
        echo 'vcan0 is up' &&
        sleep infinity
      "
    restart: unless-stopped
```

---

## 7. CAN Bridge Configuration

### 7.1 Virtual CAN (vcan0) Setup -- SIL Testing

For pure-software testing without physical CAN hardware:

```bash
# Create and enable virtual CAN interface
sudo ip link add dev vcan0 type vcan
sudo ip link set vcan0 up

# Verify
ip link show vcan0
# Output: vcan0: <NOARP,UP,LOWER_UP> ...

# Monitor CAN traffic
candump vcan0

# Send test frame
cansend vcan0 100#DEADBEEF
```

### 7.2 Real CAN Bridge -- Mixed HW+SW Testing

For testing simulated ECUs alongside physical ECUs on the real CAN bus:

```bash
# Configure CANable 2.0 (with candleLight firmware, gs_usb driver)
sudo ip link set can0 type can bitrate 500000
sudo ip link set can0 up

# Bridge: forward all real CAN traffic to vcan0 (bidirectional)
# Terminal 1: real -> virtual
candump can0 | canplayer vcan0=can0 &

# Terminal 2: virtual -> real
candump vcan0 | canplayer can0=vcan0 &

# Alternative: use cangw for kernel-space bridging (lower latency)
sudo modprobe can-gw
sudo cangw -A -s can0 -d vcan0 -e
sudo cangw -A -s vcan0 -d can0 -e
```

### 7.3 CANable 2.0 Setup

```bash
# Flash candleLight firmware (one-time, using dfu-util)
# CANable 2.0 comes with candleLight firmware by default

# Verify gs_usb driver loads
dmesg | grep gs_usb
# Output: gs_usb: ... device

# The interface appears as can0
ip link show can0

# Configure and bring up
sudo ip link set can0 type can bitrate 500000
sudo ip link set can0 up
```

---

## 8. Build System

### 8.1 Platform Selection

The build system uses a `PLATFORM` variable to select the MCAL backend:

```makefile
# Makefile (top-level)
PLATFORM ?= stm32    # Default: physical target

ifeq ($(PLATFORM),posix)
    CC = gcc
    MCAL_DIR = firmware/shared/bsw/mcal/posix
    CFLAGS += -DPLATFORM_POSIX
    LDFLAGS += -lpthread -lrt
else ifeq ($(PLATFORM),stm32)
    CC = arm-none-eabi-gcc
    MCAL_DIR = firmware/shared/bsw/mcal/stm32
    CFLAGS += -DPLATFORM_STM32 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16
endif

# Common source (same for both platforms)
COMMON_SRC = firmware/shared/bsw/ecual/*.c \
             firmware/shared/bsw/services/*.c \
             firmware/shared/bsw/rte/*.c

# MCAL source (platform-specific)
MCAL_SRC = $(MCAL_DIR)/*.c

# Per-ECU application source
BCM_SRC = firmware/bcm/src/*.c
ICU_SRC = firmware/icu/src/*.c
TCU_SRC = firmware/tcu/src/*.c
```

### 8.2 Build Commands

```bash
# Build all simulated ECUs for POSIX target
PLATFORM=posix make build-bcm build-icu build-tcu

# Build a single simulated ECU
PLATFORM=posix make build-bcm

# Build all physical ECUs for STM32 target (default)
make build-cvc build-fzc build-rzc

# Build Docker images
cd docker && docker-compose build

# Run all simulated ECUs
cd docker && docker-compose up -d

# Stop all simulated ECUs
cd docker && docker-compose down
```

### 8.3 Conditional Compilation

The `PLATFORM_POSIX` / `PLATFORM_STM32` defines allow platform-specific code paths when unavoidable:

```c
#ifdef PLATFORM_POSIX
    /* Use clock_nanosleep for timing */
    #include <time.h>
    static void delay_ms(uint32_t ms) {
        struct timespec ts = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000 };
        clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
    }
#else
    /* Use HAL_Delay on STM32 */
    #include "stm32g4xx_hal.h"
    static void delay_ms(uint32_t ms) {
        HAL_Delay(ms);
    }
#endif
```

The goal is to minimize `#ifdef` blocks. Platform differences should be isolated to the MCAL layer. Application SWCs and BSW services should compile without any `#ifdef PLATFORM_*` directives.

---

## 9. CI/CD Integration

### 9.1 GitHub Actions Workflow Concept

```yaml
# .github/workflows/sil-test.yml
name: SIL Test (Simulated ECUs)

on:
  push:
    branches: [develop, feature/*]
  pull_request:
    branches: [develop]

jobs:
  sil-test:
    runs-on: ubuntu-22.04

    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Install CAN tools
        run: |
          sudo apt-get update
          sudo apt-get install -y can-utils iproute2 build-essential

      - name: Setup vcan0
        run: |
          sudo modprobe vcan
          sudo ip link add dev vcan0 type vcan
          sudo ip link set vcan0 up

      - name: Build vECUs
        run: PLATFORM=posix make build-bcm build-icu build-tcu

      - name: Start vECUs in background
        run: |
          ./build/bcm_posix &
          ./build/icu_posix &
          ./build/tcu_posix &
          sleep 2  # Wait for initialization

      - name: Run SIL tests
        run: |
          cd test/sil
          python3 run_sil_tests.py --interface vcan0

      - name: Collect results
        if: always()
        run: |
          # Stop background processes
          kill %1 %2 %3 2>/dev/null || true
          # Collect test results
          cat test/sil/results/*.xml

      - name: Upload test results
        if: always()
        uses: actions/upload-artifact@v4
        with:
          name: sil-test-results
          path: test/sil/results/
```

### 9.2 SIL Test Categories

| Test Category | Description | Test Method |
|---------------|-------------|-------------|
| CAN message routing | Verify all CAN messages sent/received correctly | cansend + candump + validation script |
| E2E protection | Verify CRC and alive counter on safety messages | Inject corrupted CAN frame, verify rejection |
| UDS diagnostic | Verify DiagnosticSessionControl, ReadDTC, ClearDTC | Send UDS requests via CAN, check responses |
| OBD-II PID | Verify Mode 01 PID responses | Send Mode 01 requests, validate PIDs |
| BCM light logic | Verify headlight/indicator/hazard logic | Send vehicle state CAN messages, check BCM output |
| ICU display | Verify dashboard receives and displays all data | Monitor ICU CAN RX and log parsed values |
| State machine | Verify vehicle state transitions via CAN | Inject state change triggers, verify broadcast |
| DTC flow | Verify DTC storage and retrieval chain | Trigger fault, verify DTC in Dem, read via UDS |

---

## 10. Limitations

| Limitation | Impact | Mitigation |
|------------|--------|------------|
| No real sensor simulation | vECUs cannot test sensor plausibility logic | Sensor plausibility tested on physical ECUs (PIL/HIL) |
| No timing-accurate scheduling | vECU task scheduling is Linux best-effort, not RTOS | WCET analysis done on physical ECUs only |
| No MPU/memory protection | POSIX processes have flat memory model | MPU testing done on physical ECUs only |
| QM only | vECUs do not execute safety-critical code paths | Safety validation exclusively on physical ECUs |
| No hardware watchdog | TPS3823 not present in vECU | Watchdog testing done on physical ECUs |
| Linux CAN latency | SocketCAN has higher latency than FDCAN hardware | Timing-critical tests done on physical ECUs |
| No flash/NVM simulation | State persistence (TSR-037) not testable on vECU | NVM testing done on physical ECUs |

### 10.1 What vECUs ARE Good For

1. **CAN protocol testing**: Message format, E2E, routing, bus utilization
2. **BSW stack validation**: CanIf/PduR/Com chain, Dcm/Dem diagnostic stack
3. **Application logic**: BCM light/indicator logic, ICU display parsing, TCU UDS handler
4. **Regression testing**: Fast CI/CD validation on every commit
5. **Demo without hardware**: Show the system operating to reviewers/interviewers
6. **Integration rehearsal**: Practice multi-ECU integration before hardware arrives

### 10.2 What vECUs Are NOT Good For

1. Safety validation (ASIL D requirements)
2. WCET verification
3. Hardware fault injection
4. EMC/electrical testing
5. Power consumption measurement
6. Watchdog timing verification

---

## 11. Runtime Modes

| Mode | CAN Interface | Docker | Use Case |
|------|--------------|--------|----------|
| SIL (standalone) | vcan0 | docker-compose up | CI/CD testing, no hardware needed |
| SIL (bridged) | vcan0 + can0 bridge | docker-compose up + CAN bridge | Mixed testing with physical ECUs |
| Native (no Docker) | vcan0 or can0 | None (run binaries directly) | Fast development iteration |
| PC-only demo | vcan0 + CAN inject script | docker-compose up | Demo all 7 ECUs without any hardware |

### 11.1 PC-Only Demo Mode

For demonstrations without any physical hardware, a Python CAN injection script simulates the physical ECU traffic:

```python
# scripts/demo_inject.py -- Simulates physical ECU CAN messages for PC-only demo

import can
import time
import struct

bus = can.Bus(channel='vcan0', interface='socketcan')

# Simulate CVC heartbeat (0x010), FZC heartbeat (0x011), RZC heartbeat (0x012)
# Simulate vehicle state (0x100), motor status (0x300), motor current (0x301)
# Simulate steering status (0x200), brake status (0x201), lidar distance (0x220)

heartbeat_counter = 0
alive_counter = 0

while True:
    # CVC heartbeat
    bus.send(can.Message(arbitration_id=0x010, data=[
        (heartbeat_counter << 4) | 0x01,  # alive + data_id
        0x00,                              # CRC placeholder
        0x01,                              # ECU ID = CVC
        0x02                               # Mode = RUN
    ], is_extended_id=False))

    # Vehicle state (RUN, no faults)
    bus.send(can.Message(arbitration_id=0x100, data=[
        (alive_counter << 4) | 0x02,  # alive + data_id
        0x00,                          # CRC
        0x02, 0x00, 0x00, 0x64, 0x64, 0x00  # state=RUN, faults=0, torque=100%, speed=100%
    ], is_extended_id=False))

    heartbeat_counter = (heartbeat_counter + 1) & 0x0F
    alive_counter = (alive_counter + 1) & 0x0F
    time.sleep(0.050)  # 50 ms cycle
```

---

## 12. Directory Structure

```
firmware/
  bcm/
    src/        -- BCM application SWCs (Swc_Lights.c, Swc_Indicators.c, Swc_DoorLock.c)
    include/    -- BCM application headers
    cfg/        -- BCM-specific BSW configuration (Com_Cfg.h, BswM_Cfg.h)
    test/       -- BCM unit tests
  icu/
    src/        -- ICU application SWCs (Swc_Dashboard.c, Swc_DtcDisplay.c)
    include/    -- ICU application headers
    cfg/        -- ICU-specific BSW configuration
    test/       -- ICU unit tests
  tcu/
    src/        -- TCU application SWCs (UDS_Server.c, DTC_Store.c, OBD_Handler.c)
    include/    -- TCU application headers
    cfg/        -- TCU-specific BSW configuration
    test/       -- TCU unit tests
  shared/bsw/
    mcal/
      stm32/    -- STM32 MCAL drivers (Can_STM32.c, Spi_STM32.c, etc.)
      posix/    -- POSIX MCAL drivers (Can_Posix.c, Gpt_Posix.c, etc.)
    ecual/      -- CanIf, PduR, IoHwAb (platform-independent)
    services/   -- Com, Dcm, Dem, WdgM, BswM, E2E (platform-independent)
    rte/        -- Runtime Environment (platform-independent)
    include/    -- Platform_Types.h, Std_Types.h

docker/
  Dockerfile.base   -- Base image (Ubuntu 22.04 + build tools + can-utils)
  Dockerfile.bcm    -- BCM container
  Dockerfile.icu    -- ICU container
  Dockerfile.tcu    -- TCU container
  docker-compose.yml -- Orchestration for all 3 vECUs + vcan setup
```

---

## 13. Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 0.1 | 2026-02-21 | System | Initial stub (planned status) |
| 1.0 | 2026-02-21 | System | Complete vECU architecture: platform abstraction stack, POSIX MCAL implementations (Can, Gpt, Dio, Adc stubs), Docker structure, CAN bridge configuration, build system, CI/CD workflow, runtime modes, limitations |

