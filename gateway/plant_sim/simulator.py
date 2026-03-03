"""Main plant simulator — reads actuator commands from CAN, runs physics,
writes sensor data back to CAN.

Runs at 100 Hz (10ms cycle). Uses python-can + cantools for CAN I/O.
"""

import asyncio
import json
import logging
import os
import struct
import sys
import time

import can
import cantools
import paho.mqtt.client as paho_mqtt

from .motor_model import MotorModel
from .steering_model import SteeringModel
from .brake_model import BrakeModel
from .battery_model import BatteryModel
from .lidar_model import LidarModel

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [PLANT] %(message)s",
    datefmt="%H:%M:%S",
)
log = logging.getLogger("plant_sim")

# CAN IDs we read (actuator commands from ECUs)
RX_TORQUE_REQUEST = 0x101
RX_STEER_COMMAND = 0x102
RX_BRAKE_COMMAND = 0x103
RX_ESTOP = 0x001
RX_SC_RELAY_STATUS = 0x013

# CAN IDs we write (sensor feedback to ECUs)
TX_VEHICLE_STATE = 0x100
TX_STEERING_STATUS = 0x200
TX_BRAKE_STATUS = 0x201
TX_LIDAR_DISTANCE = 0x220
TX_MOTOR_STATUS = 0x300
TX_MOTOR_CURRENT = 0x301
TX_MOTOR_TEMP = 0x302
TX_BATTERY_STATUS = 0x303
TX_DTC_BROADCAST = 0x500

# DTC codes (from defect_catalog.py — must match SAP QM mock)
DTC_OVERCURRENT = 0xE301
DTC_STEER_FAULT = 0xE201
DTC_BRAKE_FAULT = 0xE202
DTC_BATTERY_UV = 0xE401

# ECU source IDs
ECU_FZC = 2
ECU_RZC = 3

# Vehicle state codes
VS_INIT = 0
VS_RUN = 1
VS_DEGRADED = 2
VS_LIMP = 3
VS_SAFE_STOP = 4
VS_SHUTDOWN = 5


class PlantSimulator:
    def __init__(self, dbc_path: str, channel: str = "vcan0"):
        self.db = cantools.database.load_file(dbc_path)
        self.channel = channel
        self.bus = None

        # Physics models
        self.motor = MotorModel()
        self.steering = SteeringModel()
        self.brake = BrakeModel()
        self.battery = BatteryModel()
        self.lidar = LidarModel()

        # Alive counters for E2E (4-bit, wraps at 15)
        self._alive = {}

        # E-stop state
        self.estop_active = False

        # SC relay kill state (from CAN 0x013)
        self.sc_relay_killed = False

        # Vehicle state — starts INIT, transitions to RUN after startup delay
        self.vehicle_state = VS_INIT
        self._startup_ticks = 0  # counts ticks since start (300 = 3s)

        # DTC tracking — only fire each DTC once until cleared
        self._active_dtcs: set[int] = set()
        self._dtc_occurrence: dict[int, int] = {}

        # Timing counters (10ms base tick)
        self._tick = 0

        # MQTT client for fault injection commands
        self._mqtt: paho_mqtt.Client | None = None

    # ------------------------------------------------------------------
    # MQTT fault injection interface
    # ------------------------------------------------------------------

    def _init_mqtt(self):
        """Connect to MQTT broker and subscribe to plant injection commands."""
        host = os.environ.get("MQTT_HOST", "localhost")
        port = int(os.environ.get("MQTT_PORT", "1883"))
        self._mqtt = paho_mqtt.Client(
            paho_mqtt.CallbackAPIVersion.VERSION2,
            client_id="taktflow-plant-sim",
        )
        self._mqtt.on_connect = self._on_mqtt_connect
        self._mqtt.message_callback_add(
            "taktflow/command/plant_inject", self._on_inject_cmd
        )
        self._mqtt.connect_async(host, port, keepalive=30)
        self._mqtt.loop_start()
        log.info("MQTT connecting to %s:%d", host, port)

    def _on_mqtt_connect(self, client, userdata, flags, rc, properties=None):
        """Subscribe to injection topic on (re)connect."""
        client.subscribe("taktflow/command/plant_inject", qos=1)
        log.info("MQTT connected — subscribed to taktflow/command/plant_inject")

    def _on_inject_cmd(self, client, userdata, msg):
        """Handle MQTT fault injection commands from fault_inject service."""
        try:
            cmd = json.loads(msg.payload)
        except (json.JSONDecodeError, TypeError):
            log.warning("Invalid inject command payload: %s", msg.payload)
            return

        cmd_type = cmd.get("type", "")
        log.info("MQTT inject command: %s", cmd_type)

        if cmd_type == "overcurrent":
            self.motor.inject_overcurrent()
            log.info("Injected overcurrent fault (current=%.0fmA)", self.motor.current_ma)
        elif cmd_type == "stall":
            self.motor.inject_stall()
            log.info("Injected motor stall fault")
        elif cmd_type == "voltage":
            mv = int(cmd.get("mV", 8500))
            soc = int(cmd.get("soc", 10))
            self.battery.inject_voltage(mv, soc)
            log.info("Injected battery voltage: %dmV, %d%% SOC", mv, soc)
        elif cmd_type == "steer_fault":
            self.steering.fault = True
            log.info("Injected steering fault")
        elif cmd_type == "brake_fault":
            self.brake.fault = True
            log.info("Injected brake fault")
        elif cmd_type == "reset":
            self.motor.reset_faults()
            self.steering.clear_fault()
            self.brake.clear_fault()
            self.battery.clear_override()
            self._active_dtcs.clear()
            log.info("Plant faults cleared via MQTT reset")

    def _next_alive(self, msg_id: int) -> int:
        val = self._alive.get(msg_id, 0)
        self._alive[msg_id] = (val + 1) & 0x0F
        return val

    def _crc8_j1850(self, data_id: int, payload: bytes, start: int = 2) -> int:
        """CRC-8 SAE J1850 over data_id + payload[start:]."""
        crc = 0xFF
        for byte in [data_id] + list(payload[start:]):
            crc ^= byte
            for _ in range(8):
                if crc & 0x80:
                    crc = ((crc << 1) ^ 0x1D) & 0xFF
                else:
                    crc = (crc << 1) & 0xFF
        return crc

    def _build_e2e_header(self, msg_id: int, data_id: int) -> tuple:
        """Return (byte0, alive) for E2E header."""
        alive = self._next_alive(msg_id)
        byte0 = (alive << 4) | (data_id & 0x0F)
        return byte0, alive

    def _encode_with_e2e(self, msg_id: int, data_id: int,
                         payload_bytes: bytearray) -> bytes:
        """Insert E2E header (byte 0) and CRC (byte 1) into payload."""
        byte0, _ = self._build_e2e_header(msg_id, data_id)
        payload_bytes[0] = byte0
        crc = self._crc8_j1850(data_id, bytes(payload_bytes))
        payload_bytes[1] = crc
        return bytes(payload_bytes)

    def _process_rx(self, msg: can.Message):
        """Process a received CAN frame from ECU actuator commands."""
        arb_id = msg.arbitration_id
        data = msg.data

        if arb_id == RX_ESTOP:
            if len(data) >= 3:
                was_active = self.estop_active
                self.estop_active = bool(data[2] & 0x01)
                if self.estop_active and not was_active:
                    log.info("E-STOP received — all outputs disabled")
                elif not self.estop_active and was_active:
                    log.info("E-STOP cleared — resetting faults, state -> INIT")
                    self.motor.reset_faults()
                    self.steering.clear_fault()
                    self.brake.clear_fault()
                    self.battery.clear_override()
                    self._active_dtcs.clear()
                    self.sc_relay_killed = False
                    self.vehicle_state = VS_INIT
                    self._startup_ticks = 0
                elif not self.estop_active and not was_active:
                    # Reset command (E-Stop clear when not active) — clear all faults
                    log.info("Reset received — clearing all faults, state -> INIT")
                    self.motor.reset_faults()
                    self.steering.clear_fault()
                    self.brake.clear_fault()
                    self.battery.clear_override()
                    self._active_dtcs.clear()
                    self.sc_relay_killed = False
                    self.vehicle_state = VS_INIT
                    self._startup_ticks = 0

        elif arb_id == RX_SC_RELAY_STATUS:
            if len(data) >= 1:
                killed = bool(data[0])
                if killed and not self.sc_relay_killed:
                    reason = data[1] if len(data) >= 2 else 0
                    log.info("SC relay KILLED (reason=%d) — motor disabled", reason)
                    self.sc_relay_killed = True
                    self.motor._hw_disabled = True
                elif not killed and self.sc_relay_killed:
                    log.info("SC relay CLEARED — motor re-enabled")
                    self.sc_relay_killed = False
                    self.motor._hw_disabled = False

        elif arb_id == TX_BATTERY_STATUS:
            # External battery injection (from fault_inject) — override model
            if len(data) >= 4:
                v = data[0] | (data[1] << 8)
                soc = data[2]
                self.battery.inject_voltage(v, soc)
                log.info("Battery override: %dmV, %d%% SOC", v, soc)

        elif arb_id == RX_TORQUE_REQUEST:
            if len(data) >= 4 and not self.estop_active:
                # CVC sends torque as uint16 LE at bytes 2-3 (Com signal 5,
                # bitPos=16, bitSize=16).  Value 0-1000 from torque LUT.
                torque_raw = struct.unpack_from('<H', data, 2)[0]
                self.motor.duty_pct = min(100.0, float(torque_raw) / 10.0)
                self.motor.direction = 1 if torque_raw > 0 else 0

        elif arb_id == RX_STEER_COMMAND:
            if len(data) >= 4 and not self.estop_active:
                raw = struct.unpack_from('<H', data, 2)[0]  # uint16 LE (DBC: unsigned)
                angle = raw * 0.01 - 45.0  # DBC: factor=0.01, offset=-45.0
                angle = max(-45.0, min(45.0, angle))
                self.steering.record_command(angle)

        elif arb_id == RX_BRAKE_COMMAND:
            if len(data) >= 3 and not self.estop_active:
                self.brake.record_command(float(data[2]))

    def _tx_motor_status(self):
        """Send Motor_Status (0x300) every 20ms."""
        payload = bytearray(8)
        # Byte 2-3: RPM (16-bit LE)
        rpm = self.motor.rpm_int
        payload[2] = rpm & 0xFF
        payload[3] = (rpm >> 8) & 0xFF
        # Byte 4: direction(2) + enable(1) + fault(5)
        direction = self.motor.direction & 0x03
        enable = 1 if self.motor.enabled else 0
        fault_bits = 0
        if self.motor.overcurrent:
            fault_bits |= 0x01
        if self.motor.overtemp:
            fault_bits |= 0x02
        if self.motor.stall_fault:
            fault_bits |= 0x04
        payload[4] = direction | (enable << 2) | (fault_bits << 3)
        # Byte 5: duty
        payload[5] = int(min(95, self.motor.duty_pct))
        # Byte 6: derating
        if self.motor.overtemp:
            payload[6] = 0
        elif self.motor.temp_c > 80:
            payload[6] = 50
        elif self.motor.temp_c > 60:
            payload[6] = 75
        else:
            payload[6] = 100

        data = self._encode_with_e2e(TX_MOTOR_STATUS, 0x0E, payload)
        self.bus.send(can.Message(arbitration_id=TX_MOTOR_STATUS,
                                  data=data, is_extended_id=False))

    def _tx_motor_current(self):
        """Send Motor_Current (0x301) every 10ms."""
        payload = bytearray(8)
        current = self.motor.current_ma_int
        payload[2] = current & 0xFF
        payload[3] = (current >> 8) & 0xFF
        # Byte 4: dir(1) + enable(1) + overcurrent(1) + torque_echo(8) starting at bit 35
        direction_bit = 0 if self.motor.direction != 2 else 1
        enable_bit = 1 if self.motor.enabled else 0
        oc_bit = 1 if self.motor.overcurrent else 0
        payload[4] = direction_bit | (enable_bit << 1) | (oc_bit << 2)
        # Torque echo at bits 35-42 (byte 4 bit 3 through byte 5)
        torque = int(self.motor.duty_pct) & 0xFF
        payload[4] |= (torque & 0x1F) << 3
        payload[5] = (torque >> 5) & 0x07

        data = self._encode_with_e2e(TX_MOTOR_CURRENT, 0x0F, payload)
        self.bus.send(can.Message(arbitration_id=TX_MOTOR_CURRENT,
                                  data=data, is_extended_id=False))

    def _tx_motor_temp(self):
        """Send Motor_Temperature (0x302) every 100ms."""
        payload = bytearray(6)
        # Byte 2: winding temp 1 (raw = temp + 40)
        payload[2] = int(self.motor.temp_c + 40) & 0xFF
        # Byte 3: winding temp 2 (board temp, slightly lower)
        payload[3] = int(self.motor.temp_c * 0.8 + 40) & 0xFF
        # Byte 4: derating percent
        if self.motor.overtemp:
            payload[4] = 0
        elif self.motor.temp_c > 80:
            payload[4] = 50
        elif self.motor.temp_c > 60:
            payload[4] = 75
        else:
            payload[4] = 100
        # Byte 5: fault status (4 bits)
        fault = 0
        if self.motor.overtemp:
            fault |= 0x04  # bit2 = overtemp
            fault |= 0x08  # bit3 = derating active
        elif self.motor.temp_c > 60:
            fault |= 0x08  # derating active
        payload[5] = fault & 0x0F

        data = self._encode_with_e2e(TX_MOTOR_TEMP, 0x00, payload)
        self.bus.send(can.Message(arbitration_id=TX_MOTOR_TEMP,
                                  data=data, is_extended_id=False))

    def _tx_battery_status(self):
        """Send Battery_Status (0x303) every 1000ms. No E2E."""
        payload = bytearray(4)
        v = self.battery.voltage_mv
        payload[0] = v & 0xFF
        payload[1] = (v >> 8) & 0xFF
        payload[2] = int(self.battery.soc) & 0xFF
        payload[3] = self.battery.status & 0x0F
        self.bus.send(can.Message(arbitration_id=TX_BATTERY_STATUS,
                                  data=payload, is_extended_id=False))

    def _send_dtc(self, dtc_code: int, ecu_source: int):
        """Send DTC_Broadcast (0x500, 8 bytes, no E2E). Only fires once per DTC."""
        if dtc_code in self._active_dtcs:
            return
        self._active_dtcs.add(dtc_code)
        count = self._dtc_occurrence.get(dtc_code, 0) + 1
        self._dtc_occurrence[dtc_code] = count

        payload = bytearray(8)
        payload[0] = dtc_code & 0xFF
        payload[1] = (dtc_code >> 8) & 0xFF
        payload[2] = 0x01  # DTC_Status: active
        payload[3] = ecu_source & 0xFF
        payload[4] = min(255, count)
        self.bus.send(can.Message(arbitration_id=TX_DTC_BROADCAST,
                                  data=payload, is_extended_id=False))
        log.info("DTC 0x%04X from ECU %d (occurrence %d)", dtc_code, ecu_source, count)

    def _check_and_send_dtcs(self):
        """Check all fault conditions and send DTCs."""
        if self.motor.overcurrent:
            self._send_dtc(DTC_OVERCURRENT, ECU_RZC)
        if self.steering.fault:
            self._send_dtc(DTC_STEER_FAULT, ECU_FZC)
        if self.brake.fault:
            self._send_dtc(DTC_BRAKE_FAULT, ECU_FZC)
        if self.battery.status == 0:  # critical_UV
            self._send_dtc(DTC_BATTERY_UV, ECU_RZC)

    def _tx_steering_status(self):
        """Send Steering_Status (0x200) every 20ms."""
        payload = bytearray(8)
        actual_raw = self.steering.actual_raw
        payload[2] = actual_raw & 0xFF
        payload[3] = (actual_raw >> 8) & 0xFF
        cmd_raw = self.steering.commanded_raw
        payload[4] = cmd_raw & 0xFF
        payload[5] = (cmd_raw >> 8) & 0xFF
        # Byte 6: fault(4) + mode(4)
        fault = 0x01 if self.steering.fault else 0x00
        payload[6] = fault
        # Byte 7: servo current (factor 10)
        payload[7] = min(255, self.steering.servo_current_ma // 10)

        data = self._encode_with_e2e(TX_STEERING_STATUS, 0x09, payload)
        self.bus.send(can.Message(arbitration_id=TX_STEERING_STATUS,
                                  data=data, is_extended_id=False))

    def _tx_brake_status(self):
        """Send Brake_Status (0x201) every 20ms."""
        payload = bytearray(8)
        payload[2] = self.brake.position_int
        payload[3] = int(self.brake.commanded_pct)
        # Bytes 4-5: servo current (16-bit LE)
        sc = self.brake.servo_current_ma
        payload[4] = sc & 0xFF
        payload[5] = (sc >> 8) & 0xFF
        # Byte 6: fault(4) + mode(4)
        payload[6] = 0x01 if self.brake.fault else 0x00

        data = self._encode_with_e2e(TX_BRAKE_STATUS, 0x0A, payload)
        self.bus.send(can.Message(arbitration_id=TX_BRAKE_STATUS,
                                  data=data, is_extended_id=False))

    def _tx_vehicle_state(self):
        """Send Vehicle_State (0x100) every 100ms."""
        payload = bytearray(8)
        # Byte 2: VehicleState (4 bits)
        payload[2] = self.vehicle_state & 0x0F
        # Byte 3: FaultMask (8 bits) — 0 for normal
        payload[3] = 0
        # Byte 4: TorqueLimit (0-100) — actual motor duty, not fixed ceiling
        payload[4] = int(self.motor.duty_pct) & 0xFF if self.vehicle_state == VS_RUN else 0
        # Byte 5: SpeedLimit (0-100)
        payload[5] = 100 if self.vehicle_state == VS_RUN else 0

        data = self._encode_with_e2e(TX_VEHICLE_STATE, 0x06, payload)
        self.bus.send(can.Message(arbitration_id=TX_VEHICLE_STATE,
                                  data=data, is_extended_id=False))

    def _tx_lidar_distance(self):
        """Send Lidar_Distance (0x220) every 10ms."""
        payload = bytearray(8)
        d = self.lidar.distance_cm
        payload[2] = d & 0xFF
        payload[3] = (d >> 8) & 0xFF
        ss = self.lidar.signal_strength
        payload[4] = ss & 0xFF
        payload[5] = (ss >> 8) & 0xFF
        zone = self.lidar.obstacle_zone
        sensor_status = 0x01 if self.lidar.fault else 0x00
        payload[6] = (zone & 0x0F) | ((sensor_status & 0x0F) << 4)

        data = self._encode_with_e2e(TX_LIDAR_DISTANCE, 0x0D, payload)
        self.bus.send(can.Message(arbitration_id=TX_LIDAR_DISTANCE,
                                  data=data, is_extended_id=False))

    async def run(self):
        """Main simulation loop at 100 Hz."""
        self.bus = can.interface.Bus(channel=self.channel,
                                     interface="socketcan")
        self._init_mqtt()
        log.info("Plant simulator started on %s", self.channel)
        log.info("Loaded DBC with %d messages", len(self.db.messages))

        dt = 0.01  # 10ms
        self._tick = 0

        try:
            while True:
                loop_start = time.monotonic()

                # Read all available CAN frames (non-blocking)
                while True:
                    msg = self.bus.recv(timeout=0)
                    if msg is None:
                        break
                    self._process_rx(msg)

                # Update physics — limit output in degraded/limp/safe states
                if self.estop_active or self.vehicle_state == VS_SAFE_STOP or self.sc_relay_killed:
                    self.motor.update(0, 0, dt)
                    self.steering.update(0, dt)
                    self.brake.update(100, dt)
                elif self.vehicle_state == VS_LIMP:
                    # Limp mode: cap torque at 15%, force gentle braking
                    capped_duty = min(self.motor.duty_pct, 15.0)
                    brake_load = max(self.brake.actual_pct / 100.0, 0.3)
                    self.motor.update(capped_duty, self.motor.direction, dt,
                                      brake_load=brake_load)
                    self.steering.update(self.steering.commanded_angle, dt)
                    self.brake.update(max(self.brake.commanded_pct, 30), dt)
                elif self.vehicle_state == VS_DEGRADED:
                    # DEGRADED: 50% torque cap (battery warning)
                    capped_duty = min(self.motor.duty_pct, 50.0)
                    brake_load = self.brake.actual_pct / 100.0
                    self.motor.update(capped_duty, self.motor.direction, dt,
                                      brake_load=brake_load)
                    self.steering.update(self.steering.commanded_angle, dt)
                    self.brake.update(self.brake.commanded_pct, dt)
                else:
                    brake_load = self.brake.actual_pct / 100.0
                    self.motor.update(self.motor.duty_pct,
                                      self.motor.direction, dt,
                                      brake_load=brake_load)
                    self.steering.update(self.steering.commanded_angle, dt)
                    self.brake.update(self.brake.commanded_pct, dt)

                self.battery.update(self.motor.current_ma, dt)
                self.lidar.update(dt)

                # Vehicle state machine
                self._startup_ticks += 1
                if self.estop_active:
                    if self.vehicle_state != VS_SAFE_STOP:
                        self.vehicle_state = VS_SAFE_STOP
                        log.info("Vehicle state -> SAFE_STOP (E-Stop)")
                elif self.vehicle_state == VS_INIT and self._startup_ticks >= 300:
                    # Transition to RUN after 3 seconds of startup
                    self.vehicle_state = VS_RUN
                    log.info("Vehicle state -> RUN (startup complete)")
                elif (self.vehicle_state == VS_SAFE_STOP
                      and not self.estop_active
                      and not self.brake.fault
                      and not self.steering.fault
                      and not self.motor.overcurrent
                      and not self.motor._hw_disabled
                      and not self.sc_relay_killed):
                    # Exit SAFE_STOP only when ALL safety triggers cleared
                    self.vehicle_state = VS_INIT
                    self._startup_ticks = 0
                    log.info("Vehicle state -> INIT (safety triggers cleared, re-initializing)")

                # Transition to SAFE_STOP/DEGRADED/LIMP on faults
                # SG-003 (ASIL D): steer fault → SAFE_STOP (SS-MOTOR-OFF)
                # SG-004 (ASIL D): brake fault → SAFE_STOP (SS-MOTOR-OFF)
                # SG-006 (ASIL A): overcurrent → SAFE_STOP (SS-MOTOR-OFF)
                safe_stop_fault = (
                    self.brake.fault or self.steering.fault
                    or self.motor.overcurrent
                    or self.motor._hw_disabled
                )
                battery_critical = self.battery.status == 0  # critical UV
                battery_warn = self.battery.status == 1      # UV warning

                if safe_stop_fault and self.vehicle_state in (
                        VS_RUN, VS_DEGRADED):
                    self.vehicle_state = VS_SAFE_STOP
                    log.info("Vehicle state -> SAFE_STOP (safety-critical fault)")
                elif battery_critical and self.vehicle_state in (
                        VS_RUN, VS_DEGRADED):
                    self.vehicle_state = VS_LIMP
                    log.info("Vehicle state -> LIMP (battery critical UV)")
                elif battery_warn and self.vehicle_state == VS_RUN:
                    self.vehicle_state = VS_DEGRADED
                    log.info("Vehicle state -> DEGRADED (battery undervoltage warning)")
                elif (not safe_stop_fault
                      and not battery_warn and not battery_critical
                      and self.vehicle_state in (VS_DEGRADED, VS_LIMP)):
                    self.vehicle_state = VS_RUN
                    log.info("Vehicle state -> RUN (faults cleared)")

                # TX schedule
                # Every 10ms: motor current, lidar
                self._tx_motor_current()
                self._tx_lidar_distance()

                # Every 20ms: motor status
                # Note: steering status (0x200) and brake status (0x201) broadcasts
                # removed — FZC firmware is the sole authority for these CAN IDs.
                # The plant sim keeps its internal steering/brake models for physics
                # constraints only.
                if self._tick % 2 == 0:
                    self._tx_motor_status()

                # Every 100ms: motor temperature, DTC check
                # Note: vehicle state broadcast removed — CVC firmware is the
                # sole authority for CAN 0x100 (Vehicle_State).  The plant sim
                # keeps its internal state machine for physics constraints only.
                if self._tick % 10 == 0:
                    self._tx_motor_temp()
                    self._check_and_send_dtcs()

                # Every 1000ms: battery
                if self._tick % 100 == 0:
                    self._tx_battery_status()

                self._tick += 1

                # Log status every 5 seconds
                if self._tick % 500 == 0:
                    log.info(
                        "RPM=%d  I=%dmA  T=%.1f°C  V=%dmV  steer=%.1f°  brake=%d%%",
                        self.motor.rpm_int,
                        self.motor.current_ma_int,
                        self.motor.temp_c,
                        self.battery.voltage_mv,
                        self.steering.actual_angle,
                        self.brake.position_int,
                    )

                # Sleep remainder of 10ms cycle
                elapsed = time.monotonic() - loop_start
                sleep_time = dt - elapsed
                if sleep_time > 0:
                    await asyncio.sleep(sleep_time)

        except KeyboardInterrupt:
            log.info("Plant simulator stopped")
        finally:
            if self._mqtt:
                self._mqtt.loop_stop()
                self._mqtt.disconnect()
            if self.bus:
                self.bus.shutdown()


def main():
    dbc_path = os.environ.get(
        "DBC_PATH",
        os.path.join(os.path.dirname(__file__), "..", "taktflow.dbc"),
    )
    channel = os.environ.get("CAN_CHANNEL", "vcan0")

    sim = PlantSimulator(dbc_path, channel)
    asyncio.run(sim.run())


if __name__ == "__main__":
    main()
