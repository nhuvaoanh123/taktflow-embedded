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


def _estop_frame(active: bool, source: int = 1) -> bytes:
    """Build an EStop_Broadcast frame (0x001, 4 bytes).

    Byte layout:
      [0] E2E: AliveCounter[7:4] | DataID[3:0]
      [1] E2E: CRC8
      [2] EStop_Active[bit0] | EStop_Source[bits 3:1]
      [3] reserved (0)
    """
    payload = bytearray(4)
    estop_byte = (1 if active else 0) | ((source & 0x07) << 1)
    payload[2] = estop_byte
    return _build_e2e_frame(CAN_ESTOP, DATA_ID_ESTOP, payload)


# ---------------------------------------------------------------------------
# Scenario functions
# ---------------------------------------------------------------------------

def normal_drive() -> str:
    """Send torque request: 50% duty forward, steer 0 deg, brake 0%.

    Drives the vehicle in normal operating mode.
    """
    bus = _get_bus()
    try:
        _send(bus, CAN_TORQUE_REQUEST, _torque_frame(50, 1))
        _send(bus, CAN_STEER_COMMAND, _steer_frame(0.0))
        _send(bus, CAN_BRAKE_COMMAND, _brake_frame(0))
    finally:
        bus.shutdown()
    return "Normal drive: 50% torque forward, steer 0 deg, brake 0%"


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
    """Drain battery by running motor at sustained high load.

    Battery voltage is modeled by the plant simulator based on current
    draw over time.  This scenario cannot be directly injected via a
    single CAN frame -- it requires sustained high-torque operation.
    We send 90% torque for 2 seconds (200 frames at 10 ms).
    """
    bus = _get_bus()
    try:
        for _ in range(200):
            _send(bus, CAN_TORQUE_REQUEST, _torque_frame(90, 1))
            time.sleep(0.01)
    finally:
        bus.shutdown()
    return ("Battery drain: 90% torque sustained for 2 s.  "
            "Plant sim models voltage drop; RZC publishes Battery_Status "
            "with BatteryStatus=critical_UV when voltage drops below threshold.")


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
    """Clear E-Stop and reset all actuators to safe idle state.

    Sends E-Stop clear (EStop_Active=0), then sets torque to 0,
    steer to 0 deg, and brake to 0%.  Also publishes MQTT reset
    command so the ML detector and ws_bridge clear their state.
    """
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
            "Normal drive: 50% torque forward, steer 0 deg, brake 0%."
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
            "Battery drain: sustained 90% torque for 2 s.  Plant sim "
            "models voltage drop to critical_UV threshold."
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
            "Reset: clears E-Stop, sets torque/steer/brake to zero."
        ),
    },
}
