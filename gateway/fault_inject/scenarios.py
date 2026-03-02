"""Predefined fault scenarios for Taktflow CAN fault injection.

Each scenario sends specific CAN frames on vcan0 to trigger fault paths
through the real ECU firmware running in Docker containers.

CAN frame layout matches taktflow.dbc.  All E2E-protected messages use
CRC-8 SAE J1850 (poly 0x1D, init 0xFF) computed over [data_id] + payload[2:].
"""

import json
import os
import time
from typing import Optional

import can
import paho.mqtt.client as paho_mqtt

from .pedal_udp import (
    clear_pedal_override,
    pedal_pct_to_angle,
    send_pedal_override,
)

# Module-level MQTT client (set by app.py at startup)
_mqtt_client: paho_mqtt.Client | None = None


def set_mqtt_client(client: paho_mqtt.Client) -> None:
    """Set the module-level MQTT client (called by app.py on startup)."""
    global _mqtt_client
    _mqtt_client = client

# ---------------------------------------------------------------------------
# CAN IDs (from taktflow.dbc)
# ---------------------------------------------------------------------------
CAN_ESTOP = 0x001           # EStop_Broadcast  — 4 bytes
CAN_TORQUE_REQUEST = 0x101  # Torque_Request   — 8 bytes
CAN_STEER_COMMAND = 0x102   # Steer_Command    — 8 bytes
CAN_BRAKE_COMMAND = 0x103   # Brake_Command    — 8 bytes
CAN_BATTERY_STATUS = 0x303  # Battery_Status   — 4 bytes, no E2E
CAN_DTC_BROADCAST = 0x500   # DTC_Broadcast    — 8 bytes, no E2E

# DTC codes (must match plant_sim/simulator.py and SAP QM mock)
DTC_BATTERY_UV = 0xE401
ECU_RZC = 3

# E2E Data IDs (lower 4 bits of byte 0, matches DBC DataID field values)
# These are fixed per message type by convention in the DBC/plant sim.
DATA_ID_ESTOP = 0x01
DATA_ID_TORQUE = 0x02
DATA_ID_STEER = 0x03
DATA_ID_BRAKE = 0x04

# Shared alive counters (per CAN ID, 4-bit wrapping)
_alive_counters: dict[int, int] = {}


# ---------------------------------------------------------------------------
# E2E helpers — matches plant_sim.simulator._crc8_j1850 exactly
# ---------------------------------------------------------------------------

def _crc8_j1850(data_id: int, payload: bytes, start: int = 2) -> int:
    """CRC-8 SAE J1850 over [data_id] + payload[start:]."""
    crc = 0xFF
    for byte in [data_id] + list(payload[start:]):
        crc ^= byte
        for _ in range(8):
            if crc & 0x80:
                crc = ((crc << 1) ^ 0x1D) & 0xFF
            else:
                crc = (crc << 1) & 0xFF
    return crc


def _next_alive(msg_id: int) -> int:
    """Increment and return the 4-bit alive counter for a CAN ID."""
    val = _alive_counters.get(msg_id, 0)
    _alive_counters[msg_id] = (val + 1) & 0x0F
    return val


def _build_e2e_frame(msg_id: int, data_id: int,
                     payload: bytearray) -> bytes:
    """Insert E2E header (byte 0) and CRC (byte 1) into payload."""
    alive = _next_alive(msg_id)
    payload[0] = (alive << 4) | (data_id & 0x0F)
    crc = _crc8_j1850(data_id, bytes(payload))
    payload[1] = crc
    return bytes(payload)


def _get_bus() -> can.Bus:
    """Create a python-can Bus on the configured channel."""
    channel = os.environ.get("CAN_CHANNEL", "vcan0")
    return can.interface.Bus(channel=channel, interface="socketcan")


def _send(bus: can.Bus, arb_id: int, data: bytes) -> None:
    """Send a single CAN frame."""
    msg = can.Message(arbitration_id=arb_id, data=data,
                      is_extended_id=False)
    bus.send(msg)


# ---------------------------------------------------------------------------
# Helper frame builders
# ---------------------------------------------------------------------------

def _torque_frame(duty_pct: int, direction: int) -> bytes:
    """Build a Torque_Request frame (0x101, 8 bytes).

    Byte layout (little-endian, from DBC):
      [0] E2E: AliveCounter[7:4] | DataID[3:0]
      [1] E2E: CRC8
      [2] TorqueRequest  (0-100 %)
      [3] Direction[1:0]  (0=stop, 1=fwd, 2=rev)
      [4..7] PedalPosition1/2, PedalFault (zeroed for injection)
    """
    payload = bytearray(8)
    payload[2] = max(0, min(100, duty_pct))
    payload[3] = direction & 0x03
    return _build_e2e_frame(CAN_TORQUE_REQUEST, DATA_ID_TORQUE, payload)


def _steer_frame(angle_deg: float, rate_limit: float = 10.0,
                 vehicle_state: int = 1) -> bytes:
    """Build a Steer_Command frame (0x102, 8 bytes).

    SteerAngleCmd: 16-bit unsigned, scale 0.01, offset -45.0
      raw = (angle_deg + 45.0) / 0.01
    """
    payload = bytearray(8)
    raw = int((angle_deg + 45.0) / 0.01)
    raw = max(0, min(9000, raw))  # -45 .. +45 deg
    payload[2] = raw & 0xFF
    payload[3] = (raw >> 8) & 0xFF
    # SteerRateLimit: scale 0.2, byte 4
    payload[4] = max(0, min(255, int(rate_limit / 0.2)))
    # VehicleState: 4 bits at byte 5
    payload[5] = vehicle_state & 0x0F
    return _build_e2e_frame(CAN_STEER_COMMAND, DATA_ID_STEER, payload)


def _brake_frame(brake_pct: int, brake_mode: int = 1,
                 vehicle_state: int = 1) -> bytes:
    """Build a Brake_Command frame (0x103, 8 bytes).

    BrakeForceCmd: 8-bit (0-100 %)
    BrakeMode: 4 bits at bits 24-27  (0=release, 1=normal, 2=emergency, 3=auto)
    VehicleState: 4 bits at bits 28-31
    """
    payload = bytearray(8)
    payload[2] = max(0, min(100, brake_pct))
    payload[3] = (brake_mode & 0x0F) | ((vehicle_state & 0x0F) << 4)
    return _build_e2e_frame(CAN_BRAKE_COMMAND, DATA_ID_BRAKE, payload)


def _battery_frame(voltage_mv: int, soc_pct: int, status: int) -> bytes:
    """Build a Battery_Status frame (0x303, 4 bytes, no E2E).

    Byte layout (from DBC):
      [0..1] BatteryVoltage_mV  (16-bit LE, 0-20000)
      [2]    BatterySOC          (0-100 %)
      [3]    BatteryStatus[3:0]  (0=critical_UV, 1=UV_warn, 2=normal)
    """
    payload = bytearray(4)
    voltage_mv = max(0, min(20000, voltage_mv))
    payload[0] = voltage_mv & 0xFF
    payload[1] = (voltage_mv >> 8) & 0xFF
    payload[2] = max(0, min(100, soc_pct))
    payload[3] = status & 0x0F
    return bytes(payload)


def _dtc_frame(dtc_code: int, ecu_source: int, occurrence: int = 1) -> bytes:
    """Build a DTC_Broadcast frame (0x500, 8 bytes, no E2E).

    Byte layout (from DBC):
      [0..1] DTC_Number     (16-bit LE)
      [2]    DTC_Status      (0x01 = active)
      [3]    ECU_Source       (1=CVC, 2=FZC, 3=RZC, 4=SC)
      [4]    OccurrenceCount
      [5..7] FreezeFrame0-2
    """
    payload = bytearray(8)
    payload[0] = dtc_code & 0xFF
    payload[1] = (dtc_code >> 8) & 0xFF
    payload[2] = 0x01  # active
    payload[3] = ecu_source & 0xFF
    payload[4] = min(255, occurrence)
    return bytes(payload)


def _estop_frame(active: bool, source: int = 1) -> bytes:
    """Build an EStop_Broadcast frame (0x001, 4 bytes).

    Byte layout:
      [0] E2E: AliveCounter[7:4] | DataID[3:0]
      [1] E2E: CRC8
      [2] EStop_Active  (uint8: 0=inactive, 1=active)
      [3] EStop_Source   (uint8: 0=CVC_button, 1=CAN_request, 2=SC_relay)
    """
    payload = bytearray(4)
    payload[2] = 1 if active else 0
    payload[3] = source & 0xFF
    return _build_e2e_frame(CAN_ESTOP, DATA_ID_ESTOP, payload)


# ---------------------------------------------------------------------------
# Scenario functions
# ---------------------------------------------------------------------------

def normal_drive() -> str:
    """Set pedal to 50% via SPI override, steer 0 deg, brake 0%.

    Injects pedal sensor value at the MCAL layer so the CVC processes
    it through its full pipeline (plausibility, ramp limit, torque lookup).
    The CVC naturally generates the Torque_Request CAN frame.
    """
    send_pedal_override(pedal_pct_to_angle(50))
    bus = _get_bus()
    try:
        _send(bus, CAN_STEER_COMMAND, _steer_frame(0.0))
        _send(bus, CAN_BRAKE_COMMAND, _brake_frame(0))
    finally:
        bus.shutdown()
    return "Normal drive: pedal 50% (SPI override), steer 0 deg, brake 0%"


def overcurrent() -> str:
    """Send very high torque request (95% duty) with emergency brake locked.

    Simulates a mechanical jam: the motor drives at full power while the
    brake holds the wheels.  RPM stays near zero, so the load factor
    remains ~1.0 and current stays above 20 A (overcurrent threshold).

    The RZC sets OvercurrentFlag, the Safety Controller detects it,
    triggers a DTC broadcast, and ultimately generates an SAP QM
    notification via the gateway.
    """
    bus = _get_bus()
    try:
        _send(bus, CAN_BRAKE_COMMAND, _brake_frame(100, brake_mode=2))
        _send(bus, CAN_TORQUE_REQUEST, _torque_frame(95, 1))
    finally:
        bus.shutdown()
    return ("Overcurrent: 95% torque + 100% brake (mechanical jam).  "
            "Plant sim models sustained overcurrent -> RZC DTC -> "
            "SC detection -> SAP notification.")


def steer_fault() -> str:
    """Inject a steering fault by rapidly oscillating steer commands.

    Sends alternating +/- 40 deg commands with zero rate-limit delay,
    exceeding the FZC's configured maximum steering rate and triggering
    a SteerFaultStatus in the Steering_Status response.
    """
    bus = _get_bus()
    try:
        for _ in range(10):
            _send(bus, CAN_STEER_COMMAND, _steer_frame(40.0, rate_limit=50.0))
            time.sleep(0.005)
            _send(bus, CAN_STEER_COMMAND, _steer_frame(-40.0, rate_limit=50.0))
            time.sleep(0.005)
    finally:
        bus.shutdown()
    return ("Steer fault: 10 rapid +/-40 deg oscillations sent.  "
            "FZC should detect rate-limit violation.")


def brake_fault() -> str:
    """Send conflicting brake commands.

    Rapidly alternates between 0% (release) and 100% (emergency) brake
    commands to trigger a brake fault detection in the FZC.
    """
    bus = _get_bus()
    try:
        for _ in range(10):
            _send(bus, CAN_BRAKE_COMMAND,
                  _brake_frame(100, brake_mode=2))  # emergency
            time.sleep(0.005)
            _send(bus, CAN_BRAKE_COMMAND,
                  _brake_frame(0, brake_mode=0))    # release
            time.sleep(0.005)
    finally:
        bus.shutdown()
    return ("Brake fault: 10 rapid 0%/100% alternations sent.  "
            "FZC should detect conflicting brake commands.")


def battery_low() -> str:
    """Simulate battery undervoltage by injecting low-voltage CAN frames.

    Sends Battery_Status frames (0x303) at 100 ms intervals with
    progressively lower voltage and SOC, overriding the plant sim's
    1000 ms Battery_Status cycle.  After the drain sequence, sends a
    DTC_BATTERY_UV broadcast to trigger the SAP QM notification path.

    Phase 1 (2 s): 12.6 V -> 10.2 V (UV_warn zone)
    Phase 2 (3 s): 10.2 V ->  8.5 V (critical_UV zone) + DTC
    """
    bus = _get_bus()
    try:
        # Phase 1: voltage drops from 12.6 V to 10.2 V (UV_warn) over 2 s
        for i in range(20):
            frac = i / 19.0
            v = int(12600 - (12600 - 10200) * frac)
            soc = int(100 - (100 - 18) * frac)
            status = 1 if v < 10500 else 2  # UV_warn below 10.5 V
            _send(bus, CAN_BATTERY_STATUS, _battery_frame(v, soc, status))
            time.sleep(0.1)

        # Phase 2: voltage drops from 10.2 V to 8.5 V (critical_UV) over 3 s
        for i in range(30):
            frac = i / 29.0
            v = int(10200 - (10200 - 8500) * frac)
            soc = int(18 - (18 - 3) * frac)
            status = 0 if v < 9000 else 1  # critical_UV below 9.0 V
            _send(bus, CAN_BATTERY_STATUS, _battery_frame(v, soc, status))
            time.sleep(0.1)

        # Fire DTC_BATTERY_UV
        _send(bus, CAN_DTC_BROADCAST,
              _dtc_frame(DTC_BATTERY_UV, ECU_RZC))
    finally:
        bus.shutdown()
    return ("Battery drain: 12.6 V -> 8.5 V over 5 s + DTC 0xE401.  "
            "Plant sim restores normal values within ~1 s after scenario ends.")


def motor_reversal() -> str:
    """Inject sudden motor direction reversal while driving forward.

    Sends a Torque_Request with direction=2 (reverse) at 80% duty while
    the vehicle is moving forward.  The Safety Controller should detect
    the plausibility violation (direction change at speed) and trigger
    a safe-stop transition.

    Maps to HE-014 (ASIL C): Motor direction reversal during forward motion.
    """
    bus = _get_bus()
    try:
        # First establish forward drive
        _send(bus, CAN_TORQUE_REQUEST, _torque_frame(50, 1))
        time.sleep(0.5)
        # Reverse at high torque
        _send(bus, CAN_TORQUE_REQUEST, _torque_frame(80, 2))
    finally:
        bus.shutdown()
    return ("Motor reversal: direction flipped fwd->rev at 80% torque.  "
            "SC should detect plausibility violation -> safe-stop.")


def unintended_braking() -> str:
    """Inject unexpected emergency braking while cruising.

    Sends a Brake_Command with 100% force in emergency mode while the
    vehicle is driving at normal speed.  Simulates a brake actuator
    fault or spurious brake command.

    Maps to HE-006 (ASIL A): Unintended braking during normal driving.
    """
    bus = _get_bus()
    try:
        # Establish normal drive first
        _send(bus, CAN_TORQUE_REQUEST, _torque_frame(50, 1))
        _send(bus, CAN_BRAKE_COMMAND, _brake_frame(0, brake_mode=0))
        time.sleep(0.5)
        # Sudden emergency brake
        _send(bus, CAN_BRAKE_COMMAND, _brake_frame(100, brake_mode=2))
    finally:
        bus.shutdown()
    return ("Unintended braking: emergency brake at 100% while cruising.  "
            "Simulates spurious brake actuator activation.")


def torque_loss() -> str:
    """Inject sudden loss of drive torque while driving.

    Sends torque request dropping to 0% with direction=stop while the
    vehicle is in motion.  Simulates motor controller failure or
    power stage dropout.

    Maps to HE-002 (ASIL B): Loss of drive torque during driving.
    """
    bus = _get_bus()
    try:
        # Establish normal drive
        _send(bus, CAN_TORQUE_REQUEST, _torque_frame(50, 1))
        time.sleep(0.5)
        # Sudden torque loss
        _send(bus, CAN_TORQUE_REQUEST, _torque_frame(0, 0))
    finally:
        bus.shutdown()
    return ("Torque loss: torque dropped to 0% mid-drive.  "
            "Simulates motor controller failure / power stage dropout.")


def runaway_accel() -> str:
    """Inject runaway acceleration at maximum torque.

    Sends a Torque_Request at 100% duty forward, simulating a stuck
    throttle or pedal sensor fault at high speed.  The Safety Controller
    should detect the plausibility violation (no pedal input matches)
    and intervene.

    Maps to HE-016 (ASIL C): Unintended acceleration at high speed.
    """
    bus = _get_bus()
    try:
        _send(bus, CAN_TORQUE_REQUEST, _torque_frame(100, 1))
    finally:
        bus.shutdown()
    return ("Runaway acceleration: 100% torque forward.  "
            "SC should detect pedal plausibility violation -> limit torque.")


def creep_from_stop() -> str:
    """Inject unintended vehicle motion from stationary state.

    Releases brakes and applies moderate torque while vehicle should be
    stationary (e.g. parked, waiting at traffic light).  Simulates
    unintended creep due to motor enable fault.

    Maps to HE-017 (ASIL D): Unintended vehicle motion from stationary.
    """
    bus = _get_bus()
    try:
        # Release brakes
        _send(bus, CAN_BRAKE_COMMAND, _brake_frame(0, brake_mode=0))
        time.sleep(0.1)
        # Apply unexpected torque
        _send(bus, CAN_TORQUE_REQUEST, _torque_frame(30, 1))
    finally:
        bus.shutdown()
    return ("Creep from stop: brakes released + 30% torque from stationary.  "
            "Simulates unintended motion due to motor enable fault.")


def babbling_node() -> str:
    """Flood the CAN bus with rapid frames simulating a babbling node.

    Sends 200 frames at maximum rate on the Torque_Request CAN ID,
    saturating bus bandwidth.  A babbling node can prevent legitimate
    messages from being transmitted, starving safety-critical
    communication.

    Maps to HE-020 (ASIL B): CAN bus babbling node.
    """
    bus = _get_bus()
    try:
        for _ in range(200):
            _send(bus, CAN_TORQUE_REQUEST, _torque_frame(0, 0))
    finally:
        bus.shutdown()
    return ("Babbling node: 200 rapid frames flooded on 0x101.  "
            "Simulates CAN bus saturation from faulty ECU.")


def heartbeat_loss() -> str:
    """Document heartbeat loss injection.

    ECU heartbeats (CVC 0x010, FZC 0x011, RZC 0x012) are sent by the
    firmware containers themselves at 50 ms intervals.  To simulate a
    heartbeat loss, stop the target container:

        docker stop cvc   # CVC heartbeat loss
        docker stop fzc   # FZC heartbeat loss
        docker stop rzc   # RZC heartbeat loss

    The Safety Controller (SC) monitors all heartbeats and will trigger
    a safe-stop transition after the configured timeout (typically
    150-200 ms = 3 missed heartbeats).

    This scenario is not directly injectable via CAN frames.
    """
    return ("Heartbeat loss: not directly injectable via CAN.  "
            "Stop an ECU container (e.g. `docker stop cvc`) to trigger "
            "SC heartbeat timeout -> safe-stop transition.")


def can_loss() -> str:
    """Document CAN bus loss injection.

    CAN loss for a specific ECU is triggered by stopping its container:

        docker stop cvc   # CVC goes offline

    This causes all CAN messages from that ECU to cease, which the
    remaining ECUs and the Safety Controller detect via E2E timeout
    monitoring and heartbeat loss.

    For a full bus failure, bring down the virtual CAN interface:

        sudo ip link set vcan0 down

    This scenario is infrastructure-level, not injectable via CAN frames.
    """
    return ("CAN loss: triggered by `docker stop <ecu>` or "
            "`sudo ip link set vcan0 down`.  "
            "Not injectable via CAN frames.")


def estop() -> str:
    """Send E-Stop frame (0x001) with EStop_Active=1.

    The CVC broadcasts EStop to all ECUs.  All actuators are disabled,
    motor is de-energized, brakes are applied (emergency mode), and the
    vehicle enters SAFE_STOP state.
    """
    bus = _get_bus()
    try:
        # Send E-Stop active with source=1 (CAN_request)
        _send(bus, CAN_ESTOP, _estop_frame(active=True, source=1))
    finally:
        bus.shutdown()
    return "E-Stop activated: EStop_Active=1, source=CAN_request"


def reset() -> str:
    """Clear E-Stop, clear pedal override, reset all actuators to idle.

    Clears the SPI pedal override (reverts to dead-zone = torque 0),
    sends E-Stop clear (EStop_Active=0), then sets steer to 0 deg
    and brake to 0%.  Also publishes MQTT reset command so the ML
    detector and ws_bridge clear their state.
    """
    clear_pedal_override()
    bus = _get_bus()
    try:
        _send(bus, CAN_ESTOP, _estop_frame(active=False, source=1))
        time.sleep(0.01)
        _send(bus, CAN_TORQUE_REQUEST, _torque_frame(0, 0))
        _send(bus, CAN_STEER_COMMAND, _steer_frame(0.0))
        _send(bus, CAN_BRAKE_COMMAND, _brake_frame(0, brake_mode=0))
    finally:
        bus.shutdown()

    # Publish MQTT reset command for ML detector and ws_bridge
    if _mqtt_client is not None:
        reset_payload = json.dumps({"action": "reset", "ts": time.time()})
        _mqtt_client.publish("taktflow/command/reset", reset_payload, qos=1)

    return "Reset: E-Stop cleared, torque=0, steer=0, brake=0"


# ---------------------------------------------------------------------------
# Registry — maps scenario name to (function, description)
# ---------------------------------------------------------------------------

SCENARIOS: dict[str, dict] = {
    "normal_drive": {
        "fn": normal_drive,
        "description": (
            "Normal drive: pedal 50% (SPI override), steer 0 deg, brake 0%.  "
            "CVC processes pedal through full pipeline -> Torque_Request CAN."
        ),
    },
    "overcurrent": {
        "fn": overcurrent,
        "description": (
            "Overcurrent: 95% torque + 100% brake (mechanical jam) "
            "-> sustained overcurrent -> RZC DTC -> SC detection -> SAP notification."
        ),
    },
    "steer_fault": {
        "fn": steer_fault,
        "description": (
            "Steering fault: rapid +/-40 deg oscillations exceed FZC "
            "rate limit, triggering SteerFaultStatus."
        ),
    },
    "brake_fault": {
        "fn": brake_fault,
        "description": (
            "Brake fault: rapid 0%/100% alternations trigger FZC "
            "conflicting-command detection."
        ),
    },
    "battery_low": {
        "fn": battery_low,
        "description": (
            "Battery drain: injects Battery_Status frames with voltage "
            "dropping from 12.6 V to 8.5 V over 5 s, then fires DTC 0xE401."
        ),
    },
    "motor_reversal": {
        "fn": motor_reversal,
        "description": (
            "Motor reversal: direction flipped fwd->rev at 80% torque.  "
            "SC detects plausibility violation -> safe-stop."
        ),
    },
    "unintended_braking": {
        "fn": unintended_braking,
        "description": (
            "Unintended braking: emergency brake at 100% while cruising.  "
            "Simulates spurious brake actuator activation."
        ),
    },
    "torque_loss": {
        "fn": torque_loss,
        "description": (
            "Torque loss: torque dropped to 0% mid-drive.  "
            "Simulates motor controller failure / power stage dropout."
        ),
    },
    "runaway_accel": {
        "fn": runaway_accel,
        "description": (
            "Runaway acceleration: 100% torque forward.  "
            "SC detects pedal plausibility violation -> limit torque."
        ),
    },
    "creep_from_stop": {
        "fn": creep_from_stop,
        "description": (
            "Creep from stop: brakes released + 30% torque from stationary.  "
            "Simulates unintended motion due to motor enable fault."
        ),
    },
    "babbling_node": {
        "fn": babbling_node,
        "description": (
            "Babbling node: 200 rapid frames flooded on 0x101.  "
            "Simulates CAN bus saturation from faulty ECU."
        ),
    },
    "heartbeat_loss": {
        "fn": heartbeat_loss,
        "description": (
            "Heartbeat loss: stop an ECU container to trigger SC "
            "heartbeat timeout.  Not injectable via CAN."
        ),
    },
    "can_loss": {
        "fn": can_loss,
        "description": (
            "CAN bus loss: stop a container or bring down vcan0.  "
            "Not injectable via CAN."
        ),
    },
    "estop": {
        "fn": estop,
        "description": (
            "Emergency stop: sends EStop_Active=1 on 0x001.  "
            "All actuators disabled, vehicle enters SAFE_STOP."
        ),
    },
    "reset": {
        "fn": reset,
        "description": (
            "Reset: clears pedal override + E-Stop, "
            "sets steer/brake to zero."
        ),
    },
}
