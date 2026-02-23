"""WebSocket Telemetry Bridge — aggregates MQTT signals into JSON snapshots.

Subscribes to MQTT taktflow/# topics, maintains a single state snapshot,
and broadcasts it to all connected WebSocket clients at 10Hz.
"""

import asyncio
import json
import logging
import os
import time
from contextlib import asynccontextmanager

import paho.mqtt.client as mqtt
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
import uvicorn

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [WS-BRIDGE] %(levelname)s %(message)s",
    datefmt="%H:%M:%S",
)
log = logging.getLogger("ws_bridge")

# Vehicle state names
VEHICLE_STATES = {0: "INIT", 1: "RUN", 2: "DEGRADED", 3: "LIMP", 4: "SAFE_STOP", 5: "SHUTDOWN"}


class TelemetryState:
    """Aggregated telemetry snapshot, updated by MQTT messages."""

    def __init__(self):
        self.motor_rpm = 0
        self.motor_current_ma = 0
        self.motor_temp_c = 0
        self.motor_temp2_c = 0
        self.motor_duty_pct = 0
        self.motor_direction = 0
        self.motor_faults = 0
        self.motor_derating = 100
        self.motor_enable = 0
        self.motor_overcurrent = 0

        self.steer_actual_deg = 0.0
        self.steer_commanded_deg = 0.0
        self.steer_fault = 0
        self.steer_servo_ma = 0

        self.brake_position_pct = 0
        self.brake_commanded_pct = 0
        self.brake_fault = 0
        self.brake_servo_ma = 0

        self.battery_voltage_mv = 12600
        self.battery_soc_pct = 100
        self.battery_status = 2  # normal

        self.lidar_distance_cm = 500
        self.lidar_zone = 3  # clear
        self.lidar_signal_strength = 8000
        self.lidar_sensor_status = 0

        self.vehicle_state = 0
        self.vehicle_fault_mask = 0
        self.torque_limit = 100
        self.speed_limit = 100

        self.hb_cvc = False
        self.hb_fzc = False
        self.hb_rzc = False

        self.anomaly_score = 0.0
        self.anomaly_alert = False

        self.can_msgs_sec = 0
        self.start_time = time.time()

        self.events: list[dict] = []

        # Heartbeat tracking
        self._hb_cvc_ts = 0.0
        self._hb_fzc_ts = 0.0
        self._hb_rzc_ts = 0.0

    def to_snapshot(self) -> dict:
        """Build the JSON snapshot for WebSocket broadcast."""
        now = time.time()
        # Heartbeats are alive if seen within last 200ms
        hb_timeout = 0.2
        return {
            "ts": now,
            "motor": {
                "rpm": self.motor_rpm,
                "current_ma": self.motor_current_ma,
                "temp_c": self.motor_temp_c,
                "temp2_c": self.motor_temp2_c,
                "duty_pct": self.motor_duty_pct,
                "direction": self.motor_direction,
                "faults": self.motor_faults,
                "derating": self.motor_derating,
                "enable": self.motor_enable,
                "overcurrent": self.motor_overcurrent,
            },
            "steering": {
                "actual_deg": round(self.steer_actual_deg, 2),
                "commanded_deg": round(self.steer_commanded_deg, 2),
                "fault": self.steer_fault,
                "servo_ma": self.steer_servo_ma,
            },
            "brake": {
                "position_pct": self.brake_position_pct,
                "commanded_pct": self.brake_commanded_pct,
                "fault": self.brake_fault,
                "servo_ma": self.brake_servo_ma,
            },
            "battery": {
                "voltage_mv": self.battery_voltage_mv,
                "soc_pct": self.battery_soc_pct,
                "status": self.battery_status,
            },
            "lidar": {
                "distance_cm": self.lidar_distance_cm,
                "zone": self.lidar_zone,
                "signal_strength": self.lidar_signal_strength,
                "sensor_status": self.lidar_sensor_status,
            },
            "vehicle": {
                "state": self.vehicle_state,
                "state_name": VEHICLE_STATES.get(self.vehicle_state, "UNKNOWN"),
                "fault_mask": self.vehicle_fault_mask,
                "torque_limit": self.torque_limit,
                "speed_limit": self.speed_limit,
            },
            "heartbeats": {
                "cvc": (now - self._hb_cvc_ts) < hb_timeout,
                "fzc": (now - self._hb_fzc_ts) < hb_timeout,
                "rzc": (now - self._hb_rzc_ts) < hb_timeout,
            },
            "anomaly": {
                "score": round(self.anomaly_score, 3),
                "alert": self.anomaly_alert,
            },
            "stats": {
                "can_msgs_sec": self.can_msgs_sec,
                "uptime_sec": int(now - self.start_time),
            },
            "events": self.events[-20:],  # Last 20 events
        }

    def add_event(self, event_type: str, message: str):
        """Add an event to the log."""
        self.events.append({
            "ts": time.time(),
            "type": event_type,
            "msg": message,
        })
        # Keep max 100 events in memory
        if len(self.events) > 100:
            self.events = self.events[-100:]


# Global state
state = TelemetryState()
ws_clients: set[WebSocket] = set()


def _parse_float(val: str, default: float = 0.0) -> float:
    try:
        return float(val)
    except (ValueError, TypeError):
        return default


def _parse_int(val: str, default: int = 0) -> int:
    try:
        return int(float(val))
    except (ValueError, TypeError):
        return default


def on_mqtt_message(client, userdata, msg):
    """Handle incoming MQTT messages — update telemetry state."""
    topic = msg.topic
    payload = msg.payload.decode("utf-8", errors="replace")

    # CAN signal topics: taktflow/can/{MsgName}/{SignalName}
    if topic.startswith("taktflow/can/"):
        parts = topic.split("/")
        if len(parts) >= 4:
            msg_name = parts[2]
            sig_name = parts[3]
            _update_signal(msg_name, sig_name, payload)

    # Stats
    elif topic == "taktflow/telemetry/stats/can_msgs_per_sec":
        state.can_msgs_sec = _parse_int(payload)

    # Anomaly
    elif topic.startswith("taktflow/anomaly/"):
        if topic.endswith("/score"):
            state.anomaly_score = _parse_float(payload)
            state.anomaly_alert = state.anomaly_score > 0.7

    # DTC alerts
    elif topic.startswith("taktflow/alerts/dtc/"):
        try:
            alert = json.loads(payload)
            dtc = alert.get("dtc", "?")
            ecu = alert.get("ecu_source", 0)
            state.add_event("dtc", f"DTC {dtc} from ECU {ecu}")
        except json.JSONDecodeError:
            pass

    # SAP QM events
    elif topic.startswith("taktflow/sap/"):
        try:
            evt = json.loads(payload)
            qn_id = evt.get("notification_id", "?")
            state.add_event("sap", f"SAP QM notification created: {qn_id}")
        except json.JSONDecodeError:
            pass


def _update_signal(msg_name: str, sig_name: str, payload: str):
    """Update the telemetry state from a decoded CAN signal."""
    # Motor Status (0x300)
    if msg_name == "Motor_Status":
        if sig_name == "MotorSpeed_RPM":
            state.motor_rpm = _parse_int(payload)
        elif sig_name == "MotorDirection":
            state.motor_direction = _parse_int(payload)
        elif sig_name == "MotorFaultStatus":
            state.motor_faults = _parse_int(payload)
        elif sig_name == "DutyPercent":
            state.motor_duty_pct = _parse_int(payload)
        elif sig_name == "DeratingPercent":
            state.motor_derating = _parse_int(payload)
        elif sig_name == "MotorEnable":
            state.motor_enable = _parse_int(payload)

    # Motor Current (0x301)
    elif msg_name == "Motor_Current":
        if sig_name == "Current_mA":
            state.motor_current_ma = _parse_int(payload)
        elif sig_name == "OvercurrentFlag":
            state.motor_overcurrent = _parse_int(payload)

    # Motor Temperature (0x302)
    elif msg_name == "Motor_Temperature":
        if sig_name == "WindingTemp1_C":
            state.motor_temp_c = _parse_int(payload)
        elif sig_name == "WindingTemp2_C":
            state.motor_temp2_c = _parse_int(payload)

    # Steering Status (0x200)
    elif msg_name == "Steering_Status":
        if sig_name == "ActualAngle":
            state.steer_actual_deg = _parse_float(payload)
        elif sig_name == "CommandedAngle":
            state.steer_commanded_deg = _parse_float(payload)
        elif sig_name == "SteerFaultStatus":
            state.steer_fault = _parse_int(payload)
        elif sig_name == "ServoCurrent_mA":
            state.steer_servo_ma = _parse_int(payload)

    # Brake Status (0x201)
    elif msg_name == "Brake_Status":
        if sig_name == "BrakePosition":
            state.brake_position_pct = _parse_int(payload)
        elif sig_name == "BrakeCommandEcho":
            state.brake_commanded_pct = _parse_int(payload)
        elif sig_name == "BrakeFaultStatus":
            state.brake_fault = _parse_int(payload)
        elif sig_name == "ServoCurrent_mA":
            state.brake_servo_ma = _parse_int(payload)

    # Battery Status (0x303)
    elif msg_name == "Battery_Status":
        if sig_name == "BatteryVoltage_mV":
            state.battery_voltage_mv = _parse_int(payload)
        elif sig_name == "BatterySOC":
            state.battery_soc_pct = _parse_int(payload)
        elif sig_name == "BatteryStatus":
            state.battery_status = _parse_int(payload)

    # Lidar Distance (0x220)
    elif msg_name == "Lidar_Distance":
        if sig_name == "Distance_cm":
            state.lidar_distance_cm = _parse_int(payload)
        elif sig_name == "ObstacleZone":
            state.lidar_zone = _parse_int(payload)
        elif sig_name == "SignalStrength":
            state.lidar_signal_strength = _parse_int(payload)
        elif sig_name == "SensorStatus":
            state.lidar_sensor_status = _parse_int(payload)

    # Vehicle State (0x100)
    elif msg_name == "Vehicle_State":
        if sig_name == "VehicleState":
            old = state.vehicle_state
            new = _parse_int(payload)
            if old != new:
                old_name = VEHICLE_STATES.get(old, "?")
                new_name = VEHICLE_STATES.get(new, "?")
                state.add_event("state", f"Vehicle state: {old_name} -> {new_name}")
            state.vehicle_state = new
        elif sig_name == "FaultMask":
            state.vehicle_fault_mask = _parse_int(payload)
        elif sig_name == "TorqueLimit":
            state.torque_limit = _parse_int(payload)
        elif sig_name == "SpeedLimit":
            state.speed_limit = _parse_int(payload)

    # Heartbeats
    elif msg_name == "CVC_Heartbeat":
        state._hb_cvc_ts = time.time()
    elif msg_name == "FZC_Heartbeat":
        state._hb_fzc_ts = time.time()
    elif msg_name == "RZC_Heartbeat":
        state._hb_rzc_ts = time.time()


def setup_mqtt() -> mqtt.Client:
    """Create and connect the MQTT client."""
    mqtt_host = os.environ.get("MQTT_HOST", "localhost")
    mqtt_port = int(os.environ.get("MQTT_PORT", "1883"))

    client = mqtt.Client(
        mqtt.CallbackAPIVersion.VERSION2,
        client_id="ws-bridge",
    )
    client.on_message = on_mqtt_message

    def on_connect(c, userdata, flags, rc, properties=None):
        if rc == 0:
            log.info("Connected to MQTT broker at %s:%d", mqtt_host, mqtt_port)
            c.subscribe("taktflow/#", qos=0)
        else:
            log.error("MQTT connect failed: rc=%d", rc)

    client.on_connect = on_connect
    client.connect_async(mqtt_host, mqtt_port, keepalive=30)
    client.loop_start()
    return client


@asynccontextmanager
async def lifespan(app: FastAPI):
    """Start MQTT client on app startup, stop on shutdown."""
    mqtt_client = setup_mqtt()
    broadcast_task = asyncio.create_task(broadcast_loop())
    log.info("WebSocket bridge started")
    yield
    broadcast_task.cancel()
    mqtt_client.loop_stop()
    mqtt_client.disconnect()
    log.info("WebSocket bridge stopped")


app = FastAPI(title="Taktflow Telemetry Bridge", lifespan=lifespan)


@app.websocket("/ws/telemetry")
async def telemetry_ws(websocket: WebSocket):
    """WebSocket endpoint for live telemetry."""
    await websocket.accept()
    ws_clients.add(websocket)
    log.info("WebSocket client connected (%d total)", len(ws_clients))

    try:
        # Keep connection alive — client doesn't send data
        while True:
            await websocket.receive_text()
    except WebSocketDisconnect:
        pass
    finally:
        ws_clients.discard(websocket)
        log.info("WebSocket client disconnected (%d remaining)", len(ws_clients))


@app.get("/health")
async def health():
    """Health check endpoint."""
    return {
        "status": "ok",
        "clients": len(ws_clients),
        "uptime_sec": int(time.time() - state.start_time),
    }


async def broadcast_loop():
    """Broadcast telemetry snapshot to all WebSocket clients at 10Hz."""
    while True:
        if ws_clients:
            snapshot = state.to_snapshot()
            data = json.dumps(snapshot)
            disconnected = set()
            for ws in ws_clients:
                try:
                    await ws.send_text(data)
                except Exception:
                    disconnected.add(ws)
            ws_clients.difference_update(disconnected)

        await asyncio.sleep(0.1)  # 10Hz


def main():
    port = int(os.environ.get("WS_PORT", "8080"))
    uvicorn.run(app, host="0.0.0.0", port=port, log_level="info")


if __name__ == "__main__":
    main()
