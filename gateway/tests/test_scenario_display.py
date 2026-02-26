"""Integration tests: click fault button → verify dashboard values.

Each test simulates clicking a button on the dashboard:
  1. Build the exact CAN frames the fault_inject scenario sends
  2. Feed them into the plant sim's _process_rx logic
  3. Run the plant sim's physics + state machine ticks
  4. Build the WS bridge snapshot
  5. Assert every displayed dashboard value

This tests the REAL pipeline: scenario CAN frames → plant sim → snapshot.
"""

import pytest

import can

from fault_inject.scenarios import (
    _torque_frame, _steer_frame, _brake_frame, _battery_frame,
    _estop_frame,
    CAN_TORQUE_REQUEST, CAN_STEER_COMMAND, CAN_BRAKE_COMMAND,
    CAN_BATTERY_STATUS, CAN_ESTOP,
)
from plant_sim.motor_model import MotorModel
from plant_sim.steering_model import SteeringModel
from plant_sim.brake_model import BrakeModel
from plant_sim.battery_model import BatteryModel
from plant_sim.lidar_model import LidarModel

# Vehicle state constants (from simulator.py)
VS_INIT = 0
VS_RUN = 1
VS_DEGRADED = 2
VS_LIMP = 3
VS_SAFE_STOP = 4

VEHICLE_STATES = {
    0: "INIT", 1: "RUN", 2: "DEGRADED", 3: "LIMP",
    4: "SAFE_STOP", 5: "SHUTDOWN",
}

# Speed factor (from ws_bridge/bridge.py)
SPEED_FACTOR = 2 * 3.14159 * 0.15 * 60 / (8 * 1000)


# ---------------------------------------------------------------------------
# SimHarness — mirrors PlantSimulator without CAN bus / DBC dependency
# ---------------------------------------------------------------------------

class SimHarness:
    """Test harness replicating PlantSimulator._process_rx + physics tick.

    Every method is copied from simulator.py so the test exercises the
    exact same CAN parsing, physics update, and state machine logic that
    runs in production.
    """

    def __init__(self):
        self.motor = MotorModel()
        self.steering = SteeringModel()
        self.brake = BrakeModel()
        self.battery = BatteryModel()
        self.lidar = LidarModel()
        self.estop_active = False
        self.vehicle_state = VS_INIT
        self._startup_ticks = 0

    # -- CAN RX: exact copy of PlantSimulator._process_rx ------------------

    def feed_can(self, arb_id: int, data: bytes):
        """Feed a CAN frame — same logic as PlantSimulator._process_rx."""
        if arb_id == 0x001:  # RX_ESTOP
            if len(data) >= 3:
                was_active = self.estop_active
                self.estop_active = bool(data[2] & 0x01)
                if self.estop_active and not was_active:
                    pass  # just set flag
                elif not self.estop_active and was_active:
                    # E-Stop cleared
                    self.motor.reset_faults()
                    self.steering.clear_fault()
                    self.brake.clear_fault()
                    self.battery.clear_override()
                    self.vehicle_state = VS_INIT
                    self._startup_ticks = 0
                elif not self.estop_active and not was_active:
                    # Reset command
                    self.motor.reset_faults()
                    self.steering.clear_fault()
                    self.brake.clear_fault()
                    self.battery.clear_override()
                    self.vehicle_state = VS_INIT
                    self._startup_ticks = 0

        elif arb_id == 0x303:  # TX_BATTERY_STATUS (external inject)
            if len(data) >= 4:
                v = data[0] | (data[1] << 8)
                soc = data[2]
                self.battery.inject_voltage(v, soc)

        elif arb_id == 0x101:  # RX_TORQUE_REQUEST
            if len(data) >= 4 and not self.estop_active:
                self.motor.duty_pct = data[2]
                self.motor.direction = data[3] & 0x03

        elif arb_id == 0x102:  # RX_STEER_COMMAND
            if len(data) >= 4 and not self.estop_active:
                raw = data[2] | (data[3] << 8)
                angle = max(-45.0, min(45.0, raw * 0.01 - 45.0))
                self.steering.record_command(angle)

        elif arb_id == 0x103:  # RX_BRAKE_COMMAND
            if len(data) >= 3 and not self.estop_active:
                self.brake.record_command(float(data[2]))

    # -- Physics tick: exact copy of PlantSimulator.run() loop body --------

    def tick(self, dt=0.01):
        """Run one 10ms physics tick + vehicle state machine."""
        # Physics update — same branching as simulator.py
        if self.estop_active or self.vehicle_state == VS_SAFE_STOP:
            self.motor.update(0, 0, dt)
            self.steering.update(0, dt)
            self.brake.update(100, dt)
        elif self.vehicle_state == VS_LIMP:
            capped_duty = min(self.motor.duty_pct, 15.0)
            brake_load = max(self.brake.actual_pct / 100.0, 0.3)
            self.motor.update(capped_duty, self.motor.direction, dt,
                              brake_load=brake_load)
            self.steering.update(self.steering.commanded_angle, dt)
            self.brake.update(max(self.brake.commanded_pct, 30), dt)
        elif self.vehicle_state == VS_DEGRADED:
            capped_duty = min(self.motor.duty_pct, 50.0)
            brake_load = self.brake.actual_pct / 100.0
            self.motor.update(capped_duty, self.motor.direction, dt,
                              brake_load=brake_load)
            self.steering.update(self.steering.commanded_angle, dt)
            self.brake.update(self.brake.commanded_pct, dt)
        else:
            brake_load = self.brake.actual_pct / 100.0
            self.motor.update(self.motor.duty_pct, self.motor.direction, dt,
                              brake_load=brake_load)
            self.steering.update(self.steering.commanded_angle, dt)
            self.brake.update(self.brake.commanded_pct, dt)

        self.battery.update(self.motor.current_ma, dt)
        self.lidar.update(dt)

        # Vehicle state machine — same logic as simulator.py
        self._startup_ticks += 1

        if self.estop_active:
            if self.vehicle_state != VS_SAFE_STOP:
                self.vehicle_state = VS_SAFE_STOP
        elif self.vehicle_state == VS_INIT and self._startup_ticks >= 300:
            self.vehicle_state = VS_RUN
        elif (self.vehicle_state == VS_SAFE_STOP
              and not self.estop_active
              and not self.brake.fault
              and not self.steering.fault
              and not self.motor.overcurrent
              and not self.motor._hw_disabled):
            self.vehicle_state = VS_INIT
            self._startup_ticks = 0

        # SG-003/SG-004/SG-006: steer fault, brake fault, or overcurrent → SAFE_STOP
        safe_stop_fault = (self.brake.fault or self.steering.fault
                           or self.motor.overcurrent
                           or self.motor._hw_disabled)
        battery_critical = self.battery.status == 0
        battery_warn = self.battery.status == 1

        if safe_stop_fault and self.vehicle_state in (VS_RUN, VS_DEGRADED):
            self.vehicle_state = VS_SAFE_STOP
        elif battery_critical and self.vehicle_state in (VS_RUN, VS_DEGRADED):
            self.vehicle_state = VS_LIMP
        elif battery_warn and self.vehicle_state == VS_RUN:
            self.vehicle_state = VS_DEGRADED
        elif (not safe_stop_fault
              and not battery_warn and not battery_critical
              and self.vehicle_state in (VS_DEGRADED, VS_LIMP)):
            self.vehicle_state = VS_RUN

    # -- Snapshot: matches ws_bridge/bridge.py TelemetryState.to_snapshot --

    def snapshot(self) -> dict:
        """Build the WS bridge snapshot dict."""
        speed_kmh = round(self.motor.rpm * SPEED_FACTOR, 1)
        return {
            "vehicle": {
                "state": self.vehicle_state,
                "state_name": VEHICLE_STATES.get(self.vehicle_state, "UNKNOWN"),
                "speed_kmh": speed_kmh,
            },
            "motor": {
                "rpm": self.motor.rpm_int,
                "current_ma": self.motor.current_ma_int,
                "temp_c": self.motor.temp_c_int,
                "duty_pct": int(self.motor.duty_pct),
                "overcurrent": 1 if self.motor.overcurrent else 0,
            },
            "steering": {
                "actual_deg": round(self.steering.actual_angle, 1),
                "commanded_deg": round(self.steering.commanded_angle, 1),
                "fault": 1 if self.steering.fault else 0,
            },
            "brake": {
                "position_pct": self.brake.position_int,
                "commanded_pct": int(self.brake.commanded_pct),
                "fault": 1 if self.brake.fault else 0,
            },
            "battery": {
                "voltage_mv": self.battery.voltage_mv,
                "soc_pct": int(self.battery.soc),
                "status": self.battery.status,
            },
            "lidar": {
                "distance_cm": self.lidar.distance_cm,
                "zone": self.lidar.obstacle_zone,
            },
        }

    # -- Helpers -----------------------------------------------------------

    def boot(self):
        """Run 300 ticks (3s) to transition INIT -> RUN."""
        for _ in range(300):
            self.tick()
        assert self.vehicle_state == VS_RUN


# ---------------------------------------------------------------------------
# Click "Normal Drive"
# ---------------------------------------------------------------------------

class TestClickNormalDrive:
    """Click 'Normal Drive' → 50% torque fwd, steer 0, brake 0."""

    @pytest.fixture()
    def snap(self):
        sim = SimHarness()
        sim.boot()

        # Click "Normal Drive" — exact CAN frames from scenarios.normal_drive()
        sim.feed_can(CAN_TORQUE_REQUEST, _torque_frame(50, 1))
        sim.feed_can(CAN_STEER_COMMAND, _steer_frame(0.0))
        sim.feed_can(CAN_BRAKE_COMMAND, _brake_frame(0))

        # Let physics settle (3s)
        for _ in range(300):
            sim.tick()

        return sim.snapshot()

    def test_state_run(self, snap):
        assert snap["vehicle"]["state_name"] == "RUN"

    def test_speed_around_14(self, snap):
        assert 12.0 < snap["vehicle"]["speed_kmh"] < 16.0

    def test_rpm_around_2000(self, snap):
        assert 1800 < snap["motor"]["rpm"] < 2200

    def test_no_overcurrent(self, snap):
        assert snap["motor"]["overcurrent"] == 0

    def test_temp_reasonable(self, snap):
        assert 20 < snap["motor"]["temp_c"] < 80

    def test_steering_centered(self, snap):
        assert abs(snap["steering"]["actual_deg"]) < 1.0
        assert snap["steering"]["fault"] == 0

    def test_brake_released(self, snap):
        assert snap["brake"]["position_pct"] == 0
        assert snap["brake"]["fault"] == 0

    def test_battery_normal(self, snap):
        assert snap["battery"]["voltage_mv"] > 11000
        assert snap["battery"]["status"] == 2

    def test_lidar_clear(self, snap):
        assert snap["lidar"]["distance_cm"] == 500
        assert snap["lidar"]["zone"] == 3


# ---------------------------------------------------------------------------
# Click "Overcurrent"
# ---------------------------------------------------------------------------

class TestClickOvercurrent:
    """Click 'Overcurrent' → 95% torque + 100% emergency brake (jam).

    SG-006 (ASIL A): Motor overcurrent → SS-MOTOR-OFF.
    Safe state = motor off, H-bridge disabled, brakes applied.
    Must transition to SAFE_STOP and STAY there until reset.
    """

    @pytest.fixture()
    def snap(self):
        sim = SimHarness()
        sim.boot()

        # Normal drive first
        sim.feed_can(CAN_TORQUE_REQUEST, _torque_frame(50, 1))
        for _ in range(300):
            sim.tick()

        # Click "Overcurrent" — exact CAN frames from scenarios.overcurrent()
        sim.feed_can(CAN_BRAKE_COMMAND, _brake_frame(100, brake_mode=2))
        sim.feed_can(CAN_TORQUE_REQUEST, _torque_frame(95, 1))

        # Let physics settle (3s) — motor self-protects
        for _ in range(300):
            sim.tick()

        return sim.snapshot(), sim.motor

    def test_state_safe_stop(self, snap):
        """SG-006: overcurrent → SAFE_STOP (SS-MOTOR-OFF), not DEGRADED."""
        snapshot, _ = snap
        assert snapshot["vehicle"]["state_name"] == "SAFE_STOP"

    def test_motor_hw_disabled(self, snap):
        """Motor self-protects: _hw_disabled persists."""
        _, motor = snap
        assert motor._hw_disabled is True

    def test_speed_near_zero(self, snap):
        """SG-006: vehicle must be stopped."""
        snapshot, _ = snap
        assert snapshot["vehicle"]["speed_kmh"] < 1.0

    def test_rpm_near_zero(self, snap):
        """SG-006: motor must be stopped (<=10 RPM = 0.07 km/h)."""
        snapshot, _ = snap
        assert snapshot["motor"]["rpm"] <= 10

    def test_current_near_zero(self, snap):
        """After self-protect, current decays to near zero."""
        snapshot, _ = snap
        assert snapshot["motor"]["current_ma"] < 100

    def test_brake_full(self, snap):
        """SG-006: SS-MOTOR-OFF requires brakes applied."""
        snapshot, _ = snap
        assert snapshot["brake"]["position_pct"] >= 95

    def test_steering_unaffected(self, snap):
        snapshot, _ = snap
        assert snapshot["steering"]["fault"] == 0


# ---------------------------------------------------------------------------
# Click "Steer Fault"
# ---------------------------------------------------------------------------

class TestClickSteerFault:
    """Click 'Steer Fault' → 10 rapid +/-40 deg oscillations."""

    @pytest.fixture()
    def snap(self):
        sim = SimHarness()
        sim.boot()

        # Normal drive first
        sim.feed_can(CAN_TORQUE_REQUEST, _torque_frame(50, 1))
        sim.feed_can(CAN_STEER_COMMAND, _steer_frame(0.0))
        sim.feed_can(CAN_BRAKE_COMMAND, _brake_frame(0))
        for _ in range(300):
            sim.tick()

        # Click "Steer Fault" — exact CAN frames from scenarios.steer_fault()
        # Scenario sends 10 pairs with 5ms sleep between — we interleave ticks
        for _ in range(10):
            sim.feed_can(CAN_STEER_COMMAND,
                         _steer_frame(40.0, rate_limit=50.0))
            sim.tick()  # ~10ms (scenario sleeps 5ms between sends)
            sim.feed_can(CAN_STEER_COMMAND,
                         _steer_frame(-40.0, rate_limit=50.0))
            sim.tick()

        # Let physics settle (3s) — vehicle must come to full stop
        for _ in range(300):
            sim.tick()

        return sim.snapshot()

    def test_state_safe_stop(self, snap):
        """SG-003 (ASIL D): steer fault → SAFE_STOP (SS-MOTOR-OFF).
        No mechanical fallback steering — vehicle must stop."""
        assert snap["vehicle"]["state_name"] == "SAFE_STOP"

    def test_steering_fault(self, snap):
        assert snap["steering"]["fault"] == 1

    def test_speed_near_zero(self, snap):
        """SG-003: vehicle must be stopped — no steering = no control."""
        assert snap["vehicle"]["speed_kmh"] < 1.0

    def test_brake_full(self, snap):
        """SG-003: SS-MOTOR-OFF requires brakes applied."""
        assert snap["brake"]["position_pct"] >= 95

    def test_rpm_near_zero(self, snap):
        """SG-003: motor must be stopped."""
        assert snap["motor"]["rpm"] <= 10

    def test_no_overcurrent(self, snap):
        assert snap["motor"]["overcurrent"] == 0


# ---------------------------------------------------------------------------
# Click "Brake Fault"
# ---------------------------------------------------------------------------

class TestClickBrakeFault:
    """Click 'Brake Fault' → 10 rapid 0%/100% alternations.

    ASIL D: Brake is safety-critical. Brake fault = unreliable braking.
    Safe state = vehicle STOPPED. Must transition to SAFE_STOP, not DEGRADED.
    Motor duty → 0%, brake → 100%, speed → 0.
    """

    @pytest.fixture()
    def snap(self):
        sim = SimHarness()
        sim.boot()

        # Normal drive first
        sim.feed_can(CAN_TORQUE_REQUEST, _torque_frame(50, 1))
        sim.feed_can(CAN_STEER_COMMAND, _steer_frame(0.0))
        sim.feed_can(CAN_BRAKE_COMMAND, _brake_frame(0))
        for _ in range(300):
            sim.tick()

        # Click "Brake Fault" — exact CAN frames from scenarios.brake_fault()
        for _ in range(10):
            sim.feed_can(CAN_BRAKE_COMMAND,
                         _brake_frame(100, brake_mode=2))
            sim.tick()
            sim.feed_can(CAN_BRAKE_COMMAND,
                         _brake_frame(0, brake_mode=0))
            sim.tick()

        # Let physics settle (3s) — vehicle must come to full stop
        for _ in range(300):
            sim.tick()

        return sim.snapshot()

    def test_state_safe_stop(self, snap):
        """ASIL D: brake fault → SAFE_STOP (not DEGRADED)."""
        assert snap["vehicle"]["state_name"] == "SAFE_STOP"

    def test_brake_fault(self, snap):
        assert snap["brake"]["fault"] == 1

    def test_brake_full(self, snap):
        """ASIL D: emergency braking — brake must be fully applied."""
        assert snap["brake"]["position_pct"] >= 95

    def test_speed_near_zero(self, snap):
        """ASIL D: vehicle must be stopped."""
        assert snap["vehicle"]["speed_kmh"] < 1.0

    def test_rpm_near_zero(self, snap):
        """ASIL D: motor must be stopped."""
        assert snap["motor"]["rpm"] < 10

    def test_steering_unaffected(self, snap):
        assert snap["steering"]["fault"] == 0

    def test_no_overcurrent(self, snap):
        assert snap["motor"]["overcurrent"] == 0


# ---------------------------------------------------------------------------
# Click "Battery Low"
# ---------------------------------------------------------------------------

class TestClickBatteryLow:
    """Click 'Battery Low' → voltage ramps 12.6V → 8.5V over 5s."""

    @pytest.fixture()
    def snap_phase1(self):
        """After phase 1: UV warning zone (~10.2V)."""
        sim = SimHarness()
        sim.boot()

        # Normal drive first
        sim.feed_can(CAN_TORQUE_REQUEST, _torque_frame(50, 1))
        for _ in range(300):
            sim.tick()

        # Phase 1: exact CAN frames from scenarios.battery_low() lines 299-305
        # 20 frames at 100ms intervals (= 10 ticks each)
        for i in range(20):
            frac = i / 19.0
            v = int(12600 - (12600 - 10200) * frac)
            soc = int(100 - (100 - 18) * frac)
            status = 1 if v < 10500 else 2
            sim.feed_can(CAN_BATTERY_STATUS,
                         _battery_frame(v, soc, status))
            for _ in range(10):  # 100ms
                sim.tick()

        return sim.snapshot()

    @pytest.fixture()
    def snap_phase2(self):
        """After phase 2: critical UV zone (~8.5V)."""
        sim = SimHarness()
        sim.boot()

        # Normal drive first
        sim.feed_can(CAN_TORQUE_REQUEST, _torque_frame(50, 1))
        for _ in range(300):
            sim.tick()

        # Phase 1 (same as above)
        for i in range(20):
            frac = i / 19.0
            v = int(12600 - (12600 - 10200) * frac)
            soc = int(100 - (100 - 18) * frac)
            status = 1 if v < 10500 else 2
            sim.feed_can(CAN_BATTERY_STATUS,
                         _battery_frame(v, soc, status))
            for _ in range(10):
                sim.tick()

        # Phase 2: exact CAN frames from scenarios.battery_low() lines 308-314
        # 30 frames at 100ms intervals
        for i in range(30):
            frac = i / 29.0
            v = int(10200 - (10200 - 8500) * frac)
            soc = int(18 - (18 - 3) * frac)
            status = 0 if v < 9000 else 1
            sim.feed_can(CAN_BATTERY_STATUS,
                         _battery_frame(v, soc, status))
            for _ in range(10):
                sim.tick()

        return sim.snapshot()

    # Phase 1 assertions
    def test_phase1_degraded(self, snap_phase1):
        assert snap_phase1["vehicle"]["state_name"] == "DEGRADED"

    def test_phase1_battery_uv_warn(self, snap_phase1):
        assert snap_phase1["battery"]["voltage_mv"] == 10200
        assert snap_phase1["battery"]["status"] == 1

    def test_phase1_soc_low(self, snap_phase1):
        assert snap_phase1["battery"]["soc_pct"] <= 20

    def test_phase1_speed_capped(self, snap_phase1):
        assert snap_phase1["vehicle"]["speed_kmh"] < 16.0

    # Phase 2 assertions
    def test_phase2_limp(self, snap_phase2):
        assert snap_phase2["vehicle"]["state_name"] == "LIMP"

    def test_phase2_battery_critical(self, snap_phase2):
        assert snap_phase2["battery"]["voltage_mv"] == 8500
        assert snap_phase2["battery"]["status"] == 0

    def test_phase2_soc_critical(self, snap_phase2):
        assert snap_phase2["battery"]["soc_pct"] <= 5

    def test_phase2_speed_very_low(self, snap_phase2):
        """LIMP: 15% duty + 30% forced brake → very slow."""
        assert snap_phase2["vehicle"]["speed_kmh"] < 5.0

    def test_phase2_brake_forced(self, snap_phase2):
        """LIMP forces minimum 30% braking."""
        assert snap_phase2["brake"]["position_pct"] >= 25


# ---------------------------------------------------------------------------
# Click "E-Stop"
# ---------------------------------------------------------------------------

class TestClickEStop:
    """Click 'E-Stop' → EStop_Active=1, all outputs disabled."""

    @pytest.fixture()
    def snap(self):
        sim = SimHarness()
        sim.boot()

        # Normal drive first
        sim.feed_can(CAN_TORQUE_REQUEST, _torque_frame(50, 1))
        for _ in range(300):
            sim.tick()

        # Click "E-Stop" — exact CAN frame from scenarios.estop()
        sim.feed_can(CAN_ESTOP, _estop_frame(active=True, source=1))

        # Let physics settle (2s) — motor decays, brake applied
        for _ in range(200):
            sim.tick()

        return sim.snapshot()

    def test_state_safe_stop(self, snap):
        assert snap["vehicle"]["state_name"] == "SAFE_STOP"

    def test_speed_near_zero(self, snap):
        assert snap["vehicle"]["speed_kmh"] < 2.0

    def test_rpm_near_zero(self, snap):
        assert snap["motor"]["rpm"] < 100

    def test_current_near_zero(self, snap):
        assert snap["motor"]["current_ma"] < 500

    def test_brake_full(self, snap):
        assert snap["brake"]["position_pct"] >= 95

    def test_steering_centered(self, snap):
        assert abs(snap["steering"]["actual_deg"]) < 1.0


# ---------------------------------------------------------------------------
# Click "Reset" (after E-Stop)
# ---------------------------------------------------------------------------

class TestClickReset:
    """Click 'Reset' after E-Stop → clear faults, INIT → RUN."""

    @pytest.fixture()
    def snap(self):
        sim = SimHarness()
        sim.boot()

        # Normal drive
        sim.feed_can(CAN_TORQUE_REQUEST, _torque_frame(50, 1))
        for _ in range(300):
            sim.tick()

        # Trigger E-Stop
        sim.feed_can(CAN_ESTOP, _estop_frame(active=True, source=1))
        for _ in range(200):
            sim.tick()
        assert sim.vehicle_state == VS_SAFE_STOP

        # Click "Reset" — exact CAN frames from scenarios.reset()
        sim.feed_can(CAN_ESTOP, _estop_frame(active=False, source=1))
        sim.feed_can(CAN_TORQUE_REQUEST, _torque_frame(0, 0))
        sim.feed_can(CAN_STEER_COMMAND, _steer_frame(0.0))
        sim.feed_can(CAN_BRAKE_COMMAND, _brake_frame(0, brake_mode=0))

        # Wait for INIT → RUN (3s = 300 ticks)
        for _ in range(300):
            sim.tick()

        return sim.snapshot()

    def test_state_run(self, snap):
        assert snap["vehicle"]["state_name"] == "RUN"

    def test_speed_zero(self, snap):
        """No torque commanded after reset."""
        assert snap["vehicle"]["speed_kmh"] < 1.0

    def test_no_motor_faults(self, snap):
        assert snap["motor"]["overcurrent"] == 0

    def test_no_steering_fault(self, snap):
        assert snap["steering"]["fault"] == 0

    def test_no_brake_fault(self, snap):
        assert snap["brake"]["fault"] == 0

    def test_brake_released(self, snap):
        assert snap["brake"]["position_pct"] == 0

    def test_battery_normal(self, snap):
        assert snap["battery"]["status"] == 2
        assert snap["battery"]["voltage_mv"] > 12000

    def test_lidar_default(self, snap):
        assert snap["lidar"]["distance_cm"] == 500
        assert snap["lidar"]["zone"] == 3
